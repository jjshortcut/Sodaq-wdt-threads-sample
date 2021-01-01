/*
 *  Copyright (c) 2016 Open-RnD Sp. z o.o.
 *  Copyright (c) 2020 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: Apache-2.0
 *  
 *  Sodaq assignment; objective:
 *  Design and implement a basic system for using a watchdog within a multi-threaded application. 
 *  A system which is able to detect a lockup in any of the threads. All threads report to a watchdog thread which feeds the watchdog
 *  
 *  The watchdog gets triggered by simulating a lockup in thread 2, this is done by pressing Button 1
 *  The user gets feedback by the serial port (baudrate 115200)
 *   
 *  Author: Jan-Jaap Schuurman 2020
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/printk.h>
#include <sys/__assert.h>
#include <string.h>
#include <drivers/watchdog.h>

/* Reset reason register masks */
#define NRF_POWER_RESETREAS_RESETPIN_MASK 0x01
#define NRF_POWER_RESETREAS_DOG_MASK      0x02
#define NRF_POWER_RESETREAS_SREQ_MASK     0x04
#define NRF_POWER_RESETREAS_LOCKUP_MASK   0x08
#define NRF_POWER_RESETREAS_OFF_MASK      0x00010000

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)

/* Button flag */
bool button_flag = 1;

/* Watchdog thread run flags */
bool thread1_flag = 0, thread2_flag = 0, thread3_flag = 0;

/*
 * To use this sample, either the devicetree's /aliases must have a
 * 'watchdog0' property, or one of the following watchdog compatibles
 * must have an enabled node.
 */
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_watchdog)
#define WDT_NODE DT_INST(0, nordic_nrf_watchdog)
#endif
/*
 * If the devicetree has a watchdog node, get its label property.
 */
#ifdef WDT_NODE
#define WDT_DEV_NAME DT_LABEL(WDT_NODE)
#else
#define WDT_DEV_NAME ""
#error "Unsupported SoC and no watchdog0 alias in zephyr.dts"
#endif

/* Button definition */
#define SW0_NODE	DT_ALIAS(sw0)
#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
#define SW0_GPIO_LABEL	DT_GPIO_LABEL(SW0_NODE, gpios)
#define SW0_GPIO_PIN	DT_GPIO_PIN(SW0_NODE, gpios)
#define SW0_GPIO_FLAGS	(GPIO_INPUT | DT_GPIO_FLAGS(SW0_NODE, gpios))

static struct gpio_callback button_cb_data;

#else
#error "Unsupported board: sw0 devicetree alias is not defined"
#define SW0_GPIO_LABEL	""
#define SW0_GPIO_PIN	0
#define SW0_GPIO_FLAGS	0
#endif

struct led {
	const char *gpio_dev_name;
	const char *gpio_pin_name;
	unsigned int gpio_pin;
	unsigned int gpio_flags;
};

/* ISR for shutting down program properly */
static void wdt_callback(const struct device *wdt_dev, int channel_id)
{
	static bool handled_event;

	if (handled_event) {
		return;
	}

	wdt_feed(wdt_dev, channel_id);

	printk("Handled things..ready to reset using watchdog\n");
	handled_event = true;
}

/* ISR called when button is pressed */
void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
}

static void print_reset_reason(void)
{
    uint32_t rr = NRF_POWER->RESETREAS; // Read reset reason
    printk("Reset reasons:");
    if (0 == rr)
    {
        printk("- NONE\n");
    }
    if (0 != (rr & NRF_POWER_RESETREAS_RESETPIN_MASK))
    {
        printk("- Reset from pin\n");
    }
    if (0 != (rr & NRF_POWER_RESETREAS_DOG_MASK     ))
    {
        printk("- Reset from pin WDT\n");
    }
    if (0 != (rr & NRF_POWER_RESETREAS_SREQ_MASK    ))
    {
        printk("- Reset from software\n");
    }
    if (0 != (rr & NRF_POWER_RESETREAS_SREQ_MASK    ))
    {
        printk("- Reset from powerup\n");
    }
    NRF_POWER->RESETREAS = 0xffffffff;  // Clear the flag
}

/* Thread 1 */
void blink0(void)
{
  int cnt = 0;
  const struct device *gpio_dev;

  const struct led led0 = {
  #if DT_NODE_HAS_STATUS(LED0_NODE, okay)
  .gpio_dev_name = DT_GPIO_LABEL(LED0_NODE, gpios),
  .gpio_pin_name = DT_LABEL(LED0_NODE),
  .gpio_pin = DT_GPIO_PIN(LED0_NODE, gpios),
  .gpio_flags = GPIO_OUTPUT | DT_GPIO_FLAGS(LED0_NODE, gpios),
  #else
  #error "Unsupported board: led0 devicetree alias is not defined"
  #endif
  };
  
  gpio_dev = device_get_binding(led0.gpio_dev_name);
  gpio_pin_configure(gpio_dev, led0.gpio_pin, led0.gpio_flags);

  printk("Blink0 thread started\n");

  while (1) {
    gpio_pin_set(gpio_dev, led0.gpio_pin, (cnt + 1) % 2);
    k_msleep(100);
    cnt++;
    thread1_flag = 1;
  }
}

/* Thread 2 */
void blink1(void)
{
  int cnt = 0;
  const struct device *gpio_dev;

  const struct led led1 = {
  #if DT_NODE_HAS_STATUS(LED1_NODE, okay)
  .gpio_dev_name = DT_GPIO_LABEL(LED1_NODE, gpios),
  .gpio_pin_name = DT_LABEL(LED1_NODE),
  .gpio_pin = DT_GPIO_PIN(LED1_NODE, gpios),
  .gpio_flags = GPIO_OUTPUT | DT_GPIO_FLAGS(LED1_NODE, gpios),
  #else
  #error "Unsupported board: led1 devicetree alias is not defined"
  #endif
  };

  gpio_dev = device_get_binding(led1.gpio_dev_name);
  gpio_pin_configure(gpio_dev, led1.gpio_pin, led1.gpio_flags);

  printk("Blink1 thread started\n");

  while (1) {
    if(button_flag)   // if button is pressed disable/enable thread running (led and thread flag)
    {
      gpio_pin_set(gpio_dev, led1.gpio_pin, (cnt + 1) % 2);
      for(int i=0; i<8; i++)
      {k_msleep(100);
      thread2_flag = 1;}
      cnt++; 
    }
    k_msleep(100);
  }
}

/* Thread 3 */
void uart_out(void)
{
  int cnt = 1;
  printk("Serial feedback thread started\n");
  
  while (1) {
    printk("Toggle USR1 LED1: Counter = %d\n", cnt);
    if (cnt >= 10) {
          
            printk("Toggle USR2 LED2: Counter = %d\n", cnt);
            cnt = 0;
    }
    k_msleep(100);
    cnt++;
    thread3_flag = 1;
  }
}

/* Thread 4 */
void button_read(void)
{
  int ret;
  static bool thread_is_running=1;

  const struct device *button;
  button = device_get_binding(SW0_GPIO_LABEL);
  if (button == NULL) {
          printk("Error: didn't find %s device\n", SW0_GPIO_LABEL);
          return;
  }

  ret = gpio_pin_configure(button, SW0_GPIO_PIN, SW0_GPIO_FLAGS);
  if (ret != 0) {
          printk("Error %d: failed to configure %s pin %d\n",
                 ret, SW0_GPIO_LABEL, SW0_GPIO_PIN);
          return;
  }

  ret = gpio_pin_interrupt_configure(button, SW0_GPIO_PIN, GPIO_INT_EDGE_TO_ACTIVE);
  if (ret != 0) {
    printk("Error %d: failed to configure interrupt on %s pin %d\n",
            ret, SW0_GPIO_LABEL, SW0_GPIO_PIN);
    return;
  }

  gpio_init_callback(&button_cb_data, button_pressed, BIT(SW0_GPIO_PIN));
  gpio_add_callback(button, &button_cb_data);
  //printk("Set up button at %s pin %d\n", SW0_GPIO_LABEL, SW0_GPIO_PIN);

  printk("Button thread running\n");
  while(1){
    bool val = gpio_pin_get(button, SW0_GPIO_PIN);  // check button status

    if(val) // if button is pressed; suspend or resume thread
    {
      if(thread_is_running) {
        button_flag=0;
        thread_is_running=0;
       printk("Stopped thread\n");
      }
      else {
        button_flag=1;
        thread_is_running=1;
        printk("Resumed thread\n");
      }
      k_msleep(250); // debounce
    }
    k_msleep(50);
  }
}

/* Thread 5 */
void watchdog(void)
{
  int err;
  int wdt_channel_id;
  const struct device *wdt;
  struct wdt_timeout_cfg wdt_config;

  print_reset_reason(); // read RESETREAS and print cause

  wdt = device_get_binding(WDT_DEV_NAME);
  if (!wdt) {
          printk("Cannot get WDT device\n");
          return;
  }

  /* Reset SoC when watchdog timer expires. */
  wdt_config.flags = WDT_FLAG_RESET_SOC;

  /* Expire watchdog after 500 milliseconds. */
  wdt_config.window.min = 0U;
  wdt_config.window.max = 500U;
  printk("WDT timeout = %dms\n", wdt_config.window.max);

  /* Set up watchdog callback. Jump into it when watchdog expired. */
  wdt_config.callback = wdt_callback;

  wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
  if (wdt_channel_id == -ENOTSUP) {
    /* IWDG driver for STM32 doesn't support callback */
    wdt_config.callback = NULL;
    wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
  }
  if (wdt_channel_id < 0) {
    printk("Watchdog install error\n");
    return;
  }

  err = wdt_setup(wdt, 0);
  if (err < 0) {
    printk("Watchdog setup error\n");
    return;
  }

  printk("Watchdog thread started\n");

  while(1)  {
    k_msleep(50);
    
    if(thread1_flag && thread2_flag && thread3_flag) // Check run flags
    { 
      //printk("Feeding watchdog...\n");
      wdt_feed(wdt, wdt_channel_id); // OK, feed watchdog
       thread1_flag = 0;  // Reset run flags
       thread2_flag = 0;
       thread3_flag = 0;
    }
  }
}

K_THREAD_DEFINE(blink0_id, STACKSIZE, blink0, NULL, NULL, NULL,
		PRIORITY, 0, 0);
K_THREAD_DEFINE(blink1_id, STACKSIZE, blink1, NULL, NULL, NULL,
		PRIORITY, 0, 0);
K_THREAD_DEFINE(uart_out_id, STACKSIZE, uart_out, NULL, NULL, NULL,
		PRIORITY, 0, 0);
K_THREAD_DEFINE(button_read_id, STACKSIZE, button_read, NULL, NULL, NULL,
                PRIORITY, 0, 0);
K_THREAD_DEFINE(watchdog_id, STACKSIZE, watchdog, NULL, NULL, NULL,
                PRIORITY, 0, 0);