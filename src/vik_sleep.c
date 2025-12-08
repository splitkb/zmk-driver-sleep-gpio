/*
 * Copyright (c) 2026 Splitkb.com
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>

#define DT_DRV_COMPAT splitkb_vik_sleep

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define GET_SPEC_AND_COMMA(node_id, prop, idx) GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx),

#define EXTRACT_CHILD_OUTPUTS(child_node)                                                          \
    COND_CODE_1(DT_NODE_HAS_PROP(child_node, output_gpios),                                        \
                (DT_FOREACH_PROP_ELEM(child_node, output_gpios, GET_SPEC_AND_COMMA)), ())

#define EXTRACT_CHILD_INPUTS(child_node)                                                           \
    COND_CODE_1(DT_NODE_HAS_PROP(child_node, input_gpios),                                         \
                (DT_FOREACH_PROP_ELEM(child_node, input_gpios, GET_SPEC_AND_COMMA)), ())

static const struct gpio_dt_spec outputs[] = {DT_INST_FOREACH_CHILD(0, EXTRACT_CHILD_OUTPUTS)};

static const struct gpio_dt_spec inputs[] = {DT_INST_FOREACH_CHILD(0, EXTRACT_CHILD_INPUTS)};

static const struct gpio_dt_spec ctrl = GPIO_DT_SPEC_INST_GET_OR(0, control_gpios, {0});

static void configure_pins(const struct gpio_dt_spec *pins, size_t count, gpio_flags_t flags) {
    for (size_t i = 0; i < count; i++) {
        if (pins[i].port)
            gpio_pin_configure_dt(&pins[i], flags);
    }
}

static int sleep_pm_action(const struct device *dev, enum pm_device_action action) {
    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND:
    case PM_DEVICE_ACTION_TURN_OFF:
        if (ctrl.port)
            gpio_pin_configure_dt(&ctrl, GPIO_INPUT | GPIO_PULL_DOWN);
        configure_pins(outputs, ARRAY_SIZE(outputs), GPIO_INPUT);
        configure_pins(inputs, ARRAY_SIZE(inputs), GPIO_INPUT);
        break;
    case PM_DEVICE_ACTION_RESUME:
        if (ctrl.port)
            gpio_pin_configure_dt(&ctrl, GPIO_OUTPUT_ACTIVE);
        configure_pins(outputs, ARRAY_SIZE(outputs), GPIO_OUTPUT_INACTIVE);
        configure_pins(inputs, ARRAY_SIZE(inputs), GPIO_INPUT);
        break;
    default:
        return -ENOTSUP;
    }
    return 0;
}

static int sleep_init(const struct device *dev) {
    if (ctrl.port && !device_is_ready(ctrl.port))
        return -ENODEV;
    if (ctrl.port)
        gpio_pin_configure_dt(&ctrl, GPIO_OUTPUT_ACTIVE);
    return 0;
}

PM_DEVICE_DEFINE(vik_sleep_pm, sleep_pm_action);

DEVICE_DT_INST_DEFINE(0, sleep_init, PM_DEVICE_GET(vik_sleep_pm), NULL, NULL, POST_KERNEL,
                      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
