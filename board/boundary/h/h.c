/*
 * Copyright (C) 2010-2013 Freescale Semiconductor, Inc.
 * Copyright (C) 2013, Boundary Devices <info@boundarydevices.com>
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
#include <asm/imx-common/sata.h>
#include <asm/imx-common/spi.h>
#include <mmc.h>
#include <fsl_esdhc.h>
#include <linux/fb.h>
#include <ipu_pixfmt.h>
#include <asm/arch/crm_regs.h>
#include <i2c.h>
#include <input.h>
#include <usb/ehci-ci.h>
#include "../common/bd_common.h"
#include "../common/padctrl.h"

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |	       \
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |	       \
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |	       \
	PAD_CTL_PUS_47K_UP  | PAD_CTL_SPEED_LOW |	       \
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define RGB_PAD_CTRL	PAD_CTL_DSE_120ohm

#define SPI_PAD_CTRL (PAD_CTL_HYS |				\
	PAD_CTL_PUS_100K_DOWN | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm     | PAD_CTL_SRE_FAST)

#define I2C_PAD_CTRL	(PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm | PAD_CTL_HYS |			\
	PAD_CTL_ODE | PAD_CTL_SRE_FAST)

/*
 *
 */
static const iomux_v3_cfg_t init_pads[] = {
	/* ECSPI1 pads */
	IOMUX_PAD_CTRL(EIM_D17__ECSPI1_MISO, SPI_PAD_CTRL),
	IOMUX_PAD_CTRL(EIM_D18__ECSPI1_MOSI, SPI_PAD_CTRL),
	IOMUX_PAD_CTRL(EIM_D16__ECSPI1_SCLK, SPI_PAD_CTRL),
#define GP_ECSPI1_NOR_CS		IMX_GPIO_NR(3, 19)
	IOMUX_PAD_CTRL(EIM_D19__GPIO3_IO19, SPI_PAD_CTRL),

	/* ENET pads that don't change for PHY reset */
	IOMUX_PAD_CTRL(ENET_MDIO__ENET_MDIO, PAD_CTRL_ENET_MDIO),
	IOMUX_PAD_CTRL(ENET_MDC__ENET_MDC, PAD_CTRL_ENET_MDC),
	IOMUX_PAD_CTRL(RGMII_TXC__RGMII_TXC, PAD_CTRL_ENET_TX),
	IOMUX_PAD_CTRL(RGMII_TD0__RGMII_TD0, PAD_CTRL_ENET_TX),
	IOMUX_PAD_CTRL(RGMII_TD1__RGMII_TD1, PAD_CTRL_ENET_TX),
	IOMUX_PAD_CTRL(RGMII_TD2__RGMII_TD2, PAD_CTRL_ENET_TX),
	IOMUX_PAD_CTRL(RGMII_TD3__RGMII_TD3, PAD_CTRL_ENET_TX),
	IOMUX_PAD_CTRL(RGMII_TX_CTL__RGMII_TX_CTL, PAD_CTRL_ENET_TX),
	IOMUX_PAD_CTRL(ENET_REF_CLK__ENET_TX_CLK, PAD_CTRL_ENET_TX),
	IOMUX_PAD_CTRL(EIM_D23__GPIO3_IO23, WEAK_PULLUP),
	/* pin 42 PHY nRST */
#define GP_RGMII_PHY_RESET		IMX_GPIO_NR(1, 27)
	IOMUX_PAD_CTRL(ENET_RXD0__GPIO1_IO27, WEAK_PULLUP),

	/* i2c mux pads */
#define GP_I2C_EN_MIPI		IMX_GPIO_NR(2, 16)
	IOMUX_PAD_CTRL(EIM_A22__GPIO2_IO16, WEAK_PULLDN),	/* mipi I2C enable */
#define GP_I2C_EN_LVDS0		IMX_GPIO_NR(2, 21)
	IOMUX_PAD_CTRL(EIM_A17__GPIO2_IO21, WEAK_PULLDN),	/* LVDS0 I2C enable */
#define GP_I2C_EN_LVDS1		IMX_GPIO_NR(2, 22)
	IOMUX_PAD_CTRL(EIM_A16__GPIO2_IO22, WEAK_PULLDN),	/* LVDS1 I2C enable */
#define GP_I2C_EN_RTC		IMX_GPIO_NR(2, 23)
	IOMUX_PAD_CTRL(EIM_CS0__GPIO2_IO23, WEAK_PULLDN),	/* RTC I2C enable */
#define GP_I2C_EN_AR1020	IMX_GPIO_NR(7, 13)
	IOMUX_PAD_CTRL(GPIO_18__GPIO7_IO13, WEAK_PULLDN),	/* AR1020 I2C enable */

	/* Broadcom bcm4330 pads */
#define GP_WL_EN		IMX_GPIO_NR(6, 7)	/* NANDF_CLE - active high */
        IOMUX_PAD_CTRL(NANDF_CLE__GPIO6_IO07, OUTPUT_40OHM),	/* wlan regulator enable */
#define GP_WL_WAKE_IRQ		IMX_GPIO_NR(6, 14)	/* NANDF_CS1 - active low */
	IOMUX_PAD_CTRL(NANDF_CS1__GPIO6_IO14, WEAK_PULLDN),	/* wlan wake irq */
#define GP_WL_BT_REG_EN		IMX_GPIO_NR(6, 15)	/* NANDF_CS2 - active high */
	IOMUX_PAD_CTRL(NANDF_CS2__GPIO6_IO15, OUTPUT_40OHM),	/* bt regulator enable */
#define GP_WL_BT_WAKE_IRQ	IMX_GPIO_NR(6, 16)	/* NANDF_CS3 - active low */
	IOMUX_PAD_CTRL(NANDF_CS3__GPIO6_IO16, OUTPUT_40OHM),	/* bt wake irq */
	IOMUX_PAD_CTRL(NANDF_D2__GPIO2_IO02, OUTPUT_40OHM),		/* bt wake */
#define GP_WL_BT_RESET		IMX_GPIO_NR(6, 8)	/* NANDF_ALE - active low */
	IOMUX_PAD_CTRL(NANDF_ALE__GPIO6_IO08, OUTPUT_40OHM),	/* bt reset */
#define GP_WL_CLK_REQ_IRQ	IMX_GPIO_NR(6, 9)	/* NANDF_WP_B - active low */

#define GP_RGB_BACKLIGHT_PWM		IMX_GPIO_NR(1, 21)
	IOMUX_PAD_CTRL(SD1_DAT3__GPIO1_IO21, WEAK_PULLDN),
#define GP_LVDS0_BACKLIGHT_PWM		IMX_GPIO_NR(1, 18)
	IOMUX_PAD_CTRL(SD1_CMD__GPIO1_IO18, WEAK_PULLDN),
#define GP_LVDS1_BACKLIGHT_PWM		IMX_GPIO_NR(1, 17)
	IOMUX_PAD_CTRL(SD1_DAT1__GPIO1_IO17, WEAK_PULLDN),
#define GP_RGB_MIRROR_H			IMX_GPIO_NR(2, 25)
	IOMUX_PAD_CTRL(EIM_OE__GPIO2_IO25, WEAK_PULLUP),	/* DI0 display left/right mirror */
#define GP_RGB_MIRROR_V			IMX_GPIO_NR(2, 27)
	IOMUX_PAD_CTRL(EIM_LBA__GPIO2_IO27, WEAK_PULLUP),	/* DI0 display up/down mirror */
#define GP_LVDS0_12V_5V_BL_SELECT	IMX_GPIO_NR(4, 5)
	IOMUX_PAD_CTRL(GPIO_19__GPIO4_IO05, WEAK_PULLDN),	/* LVDS0 12v/5v select, 0 - 5v, 1 - 12v */
#define GP_RGB_LVDS1_12V_5V_BL_SELECT	IMX_GPIO_NR(1, 7)
	IOMUX_PAD_CTRL(GPIO_7__GPIO1_IO07, WEAK_PULLDN),	/* rgb/LVDS1 12v/5v select, 0 - 5v, 1 - 12v */
#define GP_12V_POWER_EN			IMX_GPIO_NR(4, 20)
	IOMUX_PAD_CTRL(DI0_PIN4__GPIO4_IO20, WEAK_PULLDN),	/* 12v power enable */

	/* UART1 */
	IOMUX_PAD_CTRL(SD3_DAT6__UART1_RX_DATA, UART_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_DAT7__UART1_TX_DATA, UART_PAD_CTRL),

	/* UART2 */
	IOMUX_PAD_CTRL(EIM_D26__UART2_TX_DATA, UART_PAD_CTRL),
	IOMUX_PAD_CTRL(EIM_D27__UART2_RX_DATA, UART_PAD_CTRL),

	/* USB */
#define GP_USB_HUB_RESET	IMX_GPIO_NR(7, 12)
	IOMUX_PAD_CTRL(GPIO_17__GPIO7_IO12, WEAK_PULLUP),	/* Hub reset */
	IOMUX_PAD_CTRL(GPIO_1__USB_OTG_ID, USDHC_PAD_CTRL), /* USBOTG ID pin */
	IOMUX_PAD_CTRL(EIM_D22__GPIO3_IO22, WEAK_PULLUP),	/* usbotg power */
	IOMUX_PAD_CTRL(KEY_COL4__USB_OTG_OC, WEAK_PULLUP),	/* USBOTG OC pin */
	IOMUX_PAD_CTRL(EIM_RW__GPIO2_IO26, WEAK_PULLUP),	/* Rev1 usb power */
	IOMUX_PAD_CTRL(EIM_D20__GPIO3_IO20, WEAK_PULLUP),	/* Rev1 usb power */
	IOMUX_PAD_CTRL(EIM_A25__GPIO5_IO02, WEAK_PULLUP),	/* Rev1 usb power */

	/* USDHC3 - sdcard */
	IOMUX_PAD_CTRL(SD3_CLK__SD3_CLK, USDHC_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_CMD__SD3_CMD, USDHC_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_DAT0__SD3_DATA0, USDHC_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_DAT1__SD3_DATA1, USDHC_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_DAT2__SD3_DATA2, USDHC_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_DAT3__SD3_DATA3, USDHC_PAD_CTRL),
#define GP_USDHC3_CD		IMX_GPIO_NR(7, 0)
	IOMUX_PAD_CTRL(SD3_DAT5__GPIO7_IO00, WEAK_PULLUP), /* CD */

	/* USDHC4  */
	IOMUX_PAD_CTRL(SD4_CLK__SD4_CLK, USDHC_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_CMD__SD4_CMD, USDHC_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_DAT0__SD4_DATA0, USDHC_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_DAT1__SD4_DATA1, USDHC_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_DAT2__SD4_DATA2, USDHC_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_DAT3__SD4_DATA3, USDHC_PAD_CTRL),
#define GP_USDHC4_CD		IMX_GPIO_NR(2, 6)
	IOMUX_PAD_CTRL(NANDF_D6__GPIO2_IO06, WEAK_PULLUP), /* CD */
};

#ifdef CONFIG_CMD_FBPANEL
static const iomux_v3_cfg_t rgb_pads[] = {
	IOMUX_PAD_CTRL(DI0_DISP_CLK__IPU1_DI0_DISP_CLK, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DI0_PIN15__IPU1_DI0_PIN15, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DI0_PIN2__IPU1_DI0_PIN02, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DI0_PIN3__IPU1_DI0_PIN03, RGB_PAD_CTRL),
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
};
#endif

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
};

static const struct i2c_pads_info i2c_pads[] = {
	/* I2C1, SGTL5000 */
	I2C_PADS_INFO_ENTRY(I2C1, EIM_D21, 3, 21, EIM_D28, 3, 28, I2C_PAD_CTRL),
	/* I2C3, J15 - RGB connector */
	I2C_PADS_INFO_ENTRY(I2C3, GPIO_5, 1, 05, GPIO_16, 7, 11, I2C_PAD_CTRL),
};
#define I2C_BUS_CNT	2

#ifdef CONFIG_USB_EHCI_MX6
int board_ehci_hcd_init(int port)
{
	if (port) {
		/* Reset USB hub */
		gpio_direction_output(GP_USB_HUB_RESET, 0);
		mdelay(2);
		gpio_set_value(GP_USB_HUB_RESET, 1);
	}
	return 0;
}
#endif

#ifdef CONFIG_FSL_ESDHC
struct fsl_esdhc_cfg board_usdhc_cfg[] = {
	{.esdhc_base = USDHC3_BASE_ADDR, .bus_width = 4,
			.gp_cd = GP_USDHC3_CD},
	{.esdhc_base = USDHC4_BASE_ADDR, .bus_width = 8,
			.gp_cd = GP_USDHC4_CD},
};
#endif

#ifdef CONFIG_MXC_SPI
int board_spi_cs_gpio(unsigned bus, unsigned cs)
{
	return (bus == 0 && cs == 0) ? GP_ECSPI1_NOR_CS : -1;
}
#endif

#ifdef CONFIG_CMD_FBPANEL
void board_enable_lcd(const struct display_info_t *di, int enable)
{
	if (enable)
		SETUP_IOMUX_PADS(rgb_pads);
	else
		SETUP_IOMUX_PADS(rgb_gpio_pads);
	gpio_direction_output(GP_RGB_BACKLIGHT_PWM, enable);
}

void board_enable_lvds(const struct display_info_t *di, int enable)
{
	gpio_direction_output(GP_LVDS0_BACKLIGHT_PWM, enable);
	gpio_direction_output(GP_LVDS1_BACKLIGHT_PWM, enable ^ 1);
}

void board_enable_lvds2(const struct display_info_t *di, int enable)
{
	gpio_direction_output(GP_LVDS0_BACKLIGHT_PWM, enable ^ 1);
	gpio_direction_output(GP_LVDS1_BACKLIGHT_PWM, enable);
}

static const struct display_info_t displays[] = {
	VD_1024_600(LVDS, fbp_detect_i2c, 2, 0x04),
	VD_INNOLUX_WVGA(LVDS2, fbp_detect_i2c, 2, 0x48),
};
#define display_cnt	ARRAY_SIZE(displays)
#else
#define displays	NULL
#define display_cnt	0
#endif

static const unsigned short gpios_out_low[] = {
	GP_RGMII_PHY_RESET,
	/* Disable wl1271 */
	GP_WL_EN,
	GP_WL_BT_REG_EN,
	GP_WL_BT_RESET,
	GP_RGB_MIRROR_V,
	GP_LVDS0_12V_5V_BL_SELECT,
	GP_RGB_LVDS1_12V_5V_BL_SELECT,
	GP_12V_POWER_EN,
	GP_I2C_EN_MIPI,
	GP_I2C_EN_LVDS0,
	GP_I2C_EN_LVDS1,
	GP_I2C_EN_RTC,
	GP_I2C_EN_AR1020,
};

static const unsigned short gpios_out_high[] = {
	GP_ECSPI1_NOR_CS,
	GP_RGB_MIRROR_H,
};

static const unsigned short gpios_in[] = {
	GP_WL_WAKE_IRQ,
	GP_WL_BT_WAKE_IRQ,
	GP_WL_CLK_REQ_IRQ,
	GP_RGB_BACKLIGHT_PWM,
	GP_LVDS0_BACKLIGHT_PWM,
	GP_LVDS1_BACKLIGHT_PWM,
	GP_USDHC3_CD,
	GP_USDHC4_CD,
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
	{NULL, 0, 0, 0},
};

#ifdef CONFIG_CMD_BMODE
const struct boot_mode board_boot_modes[] = {
	/* 4 bit bus width */
	{"mmc0",	MAKE_CFGVAL(0x40, 0x30, 0x00, 0x00)},
	{"mmc1",	MAKE_CFGVAL(0x40, 0x38, 0x00, 0x00)},
	{NULL,		0},
};
#endif
