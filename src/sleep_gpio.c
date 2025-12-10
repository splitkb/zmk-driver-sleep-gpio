#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h> 
#include <errno.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define DT_DRV_COMPAT splitkb_sleep_gpio

struct sleep_gpio_config {
    struct gpio_dt_spec gpio;
    struct gpio_dt_spec cs_gpio;
    struct gpio_dt_spec dc_gpio;
    struct gpio_dt_spec busy_gpio;
    struct gpio_dt_spec reset_gpio;
};

static int sleep_gpio_pm_action(const struct device *dev, enum pm_device_action action) {
    const struct sleep_gpio_config *config = dev->config;

    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND:
    case PM_DEVICE_ACTION_TURN_OFF:
        gpio_pin_configure_dt(&config->gpio, GPIO_INPUT | GPIO_PULL_DOWN);

        if (config->cs_gpio.port) {
            gpio_pin_configure_dt(&config->cs_gpio, GPIO_INPUT);
        }
        if (config->dc_gpio.port) {
            gpio_pin_configure_dt(&config->dc_gpio, GPIO_INPUT);
        }
        if (config->busy_gpio.port) {
            gpio_pin_configure_dt(&config->busy_gpio, GPIO_INPUT);
        }
        if (config->reset_gpio.port) {
            gpio_pin_configure_dt(&config->reset_gpio, GPIO_INPUT);
        }
        break;

    case PM_DEVICE_ACTION_RESUME:
        gpio_pin_configure_dt(&config->gpio, GPIO_OUTPUT_ACTIVE);

        if (config->cs_gpio.port) {
            gpio_pin_configure_dt(&config->cs_gpio, GPIO_OUTPUT_INACTIVE);
        }
        if (config->dc_gpio.port) {
            gpio_pin_configure_dt(&config->dc_gpio, GPIO_OUTPUT_INACTIVE);
        }
        if (config->reset_gpio.port) {
            gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
        }
        if (config->busy_gpio.port) {
            gpio_pin_configure_dt(&config->busy_gpio, GPIO_INPUT);
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

    return 0;
}

static const struct sleep_gpio_config config = {
    .gpio = GPIO_DT_SPEC_INST_GET(0, gpios),

    .cs_gpio = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(vik_spi), cs_gpios, {0}),
    .dc_gpio = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(mipi_dbi_spi0), dc_gpios, {0}),
    .reset_gpio = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(mipi_dbi_spi0), reset_gpios, {0}),
    .busy_gpio = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(vik_ssd1680), busy_gpios, {0}),
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