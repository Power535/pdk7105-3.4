#include <linux/stm/lpm.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/stm/platform.h>

#include "lpm_def.h"

#ifdef CONFIG_STM_LPM_DEBUG
#define lpm_debug(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define lpm_debug(fmt, ...)
#endif


static irqreturn_t hdmi_hpd_pio(int irq, void *ptr)
{
	lpm_debug("got my intr \n");
	return IRQ_HANDLED;
}
int stm_lpm_gpio_init(void);

/* communicated PIO to SBC */
int stm_lpm_gpio_init(void)
{
	int ret = 0;
        struct stm_lpm_pio_setting  configurepio;

	int  pio_port= (CONFIG_STM_LPM_HDMI_HPD&0xf0)>>0x4;
	int pio_pin =  (CONFIG_STM_LPM_HDMI_HPD&0x0f);
	
	/*request gpio */
	ret = gpio_request(stm_gpio(pio_port,pio_pin), "hdmi_gpio");
	if (ret < 0) {
		pr_err("ERROR: Request pin Not done!\n");
		return ret;
	}
	/*configure gpio as input  */
	gpio_direction_input(stm_gpio(pio_port,pio_pin));
	
	/* Set IRQ type at which edge wakeup is required */;
	irq_set_irq_type(gpio_to_irq(stm_gpio(pio_port,pio_pin)), IRQF_TRIGGER_RISING);

	/* register own function with PIO ISR */
	ret = request_irq(gpio_to_irq(stm_gpio(pio_port,pio_pin)),
	hdmi_hpd_pio, IRQF_DISABLED, "hdmi_hpd_pio", NULL);
	if (ret < 0) {
		gpio_free(stm_gpio(pio_port,pio_pin));
		pr_err("ERROR: Request irq Not done!\n");
		return ret;
	}
	/*enable interrupt */
	enable_irq_wake(gpio_to_irq(stm_gpio(pio_port,pio_pin)));

	/* Inform this PIO configuration to SBC */
    	configurepio.pio_level=STM_LPM_PIO_HIGH;
    	configurepio.interrupt_enabled=1;
    	configurepio.pio_direction=STM_LPM_PIO_INPUT;
    	configurepio.pio_use=STM_LPM_PIO_WAKEUP;
        configurepio.pio_bank=pio_port;
        configurepio.pio_pin=pio_pin;
        ret=stm_lpm_setup_pio(&configurepio);

	return ret;
}

