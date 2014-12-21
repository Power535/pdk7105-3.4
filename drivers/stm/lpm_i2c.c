/*---------------------------------------------------------------------------
* /drivers/stm/lpm_i2c.c
* Copyright (C) 2011 STMicroelectronics Limited
* Contributor:Francesco Virlinzi <francesco.virlinzi@st.com>
* Author:Pooja Agarwal <pooja.agarwal@st.com>
* Author:Udit Kumar <udit-dlh.kumar@st.cm>
* May be copied or modified under the terms of the GNU General Public
* License.  See linux/COPYING for more information.
*----------------------------------------------------------------------------
*/

#include <linux/stm/lpm.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/errno.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/stm/platform.h>
#include <linux/stm/stx7108.h>
#include "lpm_def.h"

#ifdef CONFIG_STM_LPM_DEBUG
#define lpm_debug(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define lpm_debug(fmt, ...)
#endif

struct stm_lpm_driver_data{
	struct i2c_adapter *i2c_sbc_adapater;
	u8 gbltrans_id;
	char fw_major_ver;
	struct mutex msg_protection_mutex;
};


static struct stm_lpm_driver_data *lpm_drv;

static const struct i2c_device_id lpm_ids[] = {
	{ "stm-lpm", 0 },
	{ /* END OF LIST */ }
};
MODULE_DEVICE_TABLE(i2c, lpm_ids);

/************************************************************************/
/* Purpose : To set ir key setup                                        */
/*STM8 firmware does not support this                                   */
/************************************************************************/
int stm_lpm_setup_ir(u8 num_keys, struct stm_lpm_ir_keyinfo *ir_key_info)
{
	return -EPERM;
}
EXPORT_SYMBOL(stm_lpm_setup_ir);

/************************************************************************/
/* Purpose : To get wakeup data                                         */
/*STM8 firmware does not support this                                   */
/************************************************************************/
int stm_lpm_get_wakeup_info(enum stm_lpm_wakeup_devices *wakeupdevice,
	int *validsize, int datasize, char *data)
{
	return -EPERM;
}
EXPORT_SYMBOL(stm_lpm_get_wakeup_info);

/************************************************************************/
/* Purpose : To get reset part of SOC                                   */
/* STM8 firmwar does not support this                                   */
/************************************************************************/

int stm_lpm_reset(enum stm_lpm_reset_type reset_type)
{
	return -EPERM;
}
EXPORT_SYMBOL(stm_lpm_reset);

/************************************************************************/
/* lpm_recv_respone_i2c                                                 */
/* to get response from STM8 SBC, this is connected on i2c              */
/************************************************************************/

static int lpm_recv_respone_i2c(struct lpm_message *msg)
{
	struct i2c_msg i2c_message = {
	.addr = ADDRESS_OF_EXT_MC,
	/*Device address of slave*/
	.flags = I2C_M_RD,
	.buf = (char *)msg,
	.len = 2};
	int err = 1, i = 0;
	lpm_debug("%s msg size 4 addr is %d \n " , __func__,  i2c_message.addr);

	/*fill message expected length */
	switch (msg->command_id) {
	default:
	lpm_debug("response not need \n");
	return err;

	/*Use default length of 2*/
	case LPM_MSG_SET_TIMER:
	case LPM_MSG_SET_IR:
	case LPM_MSG_SET_FP:
	case LPM_MSG_SET_WDT:
	case LPM_MSG_SET_RTC:
	case LPM_MSG_SET_WUD:
	break;

	case LPM_MSG_VER:
		i2c_message.len = 7;
		break;
	case LPM_MSG_READ_RTC:
		i2c_message.len = 8;
		break;
	case LPM_MSG_GET_STATUS:
	case LPM_MSG_GET_WUD:
		i2c_message.len = 4;
		break;
	}
	/*read stm8 response */
	err = i2c_transfer(lpm_drv->i2c_sbc_adapater, &i2c_message, 1);
	lpm_debug("i2c_transfer response  is %d \n", err);
	if (err >= 0) {
		lpm_debug("%s cmd id %d tran id%d \n", __func__,
		msg->command_id, msg->transaction_id);
		for (i = 0; i < i2c_message.len-2; i++)
			lpm_debug("%X ", msg->msg_data[i]);
		lpm_debug("\n");
	}
	return err;
}

/************************************************************************/
/* lpm_send_msg                                                         */
/* send command to STM8 SBC                                             */
/************************************************************************/

static int lpm_send_msg(char *msg, unsigned char msg_size,
	struct lpm_message *response) {
	int err = 0;
	struct i2c_msg i2c_message = {.addr = ADDRESS_OF_EXT_MC,
		/*Device address of slave*/
		.flags = 0,
		.buf = (char *) msg,
		.len = msg_size+2};
	lpm_debug("stm_lpm_send_msg, size of msg %d \n", msg_size + 2);
	err = i2c_transfer(lpm_drv->i2c_sbc_adapater, &i2c_message, 1);
	lpm_debug("i2c_transfer send  is %d \n", err);
	if (err >= 0)
		/*Get the reply from external controller*/
		err = lpm_recv_respone_i2c(response);
	return err;
}

/************************************************************************/
/*                 lpm_exchange_msg                                     */
/* to send/received message with STM8 CPU                               */
/************************************************************************/

int lpm_exchange_msg(struct lpm_internal_send_msg *send_msg,
struct lpm_message *response){
	char  *lpm_msg = NULL;
	struct lpm_message msg = {0};
	int count = 0, err = 0;

	lpm_debug("lpm_exchange_msg \n");
	if (send_msg->msg_size > 14) {
		/*We would enter into this condition very rarely*/
		lpm_msg = kmalloc(send_msg->msg_size + 2, GFP_KERNEL);
		if (!lpm_msg)
			return -ENOMEM;
	} else {
		lpm_msg = (char *)&msg;
	}

	mutex_lock(&lpm_drv->msg_protection_mutex);
	lpm_msg[0] = send_msg->command_id;
	response->command_id = send_msg->command_id;
	lpm_msg[1] = lpm_drv->gbltrans_id++;

	if (send_msg->msg_size)
		memcpy(&lpm_msg[2], send_msg->msg, send_msg->msg_size);

	lpm_debug("Sending msg {%d, %d ", lpm_msg[0], lpm_msg[1]);
	for (count = 0; count < send_msg->msg_size; count++)
		lpm_debug(" %d", lpm_msg[2+count]);
	lpm_debug(" } \n");

	err = lpm_send_msg(lpm_msg, send_msg->msg_size, response);
	if (err <= 0) {
		lpm_debug("f/w is not responding \n");
		err = -EAGAIN;
		goto exit;
	}
	if (send_msg->reply_type == SBC_REPLY_NO)
		goto exit;
	lpm_debug("recd reply  %d {%d, %d ", err, response->command_id,
	response->transaction_id);

	for (count = 0; count < 14; count++)
		lpm_debug(" %d", response->msg_data[count]);
	lpm_debug("} \n");

	if (lpm_msg[1] == response->transaction_id) {
		/* Check for error in FWLPM response*/
		if (response->command_id == LPM_MSG_ERR) {
			/*just check error for command*/
			printk(KERN_ALERT "Error respone \n");
			/* Get the actual error */
			/*Fix me : Actual error still to be decided by SBC */
			/* When available convert SBC error into kernel world */
			/* err = response->msg_data[3]; */
			/* convert err into linux work */
			/* till then EREMOTEIO is ok to indicate remote
			devide has not responded well  */
			err = -EREMOTEIO;
		}
	} else {
		/* In case of i2c no correct tran id is expected */
		lpm_debug("Received response tran ID");
		lpm_debug(" %d", response->transaction_id);
		lpm_debug("for %d \n", lpm_msg[1]);
		err = -EREMOTEIO; /* invalid trans id*/
	}

exit:
	mutex_unlock(&lpm_drv->msg_protection_mutex);

	/*If we have allocated lpm_msg then free this message*/
	if (send_msg->msg_size > 14)
		kfree(lpm_msg);
	return err;
}
/************************************************************************/
/*                 lpm_enter_passive_standby                        */
/* inform STM8 to get ready for controller passive standby              */
/* Actual power off will be generated by Linux PM by i2c voilatio       */
/************************************************************************/
static int lpm_enter_passive_standby(void){
	struct lpm_internal_send_msg send_msg;
	struct lpm_message response;
	LPM_FILL_MSG(send_msg, LPM_MSG_ENTER_PASSIVE, NULL, MSG_ZERO_SIZE);
	LPM_FILL_ID_REPLY(send_msg, MSG_ID_AUTO, SBC_REPLY_NO);
	return lpm_exchange_msg(&send_msg, &response);
}

/************************************************************************/
/*  SBC firmware major protocol version number                          */
/************************************************************************/
char lpm_fw_proto_version(void)
{
	return lpm_drv->fw_major_ver;
}

/************************************************************************/
/*                 stm_lpm_probe                                        */
/* to get i2c resources connected with STM8 SBC CPU                     */
/************************************************************************/
static int __init stm_lpm_probe(struct i2c_client *pdev, const struct i2c_device_id *id)
{
	struct stm_plat_lpm_data *plat_data;
	int err = 0;
	struct stm_lpm_version driver_ver, fw_ver;
	lpm_debug("stm lpm probe \n");
	/*Allocate data structure*/
	lpm_drv = kzalloc(sizeof(struct stm_lpm_driver_data), GFP_KERNEL);
	if (unlikely(lpm_drv == NULL)) {
		printk(KERN_ERR "%s: Request memory not done\n", __func__);
		return -ENOMEM;
	}
	plat_data = i2c_get_clientdata(pdev);
	if (unlikely(plat_data == NULL)) {
		dev_err(&pdev->dev, "No platform data\n");
		err = -ENOENT;
		goto exit;
	}

	/*get i2c resource */
	lpm_debug("i2c_id %d \n", plat_data->number_i2c);
	if (plat_data->number_i2c) {
		lpm_drv->i2c_sbc_adapater = plat_data->i2c_adap;
		/*i2c_get_adapter(plat_data->number_i2c); */
		if (lpm_drv->i2c_sbc_adapater == NULL) {
			printk(KERN_ERR "%s: i2c adapter not", __func__);
			printk(KERN_ERR "%d found \n", plat_data->number_i2c);
			err = -ENODEV;
			goto exit;
		}
		lpm_debug("stlpm i2c adapter found at %d i2c is %x \n",
		plat_data->number_i2c, (unsigned int)lpm_drv->i2c_sbc_adapater);
	}
	/*mark parent */
	pdev->dev.parent = &lpm_drv->i2c_sbc_adapater->dev;
	/*mutex initialization*/
	mutex_init(&lpm_drv->msg_protection_mutex);
	err = stm_lpm_get_version(&driver_ver, &fw_ver);
	if (err < 0) {
		dev_err(&pdev->dev, "No STM8 firmware available \n");
		goto exit;
	}
	err = 0;
	lpm_drv->fw_major_ver = fw_ver.major_comm_protocol;
#ifdef CONFIG_STM_LPM_RD_MONITOR
	/* mointer front panel power key */
	/* and export status of power key on FP using status_gpio*/
	err = lpm_start_power_monitor(pdev);
#endif
	return err;
exit:
	kfree(lpm_drv);
	return err;
}

/************************************************************************/
/*                 stm_lpm_remove                                       */
/* free sources used in STM8 communication                              */
/************************************************************************/
static int stm_lpm_remove(struct i2c_client *client)
{
	struct stm_plat_lpm_data *plat_data = i2c_get_clientdata(client);
	lpm_debug("stm_lpm_remove \n");
	i2c_put_adapter(plat_data->i2c_adap);
#ifdef CONFIG_STM_LPM_RD_MONITOR
	lpm_stop_power_monitor(client);
#endif
	kfree(lpm_drv);
	return 0;
}

/************************************************************************/
/*                 stm_lpm_freeze                                       */
/************************************************************************/
static int stm_lpm_freeze(struct i2c_client *dev)
{
	lpm_debug("stm_lpm_freeze state \n");
	lpm_enter_passive_standby();
	return 0;
}

/************************************************************************/
/*                 stm_lpm_restore                                      */
/************************************************************************/
static int stm_lpm_restore(struct i2c_client *dev)
{
	struct stm_plat_lpm_data *plat_data;
	lpm_debug("stm_lpm_restore \n");
	plat_data = i2c_get_clientdata(dev);
	/*We return from CPS, mark power key as not pressed */
	plat_data->status_gpio = 1;
	return 0;
}


static struct i2c_driver stm_lpm_driver = {
	.driver.name = "stm-lpm",
	.driver.owner  = THIS_MODULE,
	.probe = stm_lpm_probe,
	.remove = stm_lpm_remove,
	.freeze =  stm_lpm_freeze,
	.restore = stm_lpm_restore,
	.id_table = lpm_ids,
};

static int __init stm_lpm_init(void){
	int err = -ENXIO;
	struct stm_plat_lpm_data *plat_data;
	struct i2c_client *client;
	/* get 7108 specific i2c data */
	struct i2c_board_info *info = stx7108_get_lpm_i2c();
	plat_data = info->platform_data;
	/* get adapter on which i2c stm8 is connected */
	plat_data->i2c_adap = i2c_get_adapter(plat_data->number_i2c);
	if (plat_data->i2c_adap == NULL)
		goto exit;
	/* add new device with above adapter */
	client = i2c_new_device(plat_data->i2c_adap, stx7108_get_lpm_i2c());
	if (client == NULL)
		goto exit;
	/*set plat_data as client i2c data */
	i2c_set_clientdata(client , plat_data);

	/* register driver on above device */
	err = i2c_add_driver(&stm_lpm_driver);
	pr_info("stm_lpm err %d \n", err);
	pr_info("STM_LPM driver registered\n");
exit:
	return err;
}

void __exit stm_lpm_exit(void){
	pr_info("STM_LPM driver removed \n");
	i2c_del_driver(&stm_lpm_driver);
}

module_init(stm_lpm_init);
module_exit(stm_lpm_exit);

MODULE_AUTHOR("STMicroelectronics  <www.st.com>");
MODULE_DESCRIPTION("lpm device driver for STMicroelectronics devices");
MODULE_LICENSE("GPL");
