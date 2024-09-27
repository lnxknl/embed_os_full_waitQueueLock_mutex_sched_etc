#include <gen_board_conf.h>
#include <stm32.h>

struct uart_conf uarts[] = {
	[1] = {
		.status = DISABLED,
		.name = "USART1",
		.dev = {
			.irqs = {
				VAL("", 37),
			},
			.pins = {
				PIN("TX", PB, PIN_6, AF7),
				PIN("RX", PB, PIN_7, AF7),
			},
			.clocks = {
				VAL("TX",   CLK_GPIOB),
				VAL("RX",   CLK_GPIOB),
				VAL("UART", CLK_USART1),
			}
		},
		.baudrate = 115200,
	},
	[2] = {
		.status = DISABLED,
		.name = "USART2",
		.dev = {
			.irqs = {
				VAL("", 38),
			},
			.pins = {
				PIN("TX", PA, PIN_2, AF7),
				PIN("RX", PA, PIN_3, AF7),
			},
			.clocks = {
				VAL("TX",   CLK_GPIOA),
				VAL("RX",   CLK_GPIOA),
				VAL("UART", CLK_USART2),
			}
		},
		.baudrate = 115200,
	},
};

struct spi_conf spis[] = {
	[1] = {
		.status = DISABLED,
		.name = "SPI1",
		.dev = {
			.pins = {
				PIN("SCK",  PA, PIN_5, AF5),
				PIN("MISO", PA, PIN_6, AF5),
				PIN("MOSI", PA, PIN_7, AF5),
				PIN("CS",   PA, PIN_15, NOAF),
			},
			.clocks = {
				VAL("SCK",  CLK_GPIOA),
				VAL("MISO", CLK_GPIOA),
				VAL("MOSI", CLK_GPIOA),
				VAL("CS",   CLK_GPIOA),
				VAL("SPI",  CLK_SPI1),
			}
		},
	},
	[2] = {
		.status = DISABLED,
		.name = "SPI2",
		.dev = {
			.pins = {
				PIN("SCK",  PB, PIN_10, AF5),
				PIN("MISO", PB, PIN_14, AF5),
				PIN("MOSI", PB, PIN_15, AF5),
				PIN("CS",   PB, PIN_12, NOAF),
			},
			.clocks = {
				VAL("SCK",  CLK_GPIOB),
				VAL("MISO", CLK_GPIOB),
				VAL("MOSI", CLK_GPIOB),
				VAL("CS",   CLK_GPIOB),
				VAL("SPI",  CLK_SPI2),
			}
		},
	},
	[3] = {
		.status = DISABLED,
		.name = "SPI3",
		.bits_per_word = 16,
		.clk_div = 8,
		.dev = {
			.pins = {
				PIN("SCK",  PC, PIN_10, AF6),
				PIN("MISO", PC, PIN_11, AF6),
				PIN("MOSI", PC, PIN_12, AF6),
				PIN("CS",   PE, PIN_0, NOAF),	// for WiFi module
			},
			.clocks = {
				VAL("SCK",  CLK_GPIOC),
				VAL("MISO", CLK_GPIOC),
				VAL("MOSI", CLK_GPIOC),
				VAL("CS",   CLK_GPIOE),		// for WiFi module
				VAL("SPI",  CLK_SPI3),
			}
		},
	},
};

struct i2c_conf i2cs[] = {
	[1] = {
		.status = ENABLED,
		.name = "I2C1",
		.dev = {
			.irqs = {
				VAL("EVENT_IRQ", 31),
				VAL("ERROR_IRQ", 32),
			},
			.pins = {
				PIN("SCL", GPIO_PORT_B, PIN_6, AF4),
				PIN("SDA", GPIO_PORT_B, PIN_9, AF4),
			},
			.clocks = {
				VAL("SCL", CLK_GPIOB),
				VAL("SDA", CLK_GPIOB),
				VAL("I2C", CLK_I2C1),
			}
		},
	},
	[2] = {
		.status = ENABLED,
		.name = "I2C2",
		.dev = {
			.irqs = {
				VAL("EVENT_IRQ", 33),
				VAL("ERROR_IRQ", 34),
			},
			.pins = {
				PIN("SCL", GPIO_PORT_B, PIN_10, AF4),
				PIN("SDA", GPIO_PORT_B, PIN_11, AF4),
			},
			.clocks = {
				VAL("SCL", CLK_GPIOB),
				VAL("SDA", CLK_GPIOB),
				VAL("I2C", CLK_I2C1),
			}
		},
	},

};


EXPORT_CONFIG(UART(uarts), SPI(spis), I2C(i2cs))
