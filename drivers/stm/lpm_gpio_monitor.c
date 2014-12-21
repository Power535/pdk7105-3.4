/*------------------------------------------------------------------------
* /drivers/stm/lpm_gpio_monitor.c
* Copyright (C) 2011 STMicroelectronics Limited
* Author:Francesco Virlinzi <francesco.virlinzi@st.com>
* Author:Pooja Agrawal <pooja.agrawal@st.com>
* Author:Udit Kumar <udit-dlh.kumar@st.cm>
* May be copied or modified under the terms of the GNU General Public
* License.  See linux/COPYING for more information.
*-------------------------------------------------------------------------
*/
#ifdef CONFIG_STM_LPM_RD_MONITOR
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/bitops.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/i2c.h>
#include <linux/hom.h>
#include <linux/stm/gpio.h>
#include <linux/stm/pms.h>
#include <linux/stm/lpm.h>
#include <linux/stm/platform.h>

/************************************************************************/
/*                 monitor_gpio_handler                                 */
/* isr to check status of front panel power key                         */
/* status_gpio is exported to /sys fs as status of power key            */
/* status_gpio means power key is pressed                               */
/************************************************************************/

static irqreturn_t monitor_gpio_handler(int irq, void *ptr)
{
	struct stm_plat_lpm_data *plat_data;
	struct i2c_client *pdev = ptr;
	plat_data = i2c_get_clientdata(pdev);
	plat_data->status_gpio = 0;
	return IRQ_HANDLED;
}

/************************************************************************/
/*                 stm_lpm_show_powerkey                                */
/* export power_key to sys fs interface                                 */
/************************************************************************/

static ssize_t stm_lpm_show_powerkey(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct stm_plat_lpm_data *plat_data = dev->platform_data;
	pr_info("\n power key pressed %s\n",
	plat_data->status_gpio == 0 ? "Yes" : "No");
	ret = sprintf(buf, "%d \n", plat_data->status_gpio);
	return ret;
}
/************************************************************************/
/*                 stm_lpm_store_powerkey                               */
/* for debug purpose only                                               */
/************************************************************************/
static ssize_t stm_lpm_store_powerkey(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	unsigned long value = 0;
	pr_info("\n");
	strict_strtoul(buf, 0, &value);
	pr_info("App want to set power key to value = %d\n", (int)value);
	return count;
}

static DEVICE_ATTR(powerkey, S_IRUGO | S_IWUSR, stm_lpm_show_powerkey,
		stm_lpm_store_powerkey);

/************************************************************************/
/*                 lpm_start_power_monitor                          */
/* register with gpio ISR and configure PIO to detect power key         */
/************************************************************************/

int __init lpm_start_power_monitor(struct i2c_client *pdev)
{
	int ret = 0;
	struct stm_plat_lpm_data *plat_data;
	plat_data = i2c_get_clientdata(pdev);
	printk(KERN_INFO "Platform data gpio no %d", plat_data->number_gpio);
	/*request gpio */
	ret = gpio_request(plat_data->number_gpio, "monitor_gpio");

	if (ret < 0) {
		pr_err("ERROR: Request pin Not done!\n");
		return ret;
	}
	/*configure gpio for power key press  */
	gpio_direction_input(plat_data->number_gpio);
	set_irq_type(gpio_to_irq(plat_data->number_gpio), IRQF_TRIGGER_FALLING);

	ret = request_irq(gpio_to_irq(plat_data->number_gpio),
	monitor_gpio_handler, IRQF_DISABLED, "monitor_irq", pdev);
	if (ret < 0) {
		gpio_free(plat_data->number_gpio);
		pr_err("ERROR: Request irq Not done!\n");
		return ret;
	}
	/*at init mark power key is not pressed */
	plat_data->status_gpio = 1;
	ret = device_create_file(&(pdev->dev), &dev_attr_powerkey);
	if (ret < 0) {
		pr_err("ERROR: Not able to  create files !\n");
		goto err;

	}
	return 0;
err:
	gpio_free(plat_data->number_gpio);
	return ret;

}
/************************************************************************/
/*                 lpm_stop_power_monitor                           */
/*                 free resources                                       */
/************************************************************************/

void lpm_stop_power_monitor(struct i2c_client *pdev)
{
	struct stm_plat_lpm_data *plat_data = i2c_get_clientdata(pdev);
	device_remove_file(&(pdev->dev), &dev_attr_powerkey);
	gpio_free(plat_data->number_gpio);
}
#endif
