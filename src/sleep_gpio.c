#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h> 

#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define DT_DRV_COMPAT splitkb_sleep_gpio

struct sleep_gpio_config {
    struct gpio_dt_spec gpio;
    struct gpio_dt_spec cs_gpio;
};

static int sleep_gpio_pm_action(const struct device *dev, enum pm_device_action action) {
    const struct sleep_gpio_config *config = dev->config;

    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND:
    case PM_DEVICE_ACTION_TURN_OFF:
        // Force input + pull-down to keep pin Low during System Off
        gpio_pin_configure_dt(&config->gpio, GPIO_INPUT | GPIO_PULL_DOWN);
        if (config->cs_gpio.port) {
            gpio_pin_configure_dt(&config->cs_gpio, GPIO_INPUT | GPIO_PULL_DOWN);
        }
        break;
    case PM_DEVICE_ACTION_RESUME:
        // Restore output High on wake
        gpio_pin_configure_dt(&config->gpio, GPIO_OUTPUT_ACTIVE);
        if( config->cs_gpio.port) {
            gpio_pin_configure_dt(&config->cs_gpio, GPIO_OUTPUT_INACTIVE);
        }
        break;
    default:
        return -ENOTSUP;
    }

    return 0;
}

static int sleep_gpio_init(const struct device *dev) {
    const struct sleep_gpio_config *config = dev->config;

    if (!gpio_is_ready_dt(&config->gpio)) {
        LOG_ERR("Sleep GPIO device not ready");
        return -ENODEV;
    }

    int ret = gpio_pin_configure_dt(&config->gpio, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return ret;
    }

    if (config->cs_gpio.port) {
        if (!gpio_is_ready_dt(&config->cs_gpio)) {
            LOG_ERR("CS GPIO device not ready");
        } else {
            gpio_pin_configure_dt(&config->cs_gpio, GPIO_OUTPUT_INACTIVE);
        }
    }

    return 0;
}

static const struct sleep_gpio_config config = {
    .gpio = GPIO_DT_SPEC_INST_GET(0, gpios),

#if DT_NODE_HAS_PROP(DT_NODELABEL(vik_spi), cs_gpios)
    .cs_gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(vik_spi), cs_gpios),
#else
    .cs_gpio = {0},
#endif
};

PM_DEVICE_DEFINE(sleep_gpio_pm, sleep_gpio_pm_action);

DEVICE_DT_INST_DEFINE(0, 
    sleep_gpio_init, 
    PM_DEVICE_GET(sleep_gpio_pm), 
    NULL, 
    &config, 
    POST_KERNEL, 
    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, 
    NULL
);