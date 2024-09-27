/**
 * @file
 * @brief Intel e1000 gigabyte NIC series driver
 *
 * @date 01.10.2012
 * @author Anton Kozlov
 */

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <sys/mman.h>

#include <drivers/pci/pci.h>
#include <drivers/pci/pci_driver.h>
#include <kernel/irq.h>
#include <net/l2/ethernet.h>
#include <net/netdevice.h>
#include <net/inetdevice.h>
#include <net/skbuff.h>
#include <net/netfilter.h>

#include <net/l0/net_entry.h>

#include <hal/reg.h>

#include <mem/misc/pool.h>

#include <util/log.h>
#include <util/binalign.h>

#include "e1000.h"

static const struct pci_id e1000_id_table[] = {
	{ PCI_VENDOR_ID_INTEL, PCI_DEV_ID_INTEL_82540EM },
	{ PCI_VENDOR_ID_INTEL, PCI_DEV_ID_INTEL_82567V3 },
};

PCI_DRIVER_TABLE("e1000", e1000_init, e1000_id_table);

#define MDELAY 1000

/** Number of receive descriptors per card. */
#define E1000_RXDESC_NR 16

/** Number of transmit descriptors per card. */
#define E1000_TXDESC_NR 16

/** Size of each I/O buffer per descriptor. */
#define E1000_IOBUF_SIZE 2048

#define E1000_RX_CHECKSUM_LEN 4

#define E1000_MAX_RX_LEN (ETH_FRAME_LEN + E1000_RX_CHECKSUM_LEN)

static int e1000_stop(struct net_device *dev);

struct e1000_rx_desc {
	uint32_t buffer_address;
	uint32_t buffer_address_h;
	uint16_t length;
	uint16_t check_sum;
	uint8_t  status;
	uint8_t  error;
	uint16_t reserved;
};

struct e1000_tx_desc {
	uint32_t buffer_address;
	uint32_t buffer_address_h; /* not used */
	uint16_t length;
	uint8_t checksum_offload;
	uint8_t cmd;
	uint8_t status; /* + reserved, not used */
	uint8_t checksum_start;
	uint16_t special;
};

struct e1000_priv {
	struct sk_buff_head txing_queue;
	struct sk_buff_head tx_dev_queue;
	struct e1000_tx_desc tx_descs[E1000_TXDESC_NR] __attribute__((aligned(16)));

	struct e1000_rx_desc rx_descs[E1000_RXDESC_NR] __attribute__((aligned(16)));
	struct sk_buff *rx_skbs[E1000_RXDESC_NR];

	char link_status;
};

static inline struct e1000_priv *e1000_get_priv(struct net_device *dev) {
	return (struct e1000_priv *) binalign_bound((uintptr_t) netdev_priv(dev), 16);
}

static void mdelay(int value) {
	volatile int delay = value;

	while (delay --);

}

static volatile uint32_t *e1000_reg(struct net_device *dev, int offset) {
	return (volatile uint32_t *) (dev->base_addr + offset);
}

static int e1000_xmit(struct net_device *dev) {
	struct e1000_priv *nic_priv = e1000_get_priv(dev);
	uint16_t head;
	uint16_t tail;
	struct sk_buff *skb;

	/* Called from kernel space and IRQ. Don't want tail to be handled twice */
	irq_lock();
	{
		head = REG32_LOAD(e1000_reg(dev, E1000_REG_TDH));
		tail = REG32_LOAD(e1000_reg(dev, E1000_REG_TDT));

		if ((tail + 1 % E1000_TXDESC_NR) == head) {
			goto out_unlock;
		}

		skb = skb_queue_pop(&nic_priv->tx_dev_queue);

		if (skb == NULL) {
			goto out_unlock;
		}

		nic_priv->tx_descs[tail].buffer_address = (uint32_t) (uintptr_t) skb->mac.raw;
		nic_priv->tx_descs[tail].status = 0;
		nic_priv->tx_descs[tail].cmd = E1000_TX_CMD_EOP |
					E1000_TX_CMD_FCS |
					E1000_TX_CMD_RS;
		nic_priv->tx_descs[tail].length  = skb->len;

		++tail;
		tail %= E1000_TXDESC_NR;

		REG32_STORE(e1000_reg(dev, E1000_REG_TDT), tail);

		skb_queue_push(&nic_priv->txing_queue, skb);
	}
out_unlock:
	irq_unlock();
	return 0;
}

static int xmit(struct net_device *dev, struct sk_buff *skb) {
	struct e1000_priv *nic_priv = e1000_get_priv(dev);

	irq_lock();
	{
		skb_queue_push(&nic_priv->tx_dev_queue, skb);
	}
	irq_unlock();

	e1000_xmit(dev);

	return ENOERR;
}

static void txed_skb_clean(struct net_device *dev) {
	struct sk_buff *skb;
	struct e1000_priv *nic_priv = e1000_get_priv(dev);

	irq_lock();
	{
		while ((skb = skb_queue_pop(&nic_priv->txing_queue))) {
			skb_free(skb);
		}
	}
	irq_unlock();
}

static void e1000_rx(struct net_device *dev) {
	/*net_device_stats_t stat = get_eth_stat(dev);*/
	struct e1000_priv *nic_priv = e1000_get_priv(dev);
	struct sk_buff *skb, *new_skb;
	uint16_t head;
	uint16_t tail;
	uint16_t cur;

	/* nested irq locked to prevent already handled packets handled once more
	 * as tail updated at exit
	 */
	irq_lock();
	{
		head = REG32_LOAD(e1000_reg(dev, E1000_REG_RDH));
		tail = REG32_LOAD(e1000_reg(dev, E1000_REG_RDT));
		cur = (1 + tail) % E1000_RXDESC_NR;

		while (cur != head) {
			int len;

			if (!(nic_priv->rx_descs[cur].status)) {
				break;
			}

			len = nic_priv->rx_descs[cur].length - E1000_RX_CHECKSUM_LEN;

			if (0 != nf_test_raw(NF_CHAIN_INPUT,
						NF_TARGET_ACCEPT,
						(char *) (uintptr_t) nic_priv->rx_descs[cur].buffer_address,
						ETH_ALEN + (char *) (uintptr_t) nic_priv->rx_descs[cur].buffer_address,
						ETH_ALEN)) {
				goto drop_pack;
			}

			new_skb = skb_alloc(E1000_MAX_RX_LEN);
			if (!new_skb) {
				goto drop_pack;
			}

			skb = nic_priv->rx_skbs[cur];
			nic_priv->rx_skbs[cur] = new_skb;
			nic_priv->rx_descs[cur].buffer_address = (uint32_t) (uintptr_t) new_skb->mac.raw;
			assert(skb);

			skb = skb_realloc(len, skb);
			if (!skb) {
				goto drop_pack;
			}
			skb->dev = dev;
			netif_rx(skb);
drop_pack:
			tail = cur;

			cur = (1 + tail) % E1000_RXDESC_NR;
		}
		REG32_STORE(e1000_reg(dev, E1000_REG_RDT), tail);
	}
	irq_unlock();
}

static irq_return_t e1000_interrupt(unsigned int irq_num, void *dev_id) {
	struct e1000_priv *nic_priv = e1000_get_priv(dev_id);
	int cause = REG32_LOAD(e1000_reg(dev_id, E1000_REG_ICR));
	irq_return_t ret = IRQ_NONE;

	if (cause & (E1000_REG_ICR_RXO | E1000_REG_ICR_RXT)) {
		e1000_rx(dev_id);
		ret = IRQ_HANDLED;
	}

	if (cause & (E1000_REG_ICR_TXDW | E1000_REG_ICR_TXQE)) {
		txed_skb_clean(dev_id);
		e1000_xmit(dev_id);
		ret = IRQ_HANDLED;
	}

	if (cause & (E1000_REG_ICR_LSC)) {
		struct net_device *dev = dev_id;
		uint32_t status = REG32_LOAD(e1000_reg(dev_id, E1000_REG_STATUS));

		nic_priv->link_status = status & E1000_REG_STATUS_LU;

		if (nic_priv->link_status) {
			log_info("e1000: Link up");
			netdev_flag_up(dev, IFF_RUNNING);
			netif_carrier_on(dev);
		} else {
			log_info("e1000: Link down. Please check and insert network cable");
			netdev_flag_down(dev, IFF_RUNNING);
			netif_carrier_off(dev);
		}
		ret = IRQ_HANDLED;
	}

	return ret;
}

static int e1000_alloc_dma_rx(struct net_device *dev) {
	struct e1000_priv *nic_priv = e1000_get_priv(dev);

	for (int i = 0; i < E1000_RXDESC_NR; ++i) {
	        struct sk_buff *skb = skb_alloc(E1000_MAX_RX_LEN);
		if (skb == NULL) {
			return -ENOMEM;
		}
		nic_priv->rx_skbs[i] = skb;
	}
	return 0;
}

static void e1000_free_dma_rx(struct net_device *dev) {
	struct e1000_priv *nic_priv = e1000_get_priv(dev);

	for (int i = 0; i < E1000_RXDESC_NR; ++i) {
		skb_free(nic_priv->rx_skbs[i]);
	}
}

static int e1000_open(struct net_device *dev) {
	struct e1000_priv *nic_priv = e1000_get_priv(dev);
	int err;

	if ((err = e1000_alloc_dma_rx(dev))) {
		e1000_free_dma_rx(dev);
		return err;
	}

	mdelay(MDELAY);
	REG32_ORIN(e1000_reg(dev, E1000_REG_CTRL), E1000_REG_CTRL_RST);

	mdelay(MDELAY);
	REG32_ORIN(e1000_reg(dev, E1000_REG_CTRL), E1000_REG_CTRL_SLU | E1000_REG_CTRL_ASDE);
	REG32_ANDIN(e1000_reg(dev, E1000_REG_CTRL), ~E1000_REG_CTRL_LRST);
	REG32_ANDIN(e1000_reg(dev, E1000_REG_CTRL), ~E1000_REG_CTRL_PHY_RST);
	REG32_ANDIN(e1000_reg(dev, E1000_REG_CTRL), ~E1000_REG_CTRL_ILOS);
	REG32_STORE(e1000_reg(dev, E1000_REG_FCAL), 0);
	REG32_STORE(e1000_reg(dev, E1000_REG_FCAH), 0);
	REG32_STORE(e1000_reg(dev, E1000_REG_FCT), 0);
	REG32_STORE(e1000_reg(dev, E1000_REG_FCTTV), 0);
	REG32_ANDIN(e1000_reg(dev, E1000_REG_CTRL), ~E1000_REG_CTRL_VME);

	mdelay(MDELAY);
	/* Clear Multicast Table Array (MTA). */
	for (int i = 0; i < 128; i++) {
		volatile uint32_t *r = i + e1000_reg(dev, E1000_REG_MTA);
		REG32_STORE(r, 0);
	}

	mdelay(MDELAY);
	/* Clear Multicast Table Array (MTA). */
#if 0 /*cleaned up on init */
	/* Initialize statistics registers. */
	for (int i = 0; i < 64; i++) {
		volatile uint32_t *r = i + e1000_reg(dev, E1000_REG_CRCERRS);
		printk("0x%x\n", (unsigned int) r);
		REG32_LOAD(r);
	}
#endif
	mdelay(MDELAY);
	REG32_ORIN(e1000_reg(dev, E1000_REG_RCTL),  E1000_REG_RCTL_MPE);

	for (int i = 0; i < E1000_RXDESC_NR; i ++) {
	        struct sk_buff *skb = nic_priv->rx_skbs[i];
		nic_priv->rx_descs[i].buffer_address = (uint32_t) (uintptr_t) skb->mac.raw;
	}

	mdelay(MDELAY);
	REG32_STORE(e1000_reg(dev, E1000_REG_RDBAL), (uint32_t) (uintptr_t) nic_priv->rx_descs);
	REG32_STORE(e1000_reg(dev, E1000_REG_RDBAH), 0);
	REG32_STORE(e1000_reg(dev, E1000_REG_RDLEN), sizeof(struct e1000_rx_desc) * E1000_RXDESC_NR);
	REG32_STORE(e1000_reg(dev, E1000_REG_RDH), 0);
	REG32_STORE(e1000_reg(dev, E1000_REG_RDT), E1000_RXDESC_NR - 1);
	REG32_ORIN( e1000_reg(dev, E1000_REG_RCTL), E1000_REG_RCTL_EN);

	mdelay(MDELAY);
	REG32_STORE(e1000_reg(dev, E1000_REG_TDBAL), (uint32_t) (uintptr_t) nic_priv->tx_descs);
	REG32_STORE(e1000_reg(dev, E1000_REG_TDBAH), 0);
	REG32_STORE(e1000_reg(dev, E1000_REG_TDLEN), sizeof(struct e1000_tx_desc) * E1000_TXDESC_NR);
	REG32_STORE(e1000_reg(dev, E1000_REG_TDH), 0);
	REG32_STORE(e1000_reg(dev, E1000_REG_TDT), 0);
	REG32_ORIN(e1000_reg(dev, E1000_REG_TCTL), E1000_REG_TCTL_EN | E1000_REG_TCTL_PSP);

	mdelay(MDELAY);
	/* Enable interrupts. */
	REG32_STORE(e1000_reg(dev, E1000_REG_IMS),
				      E1000_REG_IMS_RXO  |
				      E1000_REG_IMS_RXT  |
				      E1000_REG_IMS_TXQE |
				      E1000_REG_IMS_TXDW |
				      E1000_REG_IMS_LSC );
	return ENOERR;
}

static int e1000_stop(struct net_device *dev) {

	REG32_ORIN(e1000_reg(dev, E1000_REG_CTRL), E1000_REG_CTRL_RST);
	mdelay(MDELAY);

	e1000_free_dma_rx(dev);

	return ENOERR;
}

static int set_mac_address(struct net_device *dev, const void *addr) {
	REG32_ANDIN(e1000_reg(dev, E1000_REG_RAH), ~E1000_REG_RAH_AV);

	REG32_STORE(e1000_reg(dev, E1000_REG_RAL), *(uint32_t *) addr);
	REG32_STORE(e1000_reg(dev, E1000_REG_RAH), *(uint16_t *) (addr + 4));

	REG32_ORIN(e1000_reg(dev, E1000_REG_RAH), E1000_REG_RAH_AV);
	REG32_ORIN(e1000_reg(dev, E1000_REG_RCTL),  E1000_REG_RCTL_MPE);

	memcpy(dev->dev_addr, addr, ETH_ALEN);

	return ENOERR;
}

static const struct net_driver _drv_ops = {
	.xmit = xmit,
	.start = e1000_open,
	.stop = e1000_stop,
	.set_macaddr = set_mac_address
};

static void e1000_enable_bus_mastering(struct pci_slot_dev *pci_dev) {
	const uint8_t devfn = PCI_DEVFN(pci_dev->slot, pci_dev->func);
	uint16_t pci_command;

	pci_read_config16(pci_dev->busn, devfn, PCI_COMMAND, &pci_command);
	pci_write_config16(pci_dev->busn, devfn, PCI_COMMAND, pci_command | PCI_COMMAND_MASTER);
}

static int e1000_init(struct pci_slot_dev *pci_dev) {
	int res;
	struct net_device *nic;
	struct e1000_priv *nic_priv;

	nic = (struct net_device *) etherdev_alloc(sizeof(struct e1000_priv) + 16);
	if (nic == NULL) {
		return -ENOMEM;
	}
	nic->drv_ops = &_drv_ops;
	nic->irq = pci_dev->irq;
	nic->base_addr = (uintptr_t) mmap_device_memory(
			(void *) (uintptr_t) (pci_dev->bar[0] & PCI_BASE_ADDR_IO_MASK),
			0x6000, /* XXX */
			PROT_WRITE | PROT_READ,
			MAP_FIXED,
			pci_dev->bar[0] & PCI_BASE_ADDR_IO_MASK);
	nic_priv = e1000_get_priv(nic);
	memset(nic_priv, 0, sizeof(*nic_priv));
	skb_queue_init(&nic_priv->txing_queue);
	skb_queue_init(&nic_priv->tx_dev_queue);

	res = irq_attach(pci_dev->irq, e1000_interrupt, IF_SHARESUP, nic, "e1000");
	if (res < 0) {
		return res;
	}

	e1000_enable_bus_mastering(pci_dev);

	return inetdev_register_dev(nic);
}
