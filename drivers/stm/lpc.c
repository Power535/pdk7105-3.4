/* --------------------------------------------------------------------
 * <root>/drivers/stm/lpc.c
 * --------------------------------------------------------------------
 *  Copyright (C) 2009 : STMicroelectronics
 *  Copyright (C) 2010 : STMicroelectronics
 *  Copyright (C) 2011 : STMicroelectronics
 *  Author: Francesco M. Virlinzi <francesco.virlinzi@st.com>
 *
 * May be copied or modified under the terms of the GNU General Public
 * License version 2.0 ONLY.  See linux/COPYING for more information.
 *
 */

#include <linux/stm/platform.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <asm/clock.h>


#ifdef CONFIG_RTC_DRV_STM_LPC
#error There is a conflict in the rtc-lpc device driver
#endif

#ifdef CONFIG_STM_LPC_DEBUG
#define dgb_print(fmt, args...)	pr_info("%s: " fmt, __func__ , ## args)
#else
#define dgb_print(fmt, args...)
#endif

/*
 * LPC Workaround on 7111:
 * The LPC when enabled can not be stopped
 * To reset in any case the LPC when the system is resume
 * we route the irw_wake_irq and the eth_wake_irq to the
 * on the __'ilc3_wakeup_out' signal__ able to reset
 * the LPC.
 * In this manner any wakeup interrupt will reset the LPC
 * and will gave us the reprogramming capability.
 *
 * LPC Workaround on 7141:
 * On 7141 the LPC can work as watchdog.
 * On this platform to reset the LPC a WatchDog_reset
 * is requested able to perform a LPC reset but without
 * any real WDT Reset signal able to hangs the SOC.
 */


#define LPA_LS		0x410
#define LPA_MS		0x414
#define LPA_START	0x418
#define LPA_WDT		0x510	/* stx7141 */

#define DEFAULT_LPC_FREQ	46875	/* Hz */

struct stm_lpc {
	unsigned long base;
	struct platform_device *pdev;
	unsigned long long timeout;
	unsigned long state;
	int irq;
	struct clk *clk;
};

static struct stm_lpc lpc;

#define lpc_store32(lpc, offset, value)	iowrite32(value, (lpc)->base + offset)
#define lpc_load32(lpc, offset)		ioread32((lpc)->base + offset)

#define lpc_set_enabled(l)	device_set_wakeup_enable(&(l)->pdev->dev, 1)
#define lpc_set_disabled(l)	device_set_wakeup_enable(&(l)->pdev->dev, 0)

#define lpc_set_timeout(l, t)		(l)->timeout = (t)
#define lpc_read_timeout(l)		((l)->timeout)
#define lpc_is_enabled(l)		((l)->state & LPC_STATE_ENABLED)

void stm_lpc_write(unsigned long long counter)
{
	dgb_print("\n");
	lpc_set_timeout(&lpc, counter);
}
EXPORT_SYMBOL(stm_lpc_write);

unsigned long long stm_lpc_read(void)
{
	return lpc_read_timeout(&lpc);
}
EXPORT_SYMBOL(stm_lpc_read);

int stm_lpc_set(int enable, unsigned long long tick)
{
	dgb_print("\n");
	if (enable) {
		lpc_set_timeout(&lpc, tick);
		lpc_set_enabled(&lpc);
	} else
		lpc_set_disabled(&lpc);
	return 0;
}
EXPORT_SYMBOL(stm_lpc_set);

static irqreturn_t stm_lpc_irq(int this_irq, void *data)
{
	dgb_print("Interrupt from LPC\n");
	lpc_store32(&lpc, LPA_START, 0);
	return IRQ_HANDLED;
}

static int __init stm_lpc_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct stm_plat_rtc_lpc *data = (struct stm_plat_rtc_lpc *)
		pdev->dev.platform_data;

	dgb_print("\n");
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;
	if (data->no_hw_req)
		goto no_hw_request;
	if (!devm_request_mem_region(&pdev->dev, res->start,
		res->end - res->start, "stm_lpc")){
		printk(KERN_ERR "%s: Request mem 0x%x region not done\n",
			__func__, res->start);
		return -ENOMEM;
	}

	if (!(lpc.base =
		devm_ioremap_nocache(&pdev->dev, res->start,
				(int)(res->end - res->start)))) {
		printk(KERN_ERR "%s: Request iomem 0x%x region not done\n",
			__func__, (unsigned int)res->start);
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		printk(KERN_ERR "%s Request irq %d not done\n",
			__func__, res->start);
		return -ENODEV;
	}

	if (res->start == -1)
		goto no_irq;

	set_irq_type(res->start, data->irq_edge_level);
	enable_irq_wake(res->start);
	if (devm_request_irq(&pdev->dev, res->start, stm_lpc_irq,
		IRQF_DISABLED, "stm_lpc", &lpc) < 0){
		printk(KERN_ERR "%s: Request irq not done\n", __func__);
		return -ENODEV;
	}

no_hw_request:
	if (data->clk_id) {
		dgb_print("Looking for clk: %s\n", data->clk_id);
		lpc.clk = clk_get(NULL, data->clk_id);
		if (lpc.clk)
			dgb_print("Using clock %s @ %u Hz\n",
				lpc.clk->name, clk_get_rate(lpc.clk));
		clk_enable(lpc.clk);
	}

	device_set_wakeup_capable(&pdev->dev, 1);
no_irq:
	lpc.irq = res->start;
	lpc.pdev = pdev;

	return 0;
}

static int stm_lpc_remove(struct platform_device *pdev)
{
	struct resource *res;
	dgb_print("\n");
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	devm_free_irq(&pdev->dev, res->start, NULL);
	devm_iounmap(&pdev->dev, (void *)lpc.base);
	return 0;
}

#ifdef CONFIG_PM
static int stm_lpc_suspend(struct device *dev)
{
	struct stm_plat_rtc_lpc *pdata = (struct stm_plat_rtc_lpc *)
		dev->platform_data;
	union {
		unsigned long long value;
		unsigned long parts[2];
	} tmp;

	dgb_print("\n");
	if (!device_may_wakeup(dev) || !lpc_read_timeout(&lpc))
		return  0;

	tmp.value = lpc.timeout * clk_get_rate(lpc.clk);

 	writel(tmp.parts[1], lpc.base + LPA_MS);
 	writel(tmp.parts[0], lpc.base + LPA_LS);
 	writel(1, lpc.base + LPA_START);
#if 0
	if (pdata->need_wdt_reset)
		writel(1, lpc.base + LPA_WDT);
 	writel(1, lpc.base + LPA_START);
	if (pdata->need_wdt_start)		/* 7108 */
		writel(1, lpc.base + LPA_WDT);
	else if (pdata->need_wdt_reset)		/* 7105 */
		writel(1, lpc.base + LPA_WDT);
#endif
	return 0;
}

static int stm_lpc_resume(struct device *dev)
{
	struct stm_plat_rtc_lpc *pdata = (struct stm_plat_rtc_lpc *)
		dev->platform_data;
/*
 * Reset the 'enable' and the 'timeout' to be
 * compliant with the hardware reset
 */
	dgb_print("\n");
	if (device_may_wakeup(dev)) {
		/* turn-off the timer */
		if (pdata->need_wdt_reset) {
			writel(0, lpc.base + LPA_MS);
			writel(0, lpc.base + LPA_LS);
			writel(1, lpc.base + LPA_WDT);
			writel(1, lpc.base + LPA_START);
			writel(0, lpc.base + LPA_WDT);
		} else {
			writel(0, lpc.base + LPA_START);
			writel(1, lpc.base + LPA_MS);
			writel(1, lpc.base + LPA_LS);
		}
	}

	lpc_set_disabled(&lpc);
	lpc_set_timeout(&lpc, 0);
	return 0;
}

static struct dev_pm_ops lpc_pm_ops = {
	.suspend = stm_lpc_suspend,
	.resume = stm_lpc_resume,
};

#else
static struct dev_pm_ops lpc_pm_ops;
#endif

static struct platform_driver stm_lpc_driver = {
	.driver.name = "stm-rtc",
	.driver.owner = THIS_MODULE,
	.driver.pm = &lpc_pm_ops,
	.probe = stm_lpc_probe,
	.remove = stm_lpc_remove,
};

static int __init stm_lpc_init(void)
{
	dgb_print("\n");
	platform_driver_register(&stm_lpc_driver);
	printk(KERN_INFO "stm_lpc device driver registered\n");
	return 0;
}

static void __exit stm_lpc_exit(void)
{
	dgb_print("\n");
	platform_driver_unregister(&stm_lpc_driver);
}

module_init(stm_lpc_init);
module_exit(stm_lpc_exit);

MODULE_AUTHOR("STMicroelectronics  <www.st.com>");
MODULE_DESCRIPTION("LPC device driver for STMicroelectronics devices");
MODULE_LICENSE("GPL");





