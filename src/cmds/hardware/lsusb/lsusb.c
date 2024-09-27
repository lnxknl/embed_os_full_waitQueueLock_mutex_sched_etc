/**
 * @file
 *
 * @brief Show list of USB devices
 *
 * @date 14.05.2015
 * @author Anton Bondarev
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <drivers/usb/usb.h>

static void print_usage(void) {
	printf("Usage: lsusb [-h] [-v] [-s [[bus]:]devnum] [-d [vendor]:[product]]\n");
	printf("\t[-h]                    - print this help\n");
	printf("\t[-v]                    - print verbose output with device, configuration and interface descriptors\n");
	printf("\t[-s [[bus]:]devnum]     - print by device number and optional bus number (both in decimal)\n");
	printf("\t[-d [vendor]:[product]] - print by vendor id and/or product id (both in hexadecimal)\n");

}

static void print_error(void) {
	printf("Wrong parameters\n");
	print_usage();
}

static void show_usb_dev(struct usb_dev *usb_dev) {
	printf("Bus %03d Device %03d ID %04x:%04x\n",
			usb_dev->bus_idx,
			usb_dev->addr,
			usb_dev->dev_desc.id_vendor,
			usb_dev->dev_desc.id_product);


}

static void show_usb_desc_device(struct usb_dev *usb_dev) {
	printf(" Device Descriptor:\n"
			"   b_length             %5u\n"
			"   b_desc_type          %5u\n"
			"   bcd_usb              %2x.%02x\n"
			"   b_dev_class          %5u\n"
			"   b_dev_subclass       %5u\n"
			"   b_dev_protocol       %5u\n"
			"   b_max_packet_size    %5u\n"
			"   id_vendor           0x%04x\n"
			"   id_product          0x%04x \n"
			"   bcd_device           %2x.%02x\n"
			"   i_manufacter         %5u\n"
			"   i_product            %5u\n"
			"   i_serial_number      %5u\n"
			"   b_num_configurations %5u\n\n",
			usb_dev->dev_desc.b_length, 
			usb_dev->dev_desc.b_desc_type,
			usb_dev->dev_desc.bcd_usb >> 8, usb_dev->dev_desc.bcd_usb & 0xff,
			usb_dev->dev_desc.b_dev_class,
			usb_dev->dev_desc.b_dev_subclass,
			usb_dev->dev_desc.b_dev_protocol, 
			usb_dev->dev_desc.b_max_packet_size,
			usb_dev->dev_desc.id_vendor, 
			usb_dev->dev_desc.id_product,
			usb_dev->dev_desc.bcd_device >> 8, usb_dev->dev_desc.bcd_device & 0xff,
			usb_dev->dev_desc.i_manufacter,
			usb_dev->dev_desc.i_product,
			usb_dev->dev_desc.i_serial_number,
			usb_dev->dev_desc.b_num_configurations);
}

static void show_usb_desc_interface(struct usb_dev *usb_dev, int cfg) {
	int i;
	struct usb_dev_config *config = &usb_dev->usb_dev_configs[cfg];

	if (!config->usb_iface[0]->iface_desc[0]) {
		printf(" Interface Descriptor:\n"
			   "   No interfaces\n\n"
		);
		return;
	}

	for (i = 0; i < config->usb_iface_num; i ++) {
		struct usb_interface *iface;

		iface = config->usb_iface[i];
		printf(" Interface Descriptor:\n"
				"   b_length             %5u\n"
				"   b_desc_type          %5u\n"
				"   b_interface_number   %5u\n"
				"   b_alternate_setting  %5u\n"
				"   b_num_endpoints      %5u\n"
				"   b_interface_class    %5u\n"
				"   b_interface_subclass %5u\n"
				"   b_interface_protocol %5u\n"
				"   i_interface          %5u\n\n",
				iface->iface_desc[0]->b_length,
				iface->iface_desc[0]->b_desc_type,
				iface->iface_desc[0]->b_interface_number,
				iface->iface_desc[0]->b_alternate_setting,
				iface->iface_desc[0]->b_num_endpoints,
				iface->iface_desc[0]->b_interface_class,
				iface->iface_desc[0]->b_interface_subclass,
				iface->iface_desc[0]->b_interface_protocol,
				iface->iface_desc[0]->i_interface);
	}
}

static void show_usb_desc_configuration(struct usb_dev *usb_dev, int cfg) {
	struct usb_desc_configuration *config = usb_dev->usb_dev_configs[cfg].config_buf;
	
	if (!config) {
		printf(" Configuration Descriptor:\n"
			   "   No configurations\n\n"
		);
		return;
	}

	printf(" Configuration Descriptor:\n"
			"   b_length             %5u\n"
			"   b_desc_type          %5u\n"
			"   w_total_length      0x%04x\n"
			"   b_num_interfaces     %5u\n"
			"   bConfigurationValue  %5u\n"
			"   i_configuration      %5u\n"
			"   bm_attributes         0x%02x\n"
			"   b_max_power          %5u\n\n",
			config->b_length, 
			config->b_desc_type,
			config->w_total_length,
			config->b_num_interfaces, 
			config->b_configuration_value,
			config->i_configuration,
			config->bm_attributes,
			config->b_max_power);	
}


int main(int argc, char **argv) {
	struct usb_dev *usb_dev = NULL;
	int opt, flag = 0;

	char *cp;

	uint16_t bus = 0;
	int bus_set = 0;
	uint16_t devnum = 0;
	int devnum_set = 0;

	uint16_t vendor = 0;
	int vendor_set = 0;
	uint16_t product = 0;
	int product_set = 0;

	while (-1 != (opt = getopt(argc, argv, "s:d:h:v"))) {
		switch (opt) {
		case '?':
		case 'h':
			print_usage();
			return 0;
		case 'v':
			flag = 1;
			break;
		case 's':
			cp = strchr(optarg, ':');
			if (cp) {
				*cp++ = 0;
				if (*optarg) {
					bus_set = 1;
					bus = strtoul(optarg, NULL, 10);
				}
				if (*cp) {
					devnum_set = 1;
					devnum = strtoul(cp, NULL, 10);
				}
			} else {
				if (*optarg) {
					devnum_set = 1;
					devnum = strtoul(optarg, NULL, 10);
				}
			}
			break;
		case 'd':
			cp = strchr(optarg, ':');
			if (!cp) {
				print_error();
				return 0;
			}
			*cp++ = 0;
			if (*optarg) {
				vendor_set = 1;
				vendor = strtoul(optarg, NULL, 16);
			}
			if (*cp) {
				product_set = 1;
				product = strtoul(cp, NULL, 16);
			}
			break;
		default:
			print_error();
			return 0;
		}
	}

	while ((usb_dev = usb_dev_iterate(usb_dev))) {
		if ((bus_set && bus != usb_dev->bus_idx) ||
				(devnum_set && devnum != usb_dev->addr)) {
			continue;
		}

		if ((vendor_set && vendor != usb_dev->dev_desc.id_vendor) ||
				(product_set && product != usb_dev->dev_desc.id_product)) {
			continue;
		}

		show_usb_dev(usb_dev);
		if(flag) {
			int conf_cnt = 0;

			show_usb_desc_device(usb_dev);

			for (conf_cnt = 0;
					conf_cnt < usb_dev->dev_desc.b_num_configurations;
					conf_cnt ++) {

				show_usb_desc_configuration(usb_dev, conf_cnt);
				show_usb_desc_interface(usb_dev, conf_cnt);

			}
		}
	}

	return 0;
}
