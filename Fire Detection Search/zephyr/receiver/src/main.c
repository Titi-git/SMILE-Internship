/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
	     "No default LoRa radio specified in DT");

#define MAX_DATA_LEN 255

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_receive);

static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});

void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size,
		     int16_t rssi, int8_t snr)
{
	static int cnt;

	ARG_UNUSED(dev);
	ARG_UNUSED(size);

	LOG_INF("Received data: %s (RSSI:%ddBm, SNR:%ddBm)",
		data, rssi, snr);

	if (data[0] == 'D' && data[1] == 'I' && data[2] == 'N' && data[3] == 'G' && data[4] == 'D' && data[5] == 'O' && data[6] == 'N' && data[7] == 'G')
		{
			gpio_pin_set_dt(&led, 0);
			k_msleep(50);
			gpio_pin_set_dt(&led, 1);
		}

	/* Stop receiving after 10 packets */
	if (++cnt == 10) {
		LOG_INF("Stopping packet receptions");
		lora_recv_async(dev, NULL);
	}
}

int main(void)
{
	const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
	struct lora_modem_config config;
	int ret;

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s Device not ready", lora_dev->name);
		return 0;
	}

	if (led.port && !gpio_is_ready_dt(&led)) {
                led.port = NULL;
        }

	if (led.port) {
                ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
                if (ret != 0) {
                        printk("Error %d: failed to configure LED device %s pin %d\n",
                               ret, led.port->name, led.pin);
                        led.port = NULL;
                } else {
                        printk("Set up GPIO output at %s pin %d\n", led.port->name, led.pin);
                }
        }

	config.frequency = 865100000;
	config.bandwidth = BW_125_KHZ;
	config.datarate = SF_10;
	config.preamble_len = 8;
	config.coding_rate = CR_4_5;
	config.iq_inverted = false;
	config.public_network = false;
	config.tx_power = 14;
	config.tx = false;

	ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		LOG_ERR("LoRa config failed");
		return 0;
	}

	/* Receive 4 packets synchronously */
//	LOG_INF("Synchronous reception");
//	for (int i = 0; i < 4; i++) {
		/* Block until data arrives */
//		len = lora_recv(lora_dev, data, MAX_DATA_LEN, K_FOREVER,
//				&rssi, &snr);
//		if (len < 0) {
//			LOG_ERR("LoRa receive failed");
//			return 0;
//		}
	
//		LOG_INF("Received data: %s (RSSI:%ddBm, SNR:%ddBm)",
//			data, rssi, snr);
//	}

	/* Enable asynchronous reception */
	LOG_INF("Asynchronous reception");
	lora_recv_async(lora_dev, lora_receive_cb);
	k_sleep(K_FOREVER);
	return 0;
}
