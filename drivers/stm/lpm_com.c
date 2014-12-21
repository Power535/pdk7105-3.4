/*
 * <root>/drivers/stm/lpm_com.c
 *
 * This driver implements communication with Standby Controller
 * in some STMicroelectronics devices
 *
 * Copyright (C) 2012 STMicroelectronics Limited
 *
 * Contributor:Francesco Virlinzi <francesco.virlinzi@st.com>
 * Author:Pooja Agarwal <pooja.agarwal@st.com>
 * Author:Udit Kumar <udit-dlh.kumar@st.com>
 *
 * May be copied or modified under the terms of the GNU General Public License.
 * See linux/COPYING for more information.
 */


#include <linux/module.h>
#include <linux/stm/lpm.h>
#include <asm/unaligned.h>
#include "lpm_def.h"
#include <linux/module.h>

/**
 * stm_lpm_get_version() - To get version of driver and firmware
 * @driver_version:	driver version
 * @fw_version:	firmware version
 *
 * This function will return firmware and driver version in parameters.
 *
 * Return - 0 on success
 * Return -  negative error code on failure.
 */

int stm_lpm_get_version(struct stm_lpm_version *driver_version,
	struct stm_lpm_version *fw_version)
{
	int err = 0;
	struct lpm_message response = {0};
	struct lpm_internal_send_msg send_msg = {
		.command_id = LPM_MSG_VER,
		.msg_size = 0
	};
	/* Check parameters */
	if (unlikely((driver_version == NULL) || (fw_version == NULL)))
		return -EINVAL;

	/* Send message to SBC and get response */
	err = lpm_exchange_msg(&send_msg, &response);
	if (likely(err == 0)) {
		/* Copy the firmware version in paramater */
		fw_version->major_comm_protocol = response.msg_data[0] >> 4;
		fw_version->minor_comm_protocol = response.msg_data[0] & 0x0F;
		fw_version->major_soft = response.msg_data[1] >> 4;
		fw_version->minor_soft = response.msg_data[1] & 0x0F;
		fw_version->patch_soft = response.msg_data[2] >> 4;
		fw_version->month = response.msg_data[2] & 0x0F;
		memcpy(&fw_version->day, &response.msg_data[3], 3);

		driver_version->major_comm_protocol = LPM_MAJOR_PROTO_VER;
		driver_version->minor_comm_protocol = LPM_MINOR_PROTO_VER;
		driver_version->major_soft = LPM_MAJOR_SOFT_VER;
		driver_version->minor_soft = LPM_MINOR_SOFT_VER;
		driver_version->patch_soft = LPM_PATCH_SOFT_VER;
		driver_version->month = LPM_BUILD_MONTH;
		driver_version->day = LPM_BUILD_DAY;
		driver_version->year = LPM_BUILD_YEAR;
	}
	return err;
}
EXPORT_SYMBOL(stm_lpm_get_version);

/**
 * stm_lpm_configure_wdt - Set watchdog timeout for Standby Controller
 * @time_in_ms:	timeout in milli second
 *
 * Return - 0 on success
 * Return - negative error code on failure.
 */

int stm_lpm_configure_wdt(u16 time_in_ms, u8 wdt_type)
{
	char msg[3];
	struct lpm_internal_send_msg send_msg = {
		.command_id = LPM_MSG_SET_WDT,
		.msg = msg,
		.msg_size = 3
	};
	struct lpm_message response;
	if (time_in_ms < 0)
		return -EINVAL;
	put_unaligned_le16(time_in_ms, msg);
        msg[2] = wdt_type;
	return lpm_exchange_msg(&send_msg, &response);
}
EXPORT_SYMBOL(stm_lpm_configure_wdt);

/**
 * stm_lpm_get_fw_state - Get the SBC firmware status
 * @fw_state:	enum for firmware status
 *
 * Firmware status will be returned in passed parameter.
 *
 * Return - 0 on success
 * Return - negative  error code on failure.
 */

int stm_lpm_get_fw_state(enum stm_lpm_sbc_state *fw_state)
{
	int err = 0;
	struct lpm_internal_send_msg send_msg = {
		.command_id = LPM_MSG_GET_STATUS,
		.msg_size = 0
	};
	struct lpm_message reply_msg = {0};
	if (fw_state == NULL)
		return -EINVAL;
	err = lpm_exchange_msg(&send_msg, &reply_msg);
	if (likely(err == 0))
		*fw_state = reply_msg.msg_data[0];
	return err;
}
EXPORT_SYMBOL(stm_lpm_get_fw_state);

/**
 * stm_lpm_set_wakeup_device - To set wakeup devices
 * @devices:	All enabled wakeup devices
 *
 * In older protocol version wakeup devices were limited to 8,
 * whereas new protocol version supports upto 10 wakeup devices.
 * Therefore two different message were used to set wakeup devices,
 * driver checks firmware version and send wakeup device accordingly.
 *
 * Return - 0 on success
 * Return - negative error code on failure.
 */

int stm_lpm_set_wakeup_device(u16 devices)
{
	if (lpm_fw_proto_version() >= 1) {
		struct stm_lpm_adv_feature feature;
		feature.feature_name = STM_LPM_WU_TRIGGERS;
	devices |= 3 << 8;
		put_unaligned_le16(devices, feature.params.set_params);
		return stm_lpm_set_adv_feature(1, &feature);
	} else {
		char msg = devices & 0xFF;
		struct lpm_message response;
		struct lpm_internal_send_msg send_msg = {
			.command_id = LPM_MSG_SET_WUD,
			.msg = &msg,
			.msg_size = 1
		};
		/* In old protocol version max wakeup devices can be eight */
		return lpm_exchange_msg(&send_msg, &response);
	}
}
EXPORT_SYMBOL(stm_lpm_set_wakeup_device);

/**
 * stm_lpm_set_wakeup_time - To set wakeup time
 * @timeout:	Timeout in seconds after which wakeup is required
 *
 * Wakeup will be done after current time + timeout
 *
 * Return - 0 on success
 * Return - negative error code on failure.
 */

int stm_lpm_set_wakeup_time(u32 timeout)
{
	char msg[4];
	struct lpm_message response;
	struct lpm_internal_send_msg send_msg = {
		.command_id = LPM_MSG_SET_TIMER,
		.msg = msg,
		.msg_size = 4
	};
	timeout = cpu_to_le32(timeout);
	/* Copy timeout into message */
	memcpy(msg, &timeout, 4);
	return lpm_exchange_msg(&send_msg, &response);
}
EXPORT_SYMBOL(stm_lpm_set_wakeup_time);

/**
 * stm_lpm_set_rtc - To set rtc time for standby controller
 * @new_rtc:	rtc value
 *
 * SBC can display RTC clock when in standby mode using this RTC value,
 * This RTC will act as base for RTC hardware of SBC.
 *
 * Return - 0 on success
 * Return - negative error code on failure.
 */
int stm_lpm_set_rtc(struct rtc_time *new_rtc)
{
	char msg[6];
	struct lpm_message response = {0};
	struct lpm_internal_send_msg send_msg = {
		.command_id = LPM_MSG_SET_RTC,
		.msg = msg,
		.msg_size = 6
	};
   	/*copy received values of rtc into message */
        if((new_rtc->tm_year >= MIN_RTC_YEAR)&&(new_rtc->tm_year <= MAX_RTC_YEAR))
                msg[5] = new_rtc->tm_year - MIN_RTC_YEAR;
        else
        	return  -EINVAL;

        msg[4] = new_rtc->tm_mon;
        msg[3] = new_rtc->tm_mday;
        msg[2] = new_rtc->tm_sec;
	msg[1] = new_rtc->tm_min;
	msg[0] = new_rtc->tm_hour;
	return lpm_exchange_msg(&send_msg, &response);
}
EXPORT_SYMBOL(stm_lpm_set_rtc);

/**
 * stm_lpm_get_wakeup_device - To get wake devices
 * @wakeup_device:	wakeup device
 *
 * This function will return wakeup device because of which SBC has
 * woken up the SOC.
 * In older protocol version wakeup devices were limited to 8,
 * whereas new Protocol version supports upto 10 wakeup devices.
 * Therefore two different message were used to get wakeup device.
 * Driver checks firmware version and send wakeup device accordingly.
 *
 * Return - 0 on success
 * Return - negative error code on failure.
 */

int stm_lpm_get_wakeup_device(enum stm_lpm_wakeup_devices *wakeup_device)
{
	int err = 0;
	struct lpm_message response = {0};
	struct lpm_internal_send_msg send_msg = {
		.command_id = LPM_MSG_GET_WUD,
		.msg_size = 0
	};
	if (unlikely(wakeup_device == NULL))
		return -EINVAL;
	if (lpm_fw_proto_version() >= 1) {
		struct stm_lpm_adv_feature  feature = {0};
		err = stm_lpm_get_adv_feature(0, &feature);
		if (likely(err == 0)) {
			*wakeup_device = feature.params.get_params[5] << 8;
			*wakeup_device |= feature.params.get_params[4];
		}
	} else {
		err = lpm_exchange_msg(&send_msg, &response);
		if (likely(err == 0))
			*wakeup_device = response.msg_data[0];
	}
	return err;
}
EXPORT_SYMBOL(stm_lpm_get_wakeup_device);

/**
 * stm_lpm_setup_fp - To set front panel information for SBC
 * @fp_setting:	Front panel setting
 *
 * This function will set front panel setting.
 * By default host CPU is assumed to control the front panel.
 *
 * Return - 0 on success
 * Return - negative error code on failure.
 */

int stm_lpm_setup_fp(struct stm_lpm_fp_setting *fp_setting)
{
	char msg;
	struct lpm_message response;
	struct lpm_internal_send_msg send_msg = {
		.command_id = LPM_MSG_SET_FP,
		.msg = &msg,
		.msg_size = 1
	};
	if (unlikely(fp_setting == NULL))
		return -EINVAL;

	msg = fp_setting->owner & OWNER_MASK;
	msg |= (fp_setting->am_pm & 1) << 2;
	msg |= (fp_setting->brightness & NIBBLE_MASK) << 4;
	return lpm_exchange_msg(&send_msg, &response);
}
EXPORT_SYMBOL(stm_lpm_setup_fp);

/**
 * stm_lpm_setup_pio - To inform SBC about PIO Use
 * @pio_setting:	pio_setting
 *
 * This function will inform SBC about PIO use,
 * driver running on Host must configure the PIO.
 * SBC will not do any configuration for PIO.
 * GPIO can be used as power control for board,
 * gpio interrupt, external interrupt, Phy WOL wakeup.
 *
 * Return - 0 on success
 * Return - negative error code on failure.
 */

int stm_lpm_setup_pio(struct stm_lpm_pio_setting *pio_setting)
{
	char msg[3];
	struct lpm_message response;
	struct lpm_internal_send_msg send_msg = {
		.command_id = LPM_MSG_SET_PIO,
		.msg = msg,
		.msg_size = 3
	};
	if (unlikely(pio_setting == NULL || (pio_setting->pio_direction
		 && pio_setting->interrupt_enabled)))
		return -EINVAL;

	msg[0] = pio_setting->pio_bank;
	if (pio_setting->pio_use == STM_LPM_PIO_EXT_IT)
		msg[0] = 0xFF;

	pio_setting->pio_pin &= NIBBLE_MASK;
	msg[1] = pio_setting->pio_level << PIO_LEVEL_SHIFT |
	pio_setting->interrupt_enabled <<  PIO_IT_SHIFT |
	pio_setting->pio_direction << PIO_DIRECTION_SHIFT |
	pio_setting->pio_pin ;

	msg[2] = pio_setting->pio_use;

	return lpm_exchange_msg(&send_msg, &response);
}
EXPORT_SYMBOL(stm_lpm_setup_pio);

/**
 * stm_lpm_setup_keyscan - To inform SBC about wakeup key of Keyscan
 * @key_data:	Keyscan Key info
 *
 * This function will inform SBC about keyscan key
 * on which SBC will wakeup.
 * Driver running on Host must configure the Keyscan IP.
 * SBC will not do any configuration.
 *
 * Return - 0 on success
 * Return - negative error code on failure.
 */

int stm_lpm_setup_keyscan(u16 key_data)
{
	char msg[2];
	struct lpm_message response;
	struct lpm_internal_send_msg send_msg = {
		.command_id = LPM_MSG_SET_KEY_SCAN,
		.msg = msg,
		.msg_size = 2
	};
	memcpy(msg, &key_data, 2);
	return  lpm_exchange_msg(&send_msg, &response);
}
EXPORT_SYMBOL(stm_lpm_setup_keyscan);

/**
 * stm_lpm_set_adv_feature - Set advance feature of SBC
 * @enabled:	If feature needs to enabled
 * 		pass 1 for enabling, 0 disabling
 * @feature:	Feature type and its parameters.
 * 		Features can be :
 * 		SBC VCORE External without parameter.
 * 		SBC Low voltage detect with value of voltage.
 * 		SBC clock selection(external, AGC or 32K).
 * 		SBC RTC source 32K_TCXO or 32K_OSC
 * 		SBC Wakeup triggers.
 *
 * This function will enable/disable selected feature on SBC
 *
 * Return - 0 on success
 * Return - negative error code on failure.
 */

int stm_lpm_set_adv_feature(u8 enabled, struct stm_lpm_adv_feature *feature)
{
	char msg[14];
	struct lpm_message response;
	struct lpm_internal_send_msg send_msg = {
		.command_id = LPM_MSG_SET_ADV_FEA,
		.msg = msg,
		.msg_size = 4
	};
	if (unlikely(feature == NULL))
		return -EINVAL;
	memcpy((msg+2), feature->params.set_params, 12);
	msg[0] = feature->feature_name;
	msg[1] = enabled;
	return lpm_exchange_msg(&send_msg, &response);
}
EXPORT_SYMBOL(stm_lpm_set_adv_feature);

/**
 * stm_lpm_get_adv_feature - To get current/supported features of SBC
 * @all_features:	If required to get all features.
 * 			pass 1 to get all supported features by SBC.
 * 			pass 0 to get all current enabled feature of SBC.
 * @features:		structure to get features of SBC
 *
 * Supported or currently  enabled features will be returned in get_params
 * field of features argument.
 * get_params[0-3] are bit map for each feature. Bit map 0-3 is reserved
 * get_params[4-5] are bit map for wakeup triggers.
 *
 * Return - 0 on success
 * Return - negative error code on failure.
 */

int stm_lpm_get_adv_feature(unsigned char all_features,
			struct stm_lpm_adv_feature *features)
{
	char msg = 0;
	int err = 0;
	struct lpm_message response;
	struct lpm_internal_send_msg send_msg = {
		.command_id = LPM_MSG_GET_ADV_FEA,
		.msg = &msg,
		.msg_size = 1
	};
	if (unlikely(features == NULL))
		return -EINVAL;
	msg = all_features;
	err = lpm_exchange_msg(&send_msg, &response);
	if (likely(err == 0))
		memcpy(features->params.get_params, response.msg_data, 10);
	return err;
}
EXPORT_SYMBOL(stm_lpm_get_adv_feature);
int stm_lpm_get_standby_time(u32 *time)
{
	int err;
	struct stm_lpm_adv_feature  feature = {0};
	err = stm_lpm_get_adv_feature(0, &feature);
	if (likely(err == 0)) {
		memcpy((char*)time, &feature.params.get_params[6], 4);
	}
	return err;
}
EXPORT_SYMBOL(stm_lpm_get_standby_time);
/**
 * stm_lpm_setup_fp_pio - To setup frontpanel long press GPIO
 * @pio_setting:		PIO data
 * @long_press_delay:		Delay to detect long presss
 * @default_reset_delay:	Default delay to do SOC reset
 *
 * This function will inform SBC about long press GPIO and its delays
 *
 * Return - 0 on success
 * Return - negative error code on failure.
 */
int stm_lpm_setup_fp_pio(struct stm_lpm_pio_setting *pio_setting,
			u32 long_press_delay, u32 default_reset_delay)
{
	char msg[14] = {0};
	int err = 0;
	struct lpm_message response;
	struct lpm_internal_send_msg send_msg = {
		.command_id = LPM_MSG_SET_PIO,
		.msg = msg,
		.msg_size = 14
	};
	if (unlikely(pio_setting == NULL || (pio_setting->pio_direction
		 && pio_setting->interrupt_enabled) ||
		 pio_setting->pio_use != STM_LPM_PIO_FP_PIO))
		return -EINVAL;
	msg[0] = pio_setting->pio_bank;
	pio_setting->pio_pin &= NIBBLE_MASK;
	msg[1] = pio_setting->pio_level << PIO_LEVEL_SHIFT |
	pio_setting->interrupt_enabled <<  PIO_IT_SHIFT |
	pio_setting->pio_direction << PIO_DIRECTION_SHIFT |
	pio_setting->pio_pin;
	msg[2] = pio_setting->pio_use;
	/*msg[3,4,5] are reserved */
	memcpy(&msg[6], &long_press_delay, 4);
	memcpy(&msg[10], &default_reset_delay, 4);
	err = lpm_exchange_msg(&send_msg, &response);
	return err;
}
EXPORT_SYMBOL(stm_lpm_setup_fp_pio);

/**
 * stm_lpm_setup_power_on_delay - To setup delay on wakeup
 * @de_bounce_delay:	This is button de bounce delay.
 * @dc_stability_delay:	this is DC stability delay
 *
 * This function will inform SBC about delays on detecting valid wakeup
 * If this is not called, SBC will use default delay
 *
 * Return - 0 on success
 * Return - negative error code on failure.
 */
int stm_lpm_setup_power_on_delay(u16 de_bounce_delay,
				u16 dc_stable_delay)
{
	struct stm_lpm_adv_feature feature;
	feature.feature_name = STM_LPM_DE_BOUNCE;
	put_unaligned_le16(de_bounce_delay, feature.params.set_params);
	stm_lpm_set_adv_feature(1, &feature);
	feature.feature_name = STM_LPM_DC_STABLE;
	put_unaligned_le16(dc_stable_delay, feature.params.set_params);
	return stm_lpm_set_adv_feature(1, &feature);
}
EXPORT_SYMBOL(stm_lpm_setup_power_on_delay);
/**
 * stm_lpm_set_cec_addr - to set CEC address for SBC
 * @addr:	CEC's physical and logical address
 *
 * This function will inform SBC about CEC phy and logical addresses
 *
 * Return - 0 on success
 * Return - negative error code on failure.
 */
int stm_lpm_set_cec_addr(struct stm_lpm_cec_address *addr)
{
	char msg[4] = {0};
	struct lpm_message response;
	struct lpm_internal_send_msg send_msg = {
		.command_id = LPM_MSG_CEC_ADDR,
		.msg = msg,
		.msg_size = 4
	};
	if (unlikely(addr == NULL))
		return -EINVAL;
	put_unaligned_le16(addr->phy_addr, msg);
	put_unaligned_le16(addr->logical_addr, &msg[2]);
	return lpm_exchange_msg(&send_msg, &response);
}
EXPORT_SYMBOL(stm_lpm_set_cec_addr);

/**
 * stm_lpm_cec_config - configure SBC for CEC WU or custom message
 * @use:	WU reason or custom message
 * @params: Data associated with use
 *
 * This function will inform SBC about CEC WU or custom message
 *
 * Return - 0 on success
 * Return - negative error code on failure.
 */
int stm_lpm_cec_config(enum stm_lpm_cec_select use,
			union stm_lpm_cec_params *params)
{
	char msg[14] = {0};
	struct lpm_message response;
	struct lpm_internal_send_msg send_msg = {
		.command_id = LPM_MSG_CEC_PARAMS,
		.msg = msg,
		.msg_size = 14
	};
	if (unlikely(NULL == params))
		return -EINVAL;
	msg[0] = use;
	if (use == STM_LPM_CONFIG_CEC_WU_REASON) {
		msg[1] = params->cec_wu_reasons;
	} else {
		msg[2] = params->cec_msg.size;
		memcpy((msg+3), &params->cec_msg.opcode, msg[2]);
	}
	return lpm_exchange_msg(&send_msg, &response);
}
EXPORT_SYMBOL(stm_lpm_cec_config);

/**
 * stm_lpm_cec_set_osd_name - configure SBC for CEC OSD Name
 * @params: Data associated with OSD name i.e. name and size
 *
 * This function will set CEC OSD name
 *
 * Return - 0 on success
 * Return - negative error code on failure.
 */


int stm_lpm_cec_set_osd_name(struct stm_lpm_cec_osd_msg *params)
{
        char msg[14] = {0};
        int len = 0;
        struct lpm_message response;

	struct lpm_internal_send_msg send_msg = {
		.command_id = LPM_MSG_CEC_SET_OSD_NAME,
		.msg = msg,
		.msg_size = 14
	};
	if (unlikely(NULL == params))
		return -EINVAL;
		
        msg[0] = params->size;

        if (msg[0] > STM_LPM_CEC_MAX_OSD_NAME_LENGTH)
                return -EINVAL;

        if(msg[0] < STM_LPM_CEC_MAX_OSD_NAME_LENGTH)
        {
                len = msg[0];
        }
        else if (msg[0] == STM_LPM_CEC_MAX_OSD_NAME_LENGTH)
        {
                len = msg[0]-1;
        }

        memcpy((msg+1),&params->name,len);
        if (msg[0] == STM_LPM_CEC_MAX_OSD_NAME_LENGTH)
                send_msg.trans_id = params->name[STM_LPM_CEC_MAX_OSD_NAME_LENGTH-1];

       return lpm_exchange_msg(&send_msg, &response);
}
EXPORT_SYMBOL(stm_lpm_cec_set_osd_name);

/**
 * stm_lpm_get_rtc - To get rtc time from standby controller
 * @new_rtc:	rtc value
 *
 *
 * Return - 0 on success
 * Return - negative error code on failure.
 */

int stm_lpm_get_rtc(struct rtc_time *new_rtc)
{
        char msg[1] = {0};
        int err = 0;
        struct lpm_message response;
	struct lpm_internal_send_msg send_msg = {
		.command_id = LPM_MSG_READ_RTC,
		.msg = msg,
		.msg_size = 1
	};
        int hours, t1;
        unsigned long timeInSec;
        struct rtc_time time1, time2;

        if (new_rtc == NULL)
                return -EINVAL;

        err = lpm_exchange_msg(&send_msg, &response);
        if (err >= 0 && (response.command_id & LPM_MSG_REPLY))
        {
                hours = ((response.msg_data[9]<<24) |(response.msg_data[8]<<16)|(response.msg_data[7]<<8)|(response.msg_data[6]));

                t1= ((( hours*60)+(response.msg_data[1]))*60 +response.msg_data[2] );
                time1.tm_year = response.msg_data[12] + MIN_RTC_YEAR;
                time1.tm_mon = response.msg_data[11];
                time1.tm_mday = response.msg_data[10];
                time1.tm_hour = 0;
                time1.tm_min = 0;
                time1.tm_sec = 0;
                timeInSec = mktime(time1.tm_year, time1.tm_mon,time1.tm_mday,time1.tm_hour,time1.tm_min,time1.tm_sec);
                timeInSec +=t1;
                rtc_time_to_tm(timeInSec , &time2);

                new_rtc->tm_year = time2.tm_year + 1900;
                new_rtc->tm_mon = time2.tm_mon + 1;
                new_rtc->tm_mday = time2.tm_mday;
                new_rtc->tm_sec = time2.tm_sec;
                new_rtc->tm_min = time2.tm_min;
                new_rtc->tm_hour = time2.tm_hour;
         }
	return err;
}
EXPORT_SYMBOL(stm_lpm_get_rtc);




