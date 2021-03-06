/*
 * <root>/include/linux/stm/lpm.h
 *
 * Interface file for stm lpm driver
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


#ifndef __LPM_H
#define __LPM_H

#include <linux/rtc.h>

/**
 * enum stm_lpm_wakeup_devices- define wakeup devices
 * One bit for each wakeup device
 */

enum stm_lpm_wakeup_devices{
	STM_LPM_WAKEUP_IR = 1<<0,
	STM_LPM_WAKEUP_CEC = 1<<1,
	STM_LPM_WAKEUP_FRP = 1<<2,
	STM_LPM_WAKEUP_WOL = 1<<3,
	STM_LPM_WAKEUP_RTC = 1<<4,
	STM_LPM_WAKEUP_ASC = 1<<5,
	STM_LPM_WAKEUP_NMI = 1<<6,
	STM_LPM_WAKEUP_HPD = 1<<7,
	STM_LPM_WAKEUP_PIO = 1<<8,
	STM_LPM_WAKEUP_EXT = 1<<9,
	STM_LPM_WAKEUP_MANUAL_RST = 1<<10,
	STM_LPM_WAKEUP_SW_WDT_RST = 1<<11,
	STM_LPM_WAKEUP_CUST = 1<<15
};

/**
 * enum stm_lpm_reset_type - define reset type
 * @STM_LPM_SOC_RESET:	SOC reset
 * @STM_LPM_SBC_RESET:	Only SBC reset
 * @STM_LPM_BOOT_RESET:	reset SBC and stay in bootloader
 */
enum stm_lpm_reset_type{
	STM_LPM_SOC_RESET = 0,
	STM_LPM_SBC_RESET = 1<<0,
	STM_LPM_BOOT_RESET = 1<<1,
	STM_LPM_HOST_RESET = 1<<2
};

/**
 * enum stm_lpm_sbc_state - defines SBC state
 * @STM_LPM_SBC_BOOT:	SBC waiting in bootloader
 * @STM_LPM_SBC_RUNNING:	SBC is running
 * @STM_LPM_SBC_STANDBY:	Entering into standby
 */

enum stm_lpm_sbc_state{
	STM_LPM_SBC_BOOT = 1,
	STM_LPM_SBC_RUNNING = 4,
	STM_LPM_SBC_STANDBY = 5
};

enum stm_lpm_pio_level{
        STM_LPM_PIO_LOW = 0,
        STM_LPM_PIO_HIGH = 1 << 7
};

/* defines  MIN RTC YEAR */
#define MIN_RTC_YEAR 2000
/* defines  MAX RTC YEAR */
#define MAX_RTC_YEAR 2255

/*
* stm_lpm_pio_direction
*
* Define direction of PIO input or output
*/
enum stm_lpm_pio_direction{
        STM_LPM_PIO_INPUT = 0,
        STM_LPM_PIO_OUTPUT = 1<<5
};

/**
 * struct stm_lpm_version - define version information
 * @major_comm_protocol:	Supported Major protocol version
 * @minor_comm_protocol:	Supported Minor protocol version
 * @major_soft:	Major software version
 * @minor_soft:	Minor software version
 * @patch_soft:	Software patch version
 * @month:	Software build month version
 * @day:	Software build day
 * @year:	Software build year
 * @custom_id:	Used for customized firmware by FAE
 *
 * Same struct is used for firmware and driver version information
 */

struct stm_lpm_version{
	char major_comm_protocol;
	char minor_comm_protocol;
	char major_soft;
	char minor_soft;
	char patch_soft;
	char month;
	char day;
	char year;
	char custom_id;
};

/**
 * struct stm_lpm_fp_setting - define front panel setting
 * @owner:	Owner of front panel
 * 		when 0 - SBC firmware will be owner in standby
 *		when 1 - SBC firmware always own frontpanel display
 *		when 2 - Host will always own front panel
 * @am_pm:	AM/PM indicator, when 0 clock will be displayed in 24 hrs format
 * @brightness:	brightness of display, [0-3] bits are used max value is 15
 * This is to inform SBC how front panel display will be used
 */
struct stm_lpm_fp_setting{
	char owner;
	char am_pm;
	char brightness;
};

/**
 * enum stm_lpm_pio_use - to define how pio can be used
 * @STM_LPM_PIO_POWER:	PIO used for power control
 * @STM_LPM_PIO_ETH_MDINT:	PIO used for phy WOL
 * @STM_LPM_PIO_WAKEUP:	PIO used as GPIO interrupt for wakeup
 * @STM_LPM_PIO_EXT_IT:	PIO used as external interrupt
 * @STM_LPM_PIO_OTHER:	Reserved
 */

enum stm_lpm_pio_use{
	STM_LPM_PIO_POWER = 1,
	STM_LPM_PIO_ETH_MDINT = 2,
	STM_LPM_PIO_WAKEUP = 3,
	STM_LPM_PIO_EXT_IT = 4,
	STM_LPM_PIO_OTHER = 5,
	STM_LPM_PIO_FP_PIO = 6,
};

/**
 * struct stm_lpm_pio_setting - define PIO use
 * @pio_bank:	pio bank number
 * @pio_pin:	pio pin number, valid values [0-7]
 * @pio_direction:	direction of PIO
 *		0 means, pio is used as input.
 *		1 means, pio is used as output.
 * @interrupt_enabled:	If interrupt on this PIO is enabled
 *		0 means, interrupts are disabled.
 *		1 means, interrupt are enabled.
 *		This must be set to 0 when pio is used as output.
 * @pio_level:	PIO level high or low.
 *		0 means, Interrupt/Power off will be done when PIO goes low.
 *		1 means, Interrupt/Power off will be done when PIO goes high.
 * @pio_use:	use of this pio
 *
 */

struct stm_lpm_pio_setting{
	char pio_bank;
	char pio_pin;
	bool pio_direction;
	bool interrupt_enabled;
	bool pio_level;
	enum stm_lpm_pio_use  pio_use;
};

/**
 * enum stm_lpm_adv_feature_name - Define name of advance feature of SBC
 * @STM_LPM_USE_EXT_VCORE:	feature is external VCORE for SBC
 * @STM_LPM_USE_INT_VOLT_DETECT:	internal low voltage detect
 * @STM_LPM_EXT_CLOCK:	external clock
 * @STM_LPM_RTC_SOURCE: RTC source for SBC
 * @STM_LPM_WU_TRIGGERS: wakeup triggers
 */

enum stm_lpm_adv_feature_name{
	STM_LPM_USE_EXT_VCORE = 1,
	STM_LPM_USE_INT_VOLT_DETECT = 2,
	STM_LPM_EXT_CLOCK = 3,
	STM_LPM_RTC_SOURCE = 4,
	STM_LPM_WU_TRIGGERS = 5,
	STM_LPM_DE_BOUNCE = 6,
	STM_LPM_DC_STABLE = 7,
	STM_LPM_SBC_IDLE = 8,
	STM_LPM_RTC_CALIBRATION_TIME = 9,
	STM_LPM_SET_EXIT_CPS = 0xA,
	STM_LPM_SBC_USER_CUSTOM_MSG = 0x40,
};

/**
 * struct stm_lpm_adv_feature - define advance feature struct
 * @feature_name:	Name of feature
 * @set_params:	Used to set feature on SBC
 *
 * 		when features is STM_LPM_USE_EXT_VCORE,
 *		set_parmas[0] = 0 means Internal Vcore
 *		set_parmas[0] = 1 means Internal Vcore
 *		set_parmas[1] is unused
 *
 * 		when features is STM_LPM_USE_INT_VOLT_DETECT
 *		set_parmas[0] is voltage value to detect low voltage
 *		i..e for 3.3V use 33 and for 3.0V use 20
 *		set_parmas[1] is unused
 *
 * 		when features is STM_LPM_EXT_CLOCK
 *		set_parmas[0] = 1 means external
 *		set_parmas[0] = 2 means external ACG
 *		set_parmas[0] = 3 means Track_32K
 *		set_parmas[1] is unused
 *
 * 		when features is STM_LPM_RTC_SOURCE
 *		set_parmas[0] = 1 means RTC_32K_TCXO
 *		set_parmas[0] = 2 means external ACG
 *		set_parmas[0] = 3 means RTC_32K_OSC
 *		set_parmas[1] is unused
 *
 * 		when features is STM_LPM_WU_TRIGGERS
 *		set_parmas[0-1] is bit map for each wakeup trigger
 *		as defined in enum stm_lpm_wakeup_devices
 * 
 * 		when features is STM_LPM_SBC_IDLE
 *		set_parmas[0] = 0x55
 *		set_parmas[1] = 0xAA
 *
 * 		when features is STM_LPM_USER_CUSTOM_MSG
 *		set_parmas[0-11] = User custom msg
 * @get_params: Used to get advance feature of SBC
 *		get_params[0-3] is feature set supported by SBC , TBD
 *		get_params[4-5] is wakeup triggers
 *
 */
struct stm_lpm_adv_feature{
	enum stm_lpm_adv_feature_name feature_name;
	union {
		unsigned char set_params[12];
		unsigned char get_params[10];
	} params;
};

/* defines  MAX depth for IR FIFO */
#define MAX_IR_FIFO_DEPTH 64


/**
 * struct stm_lpm_ir_fifo - define one element of IR fifo
 * @mark:	Mark time
 * @symbol:	Symbol time
 *
 */
struct stm_lpm_ir_fifo{
	u16 mark;
	u16 symbol;
};

/**
 * sturct stm_lpm_ir_key - define raw data for IR key
 * @key_index:	Key Index acts as key identifier
 * @num_patterns:	Number of mark/symbol pattern define this key
 * @fifo:	Place holder for mark/symbol data
 *
 * Max value of fifo is kept 64, which is max value of IR IP
 */

struct stm_lpm_ir_key{
	u8 key_index;
	u8 num_patterns;
	struct stm_lpm_ir_fifo fifo[MAX_IR_FIFO_DEPTH];
};


/*
 * stm_lpm_cec_set_osd_name
 * size of CEC OSD name
 * operand  -
*/

#define STM_LPM_CEC_MAX_OSD_NAME_LENGTH        14

struct stm_lpm_cec_osd_msg {
       u8 size;
       u8 name[STM_LPM_CEC_MAX_OSD_NAME_LENGTH];
};

/**
 * struct stm_lpm_ir_keyinfo - define a IR key along with another info
 * @ir_id:	Id of IR hardware, use id 0 for first IR, 1 for second IR
 * 		use id 0x80 for first UHF and 0x81 for second so on
 * @time_period:	Time period for this key , this is dependent on protocol
 * @time_out:	Time out period for this key
 * @tolerance:	Expected tolerance in IR key from standard value
 * @ir_key:	IR key data
 *
 */

struct stm_lpm_ir_keyinfo{
	u8 ir_id;
	u16 time_period;
	u16 time_out;
	u8 tolerance;
	struct stm_lpm_ir_key ir_key;
};

/*
 * stm_lpm_cec_address
 *  Define CEC address
 *  Physical address array in format
 * phy_addr[0].phy_addr[1].phy_addr[2].phy_addr[3]
 *  represent a.b.c.d
 * logical_addr is bit field, eachh bit set
 * enables its logical address
 */
struct stm_lpm_cec_address
{
        u16 phy_addr;
        u16 logical_addr;
};

/*
 * stm_lpm_cec_select
 * to define CEC parameter type
 * STM_LPM_CONFIG_CEC_WU_REASON Enabled disable CEC WU reason
 * STM_LPM_CONFIG_CEC_WU_CUSTOM_MSG Set Custom message for CEC WU
 *
 */
enum stm_lpm_cec_select
{
        STM_LPM_CONFIG_CEC_WU_REASON=1,
        STM_LPM_CONFIG_CEC_WU_CUSTOM_MSG=2,
	STM_LPM_CONFIG_CEC_VERSION=3,
        STM_LPM_CONFIG_CEC_DEVICE_VENDOR_ID=4,
};

/*
 * stm_lpm_cec_wu_reason
 * Define CEC WU reason
 * STM_LPM_CEC_WU_STREAM_PATH opcode 0x86
 * STM_LPM_CEC_WU_USER_POWER opcode 0x44, oprand 0x40
 * STM_LPM_CEC_WU_STREAM_POWER_ON opcode 0x44, oprand 0x6B
 * STM_LPM_CEC_WU_STREAM_POWER_TOGGLE opcode 0x44, oprand 0x6D
 * STM_LPM_CEC_WU_USER_MSG user defined message
*/

enum stm_lpm_cec_wu_reason
{
        STM_LPM_CEC_WU_STREAM_PATH=1,
        STM_LPM_CEC_WU_USER_POWER=2,
        STM_LPM_CEC_WU_STREAM_POWER_ON=4,
        STM_LPM_CEC_WU_STREAM_POWER_TOGGLE=8,
        STM_LPM_CEC_WU_USER_MSG=16,
};

/*
 * stm_lpm_cec_custom_msg
 * Define CEC custom message
 * size of CEC message including oprand and opcode
 * opcode -
 * oprand  -
*/
struct stm_lpm_cec_custom_msg {
        u8 size;
        u8 opcode;
        u8 oprand[10];
};

/*
 * stm_lpm_cec_params
 * to define CEC params
 * cec_wu_reasons - Bit field for each CEC wakeup device
 * Bits should be set as per enum stm_lpm_cec_wu_reason,
 * to enable a CEC WU reason
 * cec_msg - user defined CEC message
 */
union stm_lpm_cec_params
{
        u8 cec_wu_reasons;
        struct stm_lpm_cec_custom_msg cec_msg;
};

enum stm_lpm_inform_host_event_e
{
	STM_LPM_LONG_FP_PIO_PRESS = 1,
	STM_LPM_ALARM_TIMER = 2,
};


enum stm_lpm_config_reboot_type {
	stm_lpm_reboot_with_ddr_self_refresh,
	stm_lpm_reboot_with_ddr_off
};

int stm_lpm_config_reboot(enum stm_lpm_config_reboot_type type);

void stm_lpm_power_off(void);

int stm_lpm_configure_wdt(u16 time_in_ms, u8 wdt_type);

int stm_lpm_get_fw_state(enum stm_lpm_sbc_state *fw_state);

int stm_lpm_get_wakeup_device(enum stm_lpm_wakeup_devices *wakeupdevice);

int stm_lpm_get_wakeup_info(enum stm_lpm_wakeup_devices *wakeupdevice,
	u16 *validsize, u16 datasize, char  *data) ;

int stm_lpm_get_version(struct stm_lpm_version *drv_ver,
	struct stm_lpm_version *fw_ver);

int stm_lpm_reset(enum stm_lpm_reset_type reset_type);

int stm_lpm_setup_fp(struct stm_lpm_fp_setting *fp_setting);


int stm_lpm_setup_ir(u8 num_keys, struct stm_lpm_ir_keyinfo *keys);

int stm_lpm_set_rtc(struct rtc_time *new_rtc);

int stm_lpm_set_wakeup_device(u16  wakeup_devices);

int stm_lpm_set_wakeup_time(u32 timeout);

int stm_lpm_setup_pio(struct stm_lpm_pio_setting *pio_setting);

int stm_lpm_setup_keyscan(u16 key_data);

int stm_lpm_set_adv_feature(u8 enabled, struct stm_lpm_adv_feature *feature);

int stm_lpm_get_adv_feature(unsigned char all_features,
				struct stm_lpm_adv_feature *feature);
/* This API to update new SBC firmware at run-time */
/* usage update new firmware in lib/firmware area */
/* After updating firmware in above area call this API */
int stm_lpm_reload_sbc_firmware(void); 

/*
 *	return of this callback function 
 *	should be 0 if immediate reset is required <i.e time out value is zero>
 *	should be any positive value i.e timeout in ms after which reset is required 
 *	should be any negative value if not needed to send a ack to SBC for this long key press. 
 */
typedef int (*stm_lpm_reset_notifier_fn) (void);

/* to register user reset function */
void stm_lpm_register_reset_callback(stm_lpm_reset_notifier_fn user_fn);
int stm_lpm_setup_fp_pio(struct stm_lpm_pio_setting *pio_setting,
			u32 long_press_delay, u32 default_reset_delay);
int stm_lpm_setup_power_on_delay(u16 de_bounce_delay, u16 dc_stability_delay);

int stm_lpm_set_cec_addr(struct stm_lpm_cec_address *addr);

int stm_lpm_cec_config(enum stm_lpm_cec_select use,
			union stm_lpm_cec_params *params);

int stm_lpm_get_standby_time(u32 *time);

int stm_lpm_cec_set_osd_name(struct stm_lpm_cec_osd_msg *params);

int stm_lpm_get_rtc(struct rtc_time *new_rtc);

#endif /*__LPM_H*/
