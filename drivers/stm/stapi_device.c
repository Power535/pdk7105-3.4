/*---------------------------------------------------------------------------
* /drivers/stm/stapi_device.c
* Copyright (C) 2011 STMicroelectronics Limited
* Contributor:Francesco Virlinzi <francesco.virlinzi@st.com>
* Author:Pooja Agarwal <pooja.agarwal@st.com>
* Author:Udit Kumar <udit-dlh.kumar@st.cm>
* May be copied or modified under the terms of the GNU General Public
* License.  See linux/COPYING for more information.
*----------------------------------------------------------------------------
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/stm/platform.h>

static struct platform_device stm_wakeup_device_lirc = {
	.name =	"lirc",
	.id = -1,
};

static struct platform_device stm_wakeup_device_hdmi = {
	.name =	"hdmi",
	.id = -1,
};

static struct platform_device stm_wakeup_device_hdmi_cec = {
	.name =	"stm-hdmi-cec",
	.id = -1,
};

static struct platform_device stm_wakeup_device_hdmi_hot = {
	.name =	"stm-hdmi-hot",
	.id = -1,
};

static struct platform_device stm_wakeup_device_kscan = {
	.name =	"stm-kscan",
	.id = -1,
};

static struct platform_driver stm_wakeup_driver_lirc = {
	.driver.name = "lirc",
        .driver.owner  = THIS_MODULE,
};

static struct platform_driver stm_wakeup_driver_hdmi = {
	.driver.name = "hdmi",
        .driver.owner  = THIS_MODULE,
};

static struct platform_driver stm_wakeup_driver_hdmi_cec = {
	.driver.name = "stm-hdmi-cec",
        .driver.owner  = THIS_MODULE,
};

static struct platform_driver stm_wakeup_driver_hdmi_hot = {
	.driver.name = "stm-hdmi-hot",
        .driver.owner  = THIS_MODULE,
};

static struct platform_driver stm_wakeup_driver_kscan = {
	.driver.name = "stm-kscan",
        .driver.owner  = THIS_MODULE,
};

static int __init stm_wakeup_device_init(void){
        int err = 0;

	err = platform_driver_register(&stm_wakeup_driver_lirc);
	err |= platform_device_register(&stm_wakeup_device_lirc);
	device_set_wakeup_capable(&stm_wakeup_device_lirc.dev,1);

        err |= platform_driver_register(&stm_wakeup_driver_hdmi);
	err |= platform_device_register(&stm_wakeup_device_hdmi);
	device_set_wakeup_capable(&stm_wakeup_device_hdmi.dev,1);

	err |= platform_driver_register(&stm_wakeup_driver_hdmi_cec);
	err |= platform_device_register(&stm_wakeup_device_hdmi_cec);     
	device_set_wakeup_capable(&stm_wakeup_device_hdmi_cec.dev,1);

	err |= platform_driver_register(&stm_wakeup_driver_hdmi_hot);
	err |= platform_device_register(&stm_wakeup_device_hdmi_hot); 
	device_set_wakeup_capable(&stm_wakeup_device_hdmi_hot.dev,1);

	err |= platform_driver_register(&stm_wakeup_driver_kscan);
	err |= platform_device_register(&stm_wakeup_device_kscan); 
	device_set_wakeup_capable(&stm_wakeup_device_kscan.dev,1);	

	printk("STM_WAKEUP_DEVICE registered err %x\n",err);
        return err;
}

void __exit stm_wakeup_device_exit(void){
        printk("STM_WAKEUP_DEVICE removed \n");

	platform_driver_unregister(&stm_wakeup_driver_lirc);
	platform_device_unregister(&stm_wakeup_device_lirc);

	platform_driver_unregister(&stm_wakeup_driver_hdmi);
	platform_device_unregister(&stm_wakeup_device_hdmi);

	platform_driver_unregister(&stm_wakeup_driver_hdmi_cec);
	platform_device_unregister(&stm_wakeup_device_hdmi_cec);

	platform_driver_unregister(&stm_wakeup_driver_hdmi_hot);
	platform_device_unregister(&stm_wakeup_device_hdmi_hot);

	platform_driver_unregister(&stm_wakeup_driver_kscan);
	platform_device_unregister(&stm_wakeup_device_kscan);
}

module_init(stm_wakeup_device_init);
module_exit(stm_wakeup_device_exit);

MODULE_AUTHOR("STMicroelectronics  <www.st.com>");
MODULE_DESCRIPTION("wakeup device driver for STMicroelectronics");
MODULE_LICENSE("GPL");
