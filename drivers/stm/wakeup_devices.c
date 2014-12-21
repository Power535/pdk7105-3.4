/*
 * Copyright (C) 2010  STMicroelectronics
 * Author: Francesco M. Virlinzi  <francesco.virlinzi@st.com>
 *
 * May be copied or modified under the terms of the GNU General Public
 * License V.2 ONLY.  See linux/COPYING for more information.
 */

#include <linux/stm/wakeup_devices.h>
#include <linux/stm/lpm.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/export.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/export.h>
#include <linux/phy.h>

static int wokenup_by;

int stm_get_wakeup_reason(void)
{
	return wokenup_by;
}

void stm_set_wakeup_reason(int irq)
{
	wokenup_by = irq;
}

#ifdef CONFIG_STM_LPM

#include <linux/stm/lpm.h>

int stm_notify_wakeup_devices(struct stm_wakeup_devices *wkd)
{
	u16        c_wk_device = 0;

	c_wk_device |= wkd->lirc_can_wakeup ? STM_LPM_WAKEUP_IR : 0;
	c_wk_device |= wkd->hdmi_cec ? STM_LPM_WAKEUP_CEC : 0;
	c_wk_device |= wkd->kscan ? STM_LPM_WAKEUP_FRP : 0;
	c_wk_device |= wkd->stm_mac0_can_wakeup ? STM_LPM_WAKEUP_WOL : 0;
	c_wk_device |= wkd->stm_mac1_can_wakeup ? STM_LPM_WAKEUP_WOL : 0;
	c_wk_device |= wkd->asc ? STM_LPM_WAKEUP_ASC : 0;
	c_wk_device |= wkd->rtc ? STM_LPM_WAKEUP_RTC : 0;


	return stm_lpm_set_wakeup_device(c_wk_device|STM_LPM_WAKEUP_IR|STM_LPM_WAKEUP_RTC|STM_LPM_WAKEUP_WOL|STM_LPM_WAKEUP_PIO|STM_LPM_WAKEUP_EXT|STM_LPM_WAKEUP_CEC);
}

/*
 * It isn't so easy a generic code to translate
 * the bitmap coming from the SBC to a generic interrupt number.
 * It's better a generic remapping based on a chip array
 *
 * Now each platform, where the SBC is, has to declare
 * a specific platform_lpm_to_irq[] array to translate
 * the bitmap to the Linux irq numer on the chip
 */
int __initdata platform_lpm_to_irq[8] __attribute__ ((weak)) = {
	0, 0, 0, 0, 0, 0, 0, 0
};

int stm_retrieve_wakeup_reason(void)
{
        unsigned long c_wk_device = 0;

        stm_lpm_get_wakeup_device((enum stm_lpm_wakeup_devices *)&c_wk_device);

        switch(c_wk_device)
        {
                case STM_LPM_WAKEUP_IR:
                printk("\nWakeUp Device is IR");
                break;
                case STM_LPM_WAKEUP_CEC:
                printk("\nWakeUp Device is CEC");
                break;
                case STM_LPM_WAKEUP_FRP:
                printk("\nWakeUp Device is FRP");
                break;
                case STM_LPM_WAKEUP_WOL:
                printk("\nWakeUp Device is WOL");
                break;
                case STM_LPM_WAKEUP_RTC:
                printk("\nWakeUp Device is RTC");
                break;
                case STM_LPM_WAKEUP_ASC:
                printk("\nWakeUp Device is ASC");
                break;
                case STM_LPM_WAKEUP_NMI:
                printk("\nWakeUp Device is NMI");
                break;
                case STM_LPM_WAKEUP_HPD:
                printk("\nWakeUp Device is HPD");
                break;
                case STM_LPM_WAKEUP_PIO:
                printk("\nWakeUp Device is PIO");
                break;
                case STM_LPM_WAKEUP_EXT:
                printk("\nWakeUp Device is EXT");
                break;
                default:
                break;
        }

        if (!c_wk_device) {
                pr_err("ERROR: %s: Wakeup reason Not found\n"
                        "ERROR: %s: Check if the platform has a specific \n"
                        "       platform_lpm_to_irq array implementation\n",
                        __func__, __func__);
                return -1;
        }
        return platform_lpm_to_irq[ffs(c_wk_device) - 1];
}
#else
int stm_notify_wakeup_devices(struct stm_wakeup_devices *wkd)
{
	return 0;
}

int stm_retrieve_wakeup_reason(void)
{
	return 0;
}
#endif

static void stm_wake_init(struct stm_wakeup_devices *wkd)
{
	memset(wkd, 0, sizeof(*wkd));
}

static int __check_wakeup_device(struct device *dev, void *data)
{
	struct stm_wakeup_devices *wkd = (struct stm_wakeup_devices *)data;

	if (device_may_wakeup(dev)) {
		pr_info("stm pm: -> device %s can wakeup\n", dev_name(dev));
		if (strstr(dev_name(dev), "lirc"))
			wkd->lirc_can_wakeup = 1;
		else if (!strcmp(dev_name(dev), "hdmi"))
			wkd->hdmi_can_wakeup = 1;

#ifndef CONFIG_OF
		else if (!strcmp(dev_name(dev), "stmmaceth"))
			wkd->stm_mac0_can_wakeup = 1;
		else if (!strcmp(dev_name(dev), "stmmaceth.0"))
			wkd->stm_mac0_can_wakeup = 1;
		else if (!strcmp(dev_name(dev), "stmmaceth.1"))
			wkd->stm_mac1_can_wakeup = 1;
		else if (strstr(dev_name(dev), "stm-rtc-sbc"))
			wkd->rtc_sbc = 1;
#endif
		else if (!strcmp(dev_name(dev), "snps,dwmac"))
			wkd->stm_mac0_can_wakeup = 1;
		else if (!strcmp(dev_name(dev), "stmmaceth.0"))
			wkd->stm_mac0_can_wakeup = 1;
		else if (!strcmp(dev_name(dev), "snps,dwmac.0"))
			wkd->stm_mac0_can_wakeup = 1;
		else if (!strcmp(dev_name(dev), "snps,dwmac.1"))
			wkd->stm_mac1_can_wakeup = 1;
		else if (!strcmp(dev_name(dev), "stm-hdmi-cec"))
			wkd->hdmi_cec = 1;
		else if (!strcmp(dev_name(dev), "stm-hdmi-hot"))
			wkd->hdmi_hotplug = 1;
		else if (!strcmp(dev_name(dev), "stm-keyscan"))
			wkd->kscan = 1;
		else if (!strcmp(dev_name(dev), "stm-rtc"))
			wkd->rtc = 1;
		else if (strstr(dev_name(dev), "uart"))
			wkd->asc = 1;
		else if (!strcmp(dev_name(dev), "stm-rtc-sbc"))
			wkd->rtc_sbc = 1;
	}
	return 0;
}
#ifdef CONFIG_STM_LPM
static void stm_lpm_set_wakeup_device_wrapper(struct stm_wakeup_devices *wkd)
{
	unsigned int c_wk_device = 0;

	c_wk_device |= wkd->lirc_can_wakeup ? STM_LPM_WAKEUP_IR : 0;
	c_wk_device |= wkd->stm_mac0_can_wakeup ? STM_LPM_WAKEUP_WOL : 0;
	c_wk_device |= wkd->stm_mac1_can_wakeup ? STM_LPM_WAKEUP_WOL : 0;
	c_wk_device |= wkd->hdmi_cec ? STM_LPM_WAKEUP_CEC : 0;
	c_wk_device |= wkd->hdmi_hotplug ? STM_LPM_WAKEUP_HPD : 0;
	c_wk_device |= wkd->kscan ? STM_LPM_WAKEUP_FRP : 0;
	c_wk_device |= wkd->asc ? STM_LPM_WAKEUP_ASC : 0;
	c_wk_device |= wkd->rtc ? STM_LPM_WAKEUP_RTC : 0;
	c_wk_device |= wkd->rtc_sbc ? STM_LPM_WAKEUP_RTC : 0;

	stm_lpm_set_wakeup_device(c_wk_device|STM_LPM_WAKEUP_CEC|STM_LPM_WAKEUP_IR|STM_LPM_WAKEUP_HPD|STM_LPM_WAKEUP_RTC|STM_LPM_WAKEUP_PIO);
}
#else
static void stm_lpm_set_wakeup_device_wrapper(struct stm_wakeup_devices *wkd)
{
}
#endif

#ifdef CONFIG_PHYLIB
static int __check_mdio_wakeup_device(struct device *dev, void *data)
{
	struct stm_wakeup_devices *wkd = (struct stm_wakeup_devices *)data;

	if (device_may_wakeup(dev)) {
		if (!strncmp(dev_name(dev), "0:", 2))
			wkd->stm_phy_can_wakeup = 1;
		else if (!strncmp(dev_name(dev), "1:", 2))
			wkd->stm_phy_can_wakeup = 1;
	}

	return 0;
}
#endif

int stm_check_wakeup_devices(struct stm_wakeup_devices *wkd)
{
	stm_wake_init(wkd);
	bus_for_each_dev(&platform_bus_type, NULL, wkd, __check_wakeup_device);
#ifdef CONFIG_PHYLIB
	bus_for_each_dev(&mdio_bus_type, NULL, wkd, __check_mdio_wakeup_device);
#endif
	stm_lpm_set_wakeup_device_wrapper(wkd);
	return 0;
}

EXPORT_SYMBOL(stm_check_wakeup_devices);
