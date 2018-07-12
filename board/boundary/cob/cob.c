/*
 * Copyright (C) 2015, Boundary Devices <info@boundarydevices.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <malloc.h>
#include <asm/arch/mx6-pins.h>
#include <linux/errno.h>
#include <asm/gpio.h>
#include <asm/imx-common/boot_mode.h>
#include <asm/imx-common/fbpanel.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/imx-common/mxc_i2c.h>
#include <asm/imx-common/spi.h>
#include <mmc.h>
#include <fsl_esdhc.h>
#include <linux/fb.h>
#include <ipu_pixfmt.h>
#include <asm/arch/crm_regs.h>
#include <i2c.h>
#include <input.h>
#include <splash.h>
#include <usb/ehci-ci.h>
#include <pwm.h>
#include "../common/bd_common.h"
#include "../common/padctrl.h"

DECLARE_GLOBAL_DATA_PTR;

#define I2C_PAD_CTRL	(PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS |	\
	PAD_CTL_ODE | PAD_CTL_SRE_FAST)

#define RGB_PAD_CTRL	PAD_CTL_DSE_120ohm

#define SPI_PAD_CTRL	(PAD_CTL_HYS | PAD_CTL_SPEED_MED |	\
	PAD_CTL_DSE_40ohm     | PAD_CTL_SRE_FAST)

#define UART_PAD_CTRL	(PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_HYS | PAD_CTL_SRE_FAST)

#define USDHC_CLK_PAD_CTRL (PAD_CTL_SPEED_LOW | PAD_CTL_DSE_40ohm | \
	PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL	(USDHC_CLK_PAD_CTRL | PAD_CTL_PUS_47K_UP)

/*
 *
 */
static const iomux_v3_cfg_t init_pads[] = {
	/* ECSPI1 pads (serial nor eeprom) */
	IOMUX_PAD_CTRL(EIM_D17__ECSPI1_MISO, SPI_PAD_CTRL),
	IOMUX_PAD_CTRL(EIM_D18__ECSPI1_MOSI, SPI_PAD_CTRL),
	IOMUX_PAD_CTRL(EIM_D16__ECSPI1_SCLK, SPI_PAD_CTRL),
#define GP_ECSPI1_NOR_CS	IMX_GPIO_NR(3, 19)
	IOMUX_PAD_CTRL(EIM_D19__GPIO3_IO19, WEAK_PULLUP),

	/* gpio_keys */
#define GP_GPIOKEYS_MENU		IMX_GPIO_NR(1, 0)
	IOMUX_PAD_CTRL(GPIO_0__GPIO1_IO00, WEAK_PULLUP),
#define GP_GPIOKEYS_HOME		IMX_GPIO_NR(1, 2)
	IOMUX_PAD_CTRL(GPIO_2__GPIO1_IO02, WEAK_PULLUP),
#define GP_GPIOKEYS_PB1_I		IMX_GPIO_NR(1, 8)	/* J9 pin 6 */
	IOMUX_PAD_CTRL(GPIO_8__GPIO1_IO08, WEAK_PULLUP),
#define GP_GPIOKEYS_PB2_I		IMX_GPIO_NR(7, 12)	/* J10 pin 6 */
	IOMUX_PAD_CTRL(GPIO_17__GPIO7_IO12, WEAK_PULLUP),

	/* GPIO-leds */
#define GP_GPIOLEDS_1		IMX_GPIO_NR(2, 19)
	IOMUX_PAD_CTRL(EIM_A19__GPIO2_IO19, WEAK_PULLDN_OUTPUT),
#define GP_GPIOLEDS_2		IMX_GPIO_NR(2, 18)
	IOMUX_PAD_CTRL(EIM_A20__GPIO2_IO18, WEAK_PULLDN_OUTPUT),

	/* Hog - GPIOs */
#define GP_HEATER_EN		IMX_GPIO_NR(4, 10)
	IOMUX_PAD_CTRL(KEY_COL2__GPIO4_IO10, WEAK_PULLDN_OUTPUT),

#define GP_LCD_DAY_BACKLIGHT_EN	IMX_GPIO_NR(4, 15)
	IOMUX_PAD_CTRL(KEY_ROW4__GPIO4_IO15, WEAK_PULLDN_OUTPUT),
#define GP_LCD_NIGHT_BACKLIGHT_EN IMX_GPIO_NR(4, 11)
	IOMUX_PAD_CTRL(KEY_ROW2__GPIO4_IO11, WEAK_PULLDN_OUTPUT),

#define GP_TX			IMX_GPIO_NR(2, 30)
	IOMUX_PAD_CTRL(EIM_EB2__GPIO2_IO30, WEAK_PULLUP),
#define GP_RX			IMX_GPIO_NR(2, 31)
	IOMUX_PAD_CTRL(EIM_EB3__GPIO2_IO31, WEAK_PULLUP),

#define GP_HEATER_FAULT		IMX_GPIO_NR(5, 0)
	IOMUX_PAD_CTRL(EIM_WAIT__GPIO5_IO00, WEAK_PULLUP),

#define GP_DA1_OUTA		IMX_GPIO_NR(3, 0)
	IOMUX_PAD_CTRL(EIM_DA0__GPIO3_IO00, WEAK_PULLUP),
#define GP_DA1_OUTB		IMX_GPIO_NR(3, 1)
	IOMUX_PAD_CTRL(EIM_DA1__GPIO3_IO01, WEAK_PULLUP),
#define GP_DB1_OUTA		IMX_GPIO_NR(3, 2)
	IOMUX_PAD_CTRL(EIM_DA2__GPIO3_IO02, WEAK_PULLUP),
#define GP_DB1_OUTB		IMX_GPIO_NR(3, 3)
	IOMUX_PAD_CTRL(EIM_DA3__GPIO3_IO03, WEAK_PULLUP),
#define GP_DA2_OUTA		IMX_GPIO_NR(3, 4)
	IOMUX_PAD_CTRL(EIM_DA4__GPIO3_IO04, WEAK_PULLUP),
#define GP_DA2_OUTB		IMX_GPIO_NR(3, 5)
	IOMUX_PAD_CTRL(EIM_DA5__GPIO3_IO05, WEAK_PULLUP),
#define GP_DB2_OUTA		IMX_GPIO_NR(3, 6)
	IOMUX_PAD_CTRL(EIM_DA6__GPIO3_IO06, WEAK_PULLUP),
#define GP_DB2_OUTB		IMX_GPIO_NR(3, 7)
	IOMUX_PAD_CTRL(EIM_DA7__GPIO3_IO07, WEAK_PULLUP),

	/* Hog - testpoints */
#define GP_TP5			IMX_GPIO_NR(4, 8)
	IOMUX_PAD_CTRL(KEY_COL1__GPIO4_IO08, WEAK_PULLUP),
#define GP_TP6			IMX_GPIO_NR(4, 9)
	IOMUX_PAD_CTRL(KEY_ROW1__GPIO4_IO09, WEAK_PULLUP),
#define GP_TP71			IMX_GPIO_NR(1, 30)
	IOMUX_PAD_CTRL(ENET_TXD0__GPIO1_IO30, WEAK_PULLUP),
#define GP_TP72			IMX_GPIO_NR(4, 6)
	IOMUX_PAD_CTRL(KEY_COL0__GPIO4_IO06, WEAK_PULLUP),
#define GP_TP74			IMX_GPIO_NR(2, 7)
	IOMUX_PAD_CTRL(NANDF_D7__GPIO2_IO07, WEAK_PULLDN),
#define GP_TP75			IMX_GPIO_NR(1, 11)
	IOMUX_PAD_CTRL(SD2_CMD__GPIO1_IO11, WEAK_PULLUP),
#define GP_TP76			IMX_GPIO_NR(1, 10)
	IOMUX_PAD_CTRL(SD2_CLK__GPIO1_IO10, WEAK_PULLUP),
#define GP_TP78			IMX_GPIO_NR(1, 7)
	IOMUX_PAD_CTRL(GPIO_7__GPIO1_IO07, WEAK_PULLUP),
#define GP_TP79			IMX_GPIO_NR(1, 4)
	IOMUX_PAD_CTRL(GPIO_4__GPIO1_IO04, WEAK_PULLUP),

#define GP_I2C3_CRTOUCH			IMX_GPIO_NR(1, 9)
	IOMUX_PAD_CTRL(GPIO_9__GPIO1_IO09, WEAK_PULLUP),
#define GP_I2C3_CRTOUCH_WAKE		IMX_GPIO_NR(2, 27)
	IOMUX_PAD_CTRL(EIM_LBA__GPIO2_IO27, WEAK_PULLUP),
#define GP_I2C3_CRTOUCH_RESET		IMX_GPIO_NR(2, 26)
	IOMUX_PAD_CTRL(EIM_RW__GPIO2_IO26, WEAK_PULLDN),

	/* PWM1 */
#define GP_LCD_DAY_BACKLIGHT_PWM IMX_GPIO_NR(1, 21)
	IOMUX_PAD_CTRL(SD1_DAT3__GPIO1_IO21, WEAK_PULLDN),

	/* PWM2 */
#define GP_LCD_NIGHT_BACKLIGHT_PWM IMX_GPIO_NR(1, 19)
	IOMUX_PAD_CTRL(SD1_DAT2__GPIO1_IO19, WEAK_PULLDN),

	/* Regulator - usbotg */
#define GP_USB_OTG_PWR		IMX_GPIO_NR(3, 22)
	IOMUX_PAD_CTRL(EIM_D22__GPIO3_IO22, WEAK_PULLDN_OUTPUT),

	/* UART1 */
	IOMUX_PAD_CTRL(SD3_DAT7__UART1_TX_DATA, UART_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_DAT6__UART1_RX_DATA, UART_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_DAT0__UART1_CTS_B, UART_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_DAT1__UART1_RTS_B, UART_PAD_CTRL),

	/* UART2 */
	IOMUX_PAD_CTRL(EIM_D26__UART2_TX_DATA, UART_PAD_CTRL),
	IOMUX_PAD_CTRL(EIM_D27__UART2_RX_DATA, UART_PAD_CTRL),

	/* UART3 */
	IOMUX_PAD_CTRL(EIM_D24__UART3_TX_DATA, UART_PAD_CTRL),
	IOMUX_PAD_CTRL(EIM_D25__UART3_RX_DATA, UART_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_DAT3__UART3_CTS_B, UART_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_RST__UART3_RTS_B, UART_PAD_CTRL),

	/* USBH1 */
#define GP_USBH1_SOURCE	IMX_GPIO_NR(2, 28)	/* 1 - imx, 0 - other board */
	IOMUX_PAD_CTRL(EIM_EB0__GPIO2_IO28, WEAK_PULLDN),
#define GP_USBH1_FP_OC	IMX_GPIO_NR(4, 7)
	IOMUX_PAD_CTRL(KEY_ROW0__GPIO4_IO07, WEAK_PULLDN),
	IOMUX_PAD_CTRL(EIM_D30__USB_H1_OC, WEAK_PULLUP),

	/* USB OTG */
	IOMUX_PAD_CTRL(GPIO_1__USB_OTG_ID, WEAK_PULLUP),
	IOMUX_PAD_CTRL(KEY_COL4__USB_OTG_OC, WEAK_PULLUP),

	/* USDHC4 - eMMC */
	IOMUX_PAD_CTRL(SD4_CLK__SD4_CLK, USDHC_CLK_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_CMD__SD4_CMD, USDHC_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_DAT0__SD4_DATA0, USDHC_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_DAT1__SD4_DATA1, USDHC_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_DAT2__SD4_DATA2, USDHC_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_DAT3__SD4_DATA3, USDHC_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_DAT4__SD4_DATA4, USDHC_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_DAT5__SD4_DATA5, USDHC_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_DAT6__SD4_DATA6, USDHC_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_DAT7__SD4_DATA7, USDHC_PAD_CTRL),
#define GP_EMMC_RESET		IMX_GPIO_NR(2, 6)
	IOMUX_PAD_CTRL(NANDF_D6__GPIO2_IO06, WEAK_PULLUP),
};

static const iomux_v3_cfg_t rgb_pads[] = {
	IOMUX_PAD_CTRL(DI0_DISP_CLK__IPU1_DI0_DISP_CLK, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DI0_PIN15__IPU1_DI0_PIN15, RGB_PAD_CTRL),	/* DRDY */
	IOMUX_PAD_CTRL(DI0_PIN2__IPU1_DI0_PIN02, RGB_PAD_CTRL),		/* Hsync */
	IOMUX_PAD_CTRL(DI0_PIN3__IPU1_DI0_PIN03, RGB_PAD_CTRL),		/* Vsync */
	IOMUX_PAD_CTRL(DISP0_DAT0__IPU1_DISP0_DATA00, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT1__IPU1_DISP0_DATA01, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT2__IPU1_DISP0_DATA02, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT3__IPU1_DISP0_DATA03, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT4__IPU1_DISP0_DATA04, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT5__IPU1_DISP0_DATA05, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT6__IPU1_DISP0_DATA06, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT7__IPU1_DISP0_DATA07, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT8__IPU1_DISP0_DATA08, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT9__IPU1_DISP0_DATA09, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT10__IPU1_DISP0_DATA10, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT11__IPU1_DISP0_DATA11, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT12__IPU1_DISP0_DATA12, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT13__IPU1_DISP0_DATA13, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT14__IPU1_DISP0_DATA14, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT15__IPU1_DISP0_DATA15, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT16__IPU1_DISP0_DATA16, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT17__IPU1_DISP0_DATA17, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT18__IPU1_DISP0_DATA18, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT19__IPU1_DISP0_DATA19, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT20__IPU1_DISP0_DATA20, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT21__IPU1_DISP0_DATA21, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT22__IPU1_DISP0_DATA22, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT23__IPU1_DISP0_DATA23, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(SD1_DAT3__PWM1_OUT, WEAK_PULLDN),
};

static const iomux_v3_cfg_t rgb_gpio_pads[] = {
	IOMUX_PAD_CTRL(DI0_DISP_CLK__GPIO4_IO16, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DI0_PIN15__GPIO4_IO17, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DI0_PIN2__GPIO4_IO18, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DI0_PIN3__GPIO4_IO19, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DI0_PIN4__GPIO4_IO20, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT0__GPIO4_IO21, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT1__GPIO4_IO22, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT2__GPIO4_IO23, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT3__GPIO4_IO24, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT4__GPIO4_IO25, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT5__GPIO4_IO26, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT6__GPIO4_IO27, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT7__GPIO4_IO28, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT8__GPIO4_IO29, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT9__GPIO4_IO30, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT10__GPIO4_IO31, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT11__GPIO5_IO05, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT12__GPIO5_IO06, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT13__GPIO5_IO07, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT14__GPIO5_IO08, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT15__GPIO5_IO09, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT16__GPIO5_IO10, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT17__GPIO5_IO11, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT18__GPIO5_IO12, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT19__GPIO5_IO13, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT20__GPIO5_IO14, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT21__GPIO5_IO15, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT22__GPIO5_IO16, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT23__GPIO5_IO17, WEAK_PULLUP),
	IOMUX_PAD_CTRL(SD1_DAT3__GPIO1_IO21, WEAK_PULLDN),
};

static const struct i2c_pads_info i2c_pads[] = {
	/* I2C1, rv4162 */
	I2C_PADS_INFO_ENTRY(I2C1, EIM_D21, 3, 21, EIM_D28, 3, 28, I2C_PAD_CTRL),
	I2C_PADS_INFO_ENTRY(I2C2, KEY_COL3, 4, 12, KEY_ROW3, 4, 13, I2C_PAD_CTRL),
	I2C_PADS_INFO_ENTRY(I2C3, GPIO_5, 1, 05, GPIO_16, 7, 11, I2C_PAD_CTRL),
};
#define I2C_BUS_CNT	3

#ifdef CONFIG_USB_EHCI_MX6
int board_ehci_hcd_init(int port)
{
	if (port) {
		/* Connect H1 USB to imx */
		gpio_direction_output(GP_USBH1_SOURCE, 1);
		mdelay(2);
	}
	return 0;
}

int board_ehci_power(int port, int on)
{
	if (port)
		return 0;
	gpio_set_value(GP_USB_OTG_PWR, on);
	return 0;
}

#endif

#ifdef CONFIG_FSL_ESDHC
struct fsl_esdhc_cfg board_usdhc_cfg[] = {
	{.esdhc_base = USDHC4_BASE_ADDR, .bus_width = 8,
			.gp_reset = GP_EMMC_RESET},
};
#endif

#ifdef CONFIG_MXC_SPI
int board_spi_cs_gpio(unsigned bus, unsigned cs)
{
	int gp = (bus == 0 && cs == 0) ? GP_ECSPI1_NOR_CS : -1;
	return gp;
}
#endif

#ifdef CONFIG_CMD_FBPANEL
void board_enable_lcd(const struct display_info_t *di, int enable)
{
	if (enable) {
		SETUP_IOMUX_PADS(rgb_pads);
		/* enable backlight PWM 1 */
		pwm_init(0, 0, 0);

		/* 300 Hz, duty cycle 2 ms, period: 3.3 ms */
		pwm_config(0, 1666667, 3333333);
		pwm_enable(0);
	} else {
		SETUP_IOMUX_PADS(rgb_gpio_pads);
	}
	gpio_direction_output(GP_LCD_DAY_BACKLIGHT_EN, enable);
	gpio_direction_output(GP_LCD_DAY_BACKLIGHT_PWM, enable);
}

static const struct display_info_t displays[] = {
	/* tsc2004 */
	VD_DC050WX(LCD, fbp_detect_i2c, 2, 0x48),
	VD_QVGA(LCD, NULL, 2, 0x48),

	/* fusion7 specific touchscreen */
	VD_FUSION7(LCD, fbp_detect_i2c, 2, 0x10),
};
#define display_cnt	ARRAY_SIZE(displays)
#else
#define displays	NULL
#define display_cnt	0
#endif


static const unsigned short gpios_out_low[] = {
	GP_USBH1_SOURCE,
	GP_USB_OTG_PWR,		/* disable USB otg power */
	GP_EMMC_RESET,		/* hold in reset */
	GP_HEATER_EN,
	GP_LCD_DAY_BACKLIGHT_EN,
	GP_LCD_DAY_BACKLIGHT_PWM,
	GP_LCD_NIGHT_BACKLIGHT_EN,
	GP_LCD_NIGHT_BACKLIGHT_PWM,
	GP_I2C3_CRTOUCH_RESET,
	GP_DA1_OUTA,
	GP_DA1_OUTB,
	GP_DB1_OUTA,
	GP_DB1_OUTB,
	GP_DA2_OUTA,
	GP_DA2_OUTB,
	GP_DB2_OUTA,
	GP_DB2_OUTB,
};

static const unsigned short gpios_out_high[] = {
	GP_ECSPI1_NOR_CS,		/* SS1 of spi nor */
	GP_I2C3_CRTOUCH_WAKE,
};

static const unsigned short gpios_in[] = {
	GP_USBH1_FP_OC,
	GP_GPIOLEDS_1,
	GP_GPIOLEDS_2,
	GP_GPIOKEYS_MENU,
	GP_GPIOKEYS_HOME,
	GP_GPIOKEYS_PB1_I,
	GP_GPIOKEYS_PB2_I,
	GP_TX,
	GP_RX,
	GP_HEATER_FAULT,
	GP_TP5,
	GP_TP6,
	GP_TP71,
	GP_TP72,
	GP_TP74,
	GP_TP75,
	GP_TP76,
	GP_TP78,
	GP_TP79,
	GP_I2C3_CRTOUCH,
};

int board_early_init_f(void)
{
	set_gpios_in(gpios_in, ARRAY_SIZE(gpios_in));
	set_gpios(gpios_out_high, ARRAY_SIZE(gpios_out_high), 1);
	set_gpios(gpios_out_low, ARRAY_SIZE(gpios_out_low), 0);
	SETUP_IOMUX_PADS(init_pads);
	SETUP_IOMUX_PADS(rgb_gpio_pads);
	return 0;
}

int board_init(void)
{
	common_board_init(i2c_pads, I2C_BUS_CNT, IOMUXC_GPR1_OTG_ID_GPIO1,
			displays, display_cnt, 0);
	return 0;
}

const struct button_key board_buttons[] = {
	{"home",	GP_GPIOKEYS_HOME,	'H', 1},
	{"menu",	GP_GPIOKEYS_MENU,	'M', 1},
	{NULL, 0, 0, 0},
};

#ifdef CONFIG_CMD_BMODE
const struct boot_mode board_boot_modes[] = {
	{"mmc0",	MAKE_CFGVAL(0x60, 0x58, 0x00, 0x00)},	/* 8-bit eMMC */
	{NULL,		0},
};
#endif
