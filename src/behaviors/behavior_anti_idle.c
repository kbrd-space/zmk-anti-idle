/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_anti_idle

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/keymap.h>

#include <dt-bindings/anti_idle.h>
#include <anti_idle_generic.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata no_arg_values[] = {
    {
        .display_name = "Toggle On/Off",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = ANTI_IDLE_TOG_CMD,
    },
    {
        .display_name = "Turn On",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = ANTI_IDLE_ON_CMD,
    },
    {
        .display_name = "Turn Off",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = ANTI_IDLE_OFF_CMD,
    },
};

static const struct behavior_parameter_metadata_set no_args_set = {
    .param1_values = no_arg_values,
    .param1_values_len = ARRAY_SIZE(no_arg_values),
};

static const struct behavior_parameter_metadata_set sets[] = {
    no_args_set,
    // hsv_value_metadata_set,
};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = ARRAY_SIZE(sets),
    .sets = sets,
};

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static int on_keymap_binding_pressed(
    struct zmk_behavior_binding *binding,
    struct zmk_behavior_binding_event event
) {
    LOG_DBG("handling keymap event (param %d)", binding->param1);
    switch (binding->param1) {
        case ANTI_IDLE_TOG_CMD:
            return zmk_anti_idle_toggle();
        case ANTI_IDLE_ON_CMD:
            return zmk_anti_idle_on();
        case ANTI_IDLE_OFF_CMD:
            return zmk_anti_idle_off();
    }

    return -ENOTSUP;
}

static int on_keymap_binding_released(
    struct zmk_behavior_binding *binding,
    struct zmk_behavior_binding_event event
) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_anti_idle_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_anti_idle_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
