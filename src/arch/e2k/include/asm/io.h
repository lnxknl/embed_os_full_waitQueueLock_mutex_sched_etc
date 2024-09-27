/**
 * @file io.h
 * @brief
 * @author Denis Deryugin <deryugin.denis@gmail.com>
 * @version
 * @date 18.12.2017
 */

#ifndef E2K_IO_H_
#define E2K_IO_H_

#include <stdint.h>

#include <e2k_mas.h>
#include <e2k_api.h>
#include <asm/hal/memory_barrier.h>


static inline void e2k_write8(uint8_t val, uintptr_t addr) {
	E2K_WRITE_MAS_B(addr, val, MAS_IOADDR);
#if 0
       asm volatile ("stb,2\t0x0, [%0] %2, %1" :
		: "r" (addr), "r" (val), "i" (MAS_IOADDR) : "memory");
       wmb();
#endif
}

static inline void e2k_write16(uint16_t val, uintptr_t addr) {
	E2K_WRITE_MAS_H(addr, val, MAS_IOADDR);
#if 0
       asm volatile ("sth,2\t0x0, [%0] %2, %1" :
		: "r" (addr), "r" (val), "i" (MAS_IOADDR) : "memory");
#endif
}

static inline void e2k_write32(uint32_t val, uintptr_t addr) {
	E2K_WRITE_MAS_W(addr, val, MAS_IOADDR);
#if 0
       asm volatile ("stw,2\t0x0, [%0] %2, %1" :
		: "r" (addr), "r" (val), "i" (MAS_IOADDR) : "memory");
#endif
}

static inline void e2k_write64(uint64_t val, uintptr_t addr) {
	E2K_WRITE_MAS_D(addr, val, MAS_IOADDR);
#if 0
       asm volatile ("std,2\t0x0, [%0] %2, %1" :
		: "r" (addr), "r" (val), "i" (MAS_IOADDR) : "memory");
#endif
}

static inline uint8_t e2k_read8(uintptr_t addr) {
	return E2K_READ_MAS_B(addr, MAS_IOADDR);
#if 0
	register uint8_t res;
	asm volatile ("ldb,2 \t0x0, [%1] %2, %0" :
		"=r" (res) : "r" (addr), "i" (MAS_IOADDR));
	wmb();
	return res;
#endif
}

static inline uint16_t e2k_read16(uintptr_t addr) {
	return E2K_READ_MAS_H(addr, MAS_IOADDR);
#if 0
	register uint16_t res;
	asm volatile ("ldh,2 \t0x0, [%1] %2, %0" :
		"=r" (res) : "r" (addr), "i" (MAS_IOADDR));
	return res;
#endif
}

static inline uint32_t e2k_read32(uintptr_t addr) {
	return E2K_READ_MAS_W(addr, MAS_IOADDR);
#if 0
	register uint32_t res;
	asm volatile ("ldw,2 \t0x0, [%1] %2, %0" :
		"=r" (res) : "r" (addr), "i" (MAS_IOADDR));
	return res;
#endif
}

static inline uint64_t e2k_read64(uintptr_t addr) {
	return E2K_READ_MAS_D(addr, MAS_IOADDR);
#if 0
	register uint64_t res;
	asm volatile ("ldd,2 \t0x0, [%1] %2, %0" :
		"=r" (res) : "r" (addr), "i" (MAS_IOADDR));
	return res;
#endif
}

/* x86 port simulation */
//#define E2K_X86_IO_PORT_BASE	0xfff0000000UL
#define E2K_X86_IO_PORT_BASE    (uintptr_t)0x0000000101000000UL

static inline void e2k_out8(uint8_t val, int port) {
	e2k_write8(val, (E2K_X86_IO_PORT_BASE + port));
}

static inline uint8_t e2k_in8(int port) {
	return e2k_read8((E2K_X86_IO_PORT_BASE + port));
}

#define out8(val, port)    e2k_out8(val, port)
#define in8(port)          e2k_read8(port)

#endif /* E2K_IO_H_ */
