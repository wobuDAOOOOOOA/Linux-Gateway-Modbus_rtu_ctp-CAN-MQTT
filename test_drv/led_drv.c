#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/gpio.h>

#define LED_GPIO  0              // GPIO0_A0 = GPIO编号0
#define LED_NAME  "led_demo"

static int __init led_init(void)
{
    int ret;

    printk(KERN_INFO "[LED] Module init\n");

    // 1. 申请GPIO
    ret = gpio_request(LED_GPIO, LED_NAME);
    if (ret) {
        printk(KERN_ERR "[LED] gpio_request failed for GPIO%d, ret=%d\n", LED_GPIO, ret);
        return ret;
    }

    // 2. 设为输出，输出高电平点亮LED
    ret = gpio_direction_output(LED_GPIO, 1);
    if (ret) {
        printk(KERN_ERR "[LED] gpio_direction_output failed, ret=%d\n", ret);
        gpio_free(LED_GPIO);
        return ret;
    }

    printk(KERN_INFO "[LED] GPIO%d set to HIGH, LED should be ON\n", LED_GPIO);
    return 0;
}

static void __exit led_exit(void)
{
    gpio_set_value(LED_GPIO, 0);   // 熄灭LED
    gpio_free(LED_GPIO);           // 释放GPIO
    printk(KERN_INFO "[LED] Module exit, GPIO%d freed\n", LED_GPIO);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhi");
MODULE_DESCRIPTION("LED driver for LubanCat RV06");