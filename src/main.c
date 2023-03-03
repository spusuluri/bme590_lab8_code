/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Lab8_Satya, LOG_LEVEL_DBG);

#define SLEEP_TIME_MS   1000
#define HEARTBEAT_PERIOD_MS 1000

/*LEDs*/
static const struct gpio_dt_spec heartbeat_led = GPIO_DT_SPEC_GET(DT_ALIAS(heartbeat), gpios);
static const struct gpio_dt_spec buzzer_led = GPIO_DT_SPEC_GET(DT_ALIAS(buzzer), gpios);
static const struct gpio_dt_spec ivdrip_led = GPIO_DT_SPEC_GET(DT_ALIAS(ivdrip), gpios);
static const struct gpio_dt_spec alarm_led = GPIO_DT_SPEC_GET(DT_ALIAS(alarm), gpios);
static const struct gpio_dt_spec error_led = GPIO_DT_SPEC_GET(DT_ALIAS(error), gpios);

/*Buttons*/
static const struct gpio_dt_spec sleep = GPIO_DT_SPEC_GET(DT_ALIAS(button0), gpios);
static const struct gpio_dt_spec freq_up = GPIO_DT_SPEC_GET(DT_ALIAS(button1), gpios);
static const struct gpio_dt_spec freq_down = GPIO_DT_SPEC_GET(DT_ALIAS(button2), gpios);
static const struct gpio_dt_spec reset = GPIO_DT_SPEC_GET(DT_ALIAS(button3), gpios);

static bool sleep_detected = 0;
static bool freq_up_detected = 0;
static bool freq_down_detected = 0;
static bool reset_detected = 0;

static struct gpio_callback sleep_cb;
static struct gpio_callback freq_up_cb;
static struct gpio_callback freq_down_cb;
static struct gpio_callback reset_cb;

/* Declarations*/
int setup_channels_and_pins(void);
int check_interfaces_ready(void);
void sleep_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void freq_up_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void freq_down_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void reset_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void heartbeat_toggle(struct k_timer *heartbeat_timer);

/*Callbacks*/
void sleep_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	LOG_DBG("Sleep button pressed.");
	sleep_detected = 1;
}
void freq_up_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	LOG_DBG("Freq Up button pressed.");
	freq_up_detected = 1;
}
void freq_down_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	LOG_DBG("Freq down button pressed.");
	freq_down_detected = 1;
}
void reset_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	LOG_DBG("Reset button pressed.");
	reset_detected = 1;
}

/*Timers*/
K_TIMER_DEFINE(heartbeat_timer, heartbeat_toggle, NULL);

void heartbeat_toggle(struct k_timer *heartbeat_timer){
	gpio_pin_toggle_dt(&heartbeat_led);
	LOG_DBG("Heartbeat");
}

void main(void)
{
	int err;

	err = check_interfaces_ready();
	if (err){
		LOG_ERR("Device interfaces not ready (err = %d)", err);
	}

	err = setup_channels_and_pins();
	if (err){
		LOG_ERR("Error configuring IO pins (err = %d)", err);
	}

	/* Setup Callbacks*/
	err = gpio_pin_interrupt_configure_dt(&sleep, GPIO_INT_EDGE_TO_ACTIVE);
	if (err < 0){
		LOG_ERR("Error configuring sleep callback");
		return;
	}	
	gpio_init_callback(&sleep_cb, sleep_callback, BIT(sleep.pin));
	gpio_add_callback(sleep.port, &sleep_cb);

	err = gpio_pin_interrupt_configure_dt(&freq_up, GPIO_INT_EDGE_TO_ACTIVE);
	if (err < 0){
		LOG_ERR("Error configuring freq_up callback");
		return;
	}	
	gpio_init_callback(&freq_up_cb, freq_up_callback, BIT(freq_up.pin));
	gpio_add_callback(freq_up.port, &freq_up_cb);

	err = gpio_pin_interrupt_configure_dt(&freq_down, GPIO_INT_EDGE_TO_ACTIVE);
	if (err < 0){
		LOG_ERR("Error configuring freq_down callback");
		return;
	}	
	gpio_init_callback(&freq_down_cb, freq_down_callback, BIT(freq_down.pin));
	gpio_add_callback(freq_down.port, &freq_down_cb);

	err = gpio_pin_interrupt_configure_dt(&reset, GPIO_INT_EDGE_TO_ACTIVE);
	if (err < 0){
		LOG_ERR("Error configuring reset callback");
		return;
	}	
	gpio_init_callback(&reset_cb, reset_callback, BIT(reset.pin));
	gpio_add_callback(reset.port, &reset_cb);

	/* Start indefinite timer*/
	k_timer_start(&heartbeat_timer, K_MSEC(HEARTBEAT_PERIOD_MS), K_MSEC(HEARTBEAT_PERIOD_MS));

	while (1) {
		if (sleep_detected){
			sleep_detected = 0;
		}
		if (freq_up_detected){
			freq_up_detected = 0;
		}
		if (freq_down_detected){
			freq_down_detected=0;
		}
		if (reset_detected){
			reset_detected = 0;
		}
		gpio_pin_toggle_dt(&error_led);
		/*
		ret = gpio_pin_toggle_dt(&heartbeat_led);
		ret = gpio_pin_toggle_dt(&buzzer_led);
		ret = gpio_pin_toggle_dt(&ivdrip_led);
		ret = gpio_pin_toggle_dt(&alarm_led);
		*/
		LOG_DBG("Error LED is blinking!");
		k_msleep(SLEEP_TIME_MS);
	}
}
int setup_channels_and_pins(void){
	int ret;
	/* Configure GPIO pins*/
	ret = gpio_pin_configure_dt(&heartbeat_led, GPIO_OUTPUT_INACTIVE | GPIO_ACTIVE_LOW);
	if (ret < 0) {
		LOG_ERR("Cannot configure heartbeat led.");
		return ret;
	}
	ret = gpio_pin_configure_dt(&buzzer_led, GPIO_OUTPUT_INACTIVE | GPIO_ACTIVE_LOW);
	if (ret < 0){
		LOG_ERR("Cannot configure buzzer led.");
		return ret;
	}
	ret = gpio_pin_configure_dt(&ivdrip_led, GPIO_OUTPUT_INACTIVE | GPIO_ACTIVE_LOW);
	if (ret < 0){
		LOG_ERR("Cannot configure IV Drip led.");
		return ret;
	}
	ret = gpio_pin_configure_dt(&alarm_led, GPIO_OUTPUT_INACTIVE | GPIO_ACTIVE_LOW);
	if (ret < 0){
		LOG_ERR("Cannot configure alarm led.");
		return ret;
	}
	ret = gpio_pin_configure_dt(&error_led, GPIO_OUTPUT_INACTIVE | GPIO_ACTIVE_LOW);
	if (ret < 0){
		LOG_ERR("Cannot configure error led.");
		return ret;
	}
	ret = gpio_pin_configure_dt(&sleep, GPIO_INPUT);
	if (ret < 0){
		LOG_ERR("Cannot configure sleep button.");
		return ret;
	}
	ret = gpio_pin_configure_dt(&freq_up, GPIO_INPUT);
	if (ret < 0){
		LOG_ERR("Cannot configure freq_up button.");
		return ret;
	}
	ret = gpio_pin_configure_dt(&freq_down, GPIO_INPUT);
	if (ret < 0){
		LOG_ERR("Cannot configure freq_down button.");
		return ret;
	}
	ret = gpio_pin_configure_dt(&reset, GPIO_INPUT);
	if (ret < 0){
		LOG_ERR("Cannot configure reset button.");
		return ret;
	}
	return 0;
}

int check_interfaces_ready(void){
	/* This should check for the entire gpio0 interface*/
	if (!device_is_ready(heartbeat_led.port)) {
		LOG_ERR("gpio0 interface not ready.");
		return -1;
	}
	return 0;	
}
