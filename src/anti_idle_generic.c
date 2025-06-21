/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_anti_idle_generic

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/settings/settings.h>
#include <zephyr/device.h>
#include <zephyr/sys/dlist.h>

#include <zmk/behavior.h>
#include <zmk/behavior_queue.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/endpoints_types.h>
#include <zmk/event_manager.h>
#include <zmk/events/mouse_button_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/sensor_event.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define ANTI_IDLE_INSTANCE DT_INST(0, zmk_anti_idle_generic)

#define BINDINGS_VAR(_name, inst, prop) \
    static struct zmk_behavior_binding _name[DT_PROP_LEN(inst, prop)] = { \
        LISTIFY(DT_PROP_LEN(inst, prop), ZMK_KEYMAP_EXTRACT_BINDING, (, ), inst) \
    };

#define ANTI_IDLE_INTERVAL_MS DT_PROP(ANTI_IDLE_INSTANCE, interval_ms)
#define ANTI_IDLE_TAP_MS DT_PROP_OR(ANTI_IDLE_INSTANCE, tap_ms, CONFIG_ZMK_MACRO_DEFAULT_TAP_MS)
#define ANTI_IDLE_WAIT_MS DT_PROP_OR(ANTI_IDLE_INSTANCE, wait_ms, CONFIG_ZMK_MACRO_DEFAULT_WAIT_MS)
#define ANTI_IDLE_BINDINGS_COUNT DT_PROP_LEN(ANTI_IDLE_INSTANCE, bindings)
BINDINGS_VAR(bindings, ANTI_IDLE_INSTANCE, bindings);

struct anti_idle_state {
    bool on;
};

#if IS_ENABLED(CONFIG_ZMK_ANTI_IDLE_SHARED_ENDPOINT_CONFIGURATION)
static struct anti_idle_state state = { .on = false };
#else
#if ZMK_ENDPOINT_COUNT == 0
#error "ZMK_ENDPOINT_COUNT must be greater than 0 for anti-idle to work"
#endif // ZMK_ENDPOINT_COUNT == 0
static struct anti_idle_state state[ZMK_ENDPOINT_COUNT] = {[0 ... (ZMK_ENDPOINT_COUNT-1)] = { .on = false } };
#endif // IS_ENABLED(CONFIG_ZMK_ANTI_IDLE_SHARED_ENDPOINT_CONFIGURATION)

static struct k_work_delayable anti_idle_work;

int zmk_anti_idle_get_state(bool *on_off) {
#if IS_ENABLED(CONFIG_ZMK_ANTI_IDLE_SHARED_ENDPOINT_CONFIGURATION)
    *on_off = state.on;
#else 
    struct zmk_endpoint_instance endpoint_instance = zmk_endpoints_selected();
    int endpoint_index = zmk_endpoint_instance_to_index(endpoint_instance);
    *on_off = state[endpoint_index].on;
#endif // IS_ENABLED(CONFIG_ZMK_ANTI_IDLE_SHARED_ENDPOINT_CONFIGURATION)
    return 0;
}

int zmk_anti_idle_set_state(bool on_off) {
#if IS_ENABLED(CONFIG_ZMK_ANTI_IDLE_SHARED_ENDPOINT_CONFIGURATION)
    state.on = on_off;
    LOG_DBG("anti-idle state set to %s for shared", on_off ? "on" : "off");
#else
    struct zmk_endpoint_instance endpoint_instance = zmk_endpoints_selected();
    int endpoint_index = zmk_endpoint_instance_to_index(endpoint_instance);
    state[endpoint_index].on = on_off;
    LOG_DBG("anti-idle state set to %s for endpoint %d", on_off ? "on" : "off", endpoint_index);
#endif // IS_ENABLED(CONFIG_ZMK_ANTI_IDLE_SHARED_ENDPOINT_CONFIGURATION)
    return 0;
}

#if IS_ENABLED(CONFIG_ZMK_ANTI_IDLE_ENABLE_ONLY_CONNECTED)
static bool anti_idle_is_endpoint_connected(void) {
    enum zmk_transport transport = zmk_endpoints_selected().transport;

    switch (transport) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        case ZMK_TRANSPORT_USB:
            return zmk_usb_is_powered();
#endif
#if IS_ENABLED(CONFIG_ZMK_BLE)
        case ZMK_TRANSPORT_BLE:
            return zmk_ble_active_profile_is_connected();
#endif
        default:
            break;
    }

    LOG_ERR("unsupported transport type: %d", transport);
    return false;
}
#endif // !(IS_ENABLED(CONFIG_ZMK_ANTI_IDLE_ENABLE_ONLY_CONNECTED))

void raise_activity_to_prevent_sleep() {
    raise_zmk_sensor_event((struct zmk_sensor_event) {
        .sensor_index = UINT8_MAX,
        .channel_data_size = 1,
        .channel_data = {(struct zmk_sensor_channel_data) {
            .channel = SENSOR_CHAN_GAUGE_STATE_OF_CHARGE}},
        .timestamp = k_uptime_get()});
}

static void anti_idle_handler(struct k_work *work) {
    bool is_on;
    int err = zmk_anti_idle_get_state(&is_on);
    if (err) {
        LOG_ERR("Failed to get anti-idle state");
        return;
    }

    if (!is_on) {
        LOG_DBG("anti-idle is off, skipping execution");
        k_work_reschedule(&anti_idle_work, K_MSEC(ANTI_IDLE_INTERVAL_MS));
        return;
    }

#if IS_ENABLED(CONFIG_ZMK_ANTI_IDLE_ENABLE_ONLY_CONNECTED)
    if (!anti_idle_is_endpoint_connected()) {
        LOG_DBG("endpoint is not connected, skipping execution");
        k_work_reschedule(&anti_idle_work, K_MSEC(ANTI_IDLE_INTERVAL_MS));
        return;
    }
#endif // IS_ENABLED(CONFIG_ZMK_ANTI_IDLE_ENABLE_ONLY_CONNECTED)

    struct zmk_behavior_binding_event event = {
        .position = INT32_MAX,
        .timestamp = k_uptime_get(),
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        .source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
#endif
    };

    LOG_DBG("queue anti-idle %d bindings", ANTI_IDLE_BINDINGS_COUNT);
    for (int i = 0; i < ANTI_IDLE_BINDINGS_COUNT; i++) {
        struct zmk_behavior_binding binding = bindings[i];
        zmk_behavior_queue_add(&event, binding, true, ANTI_IDLE_TAP_MS);
        zmk_behavior_queue_add(&event, binding, false, ANTI_IDLE_WAIT_MS);
    }

    raise_activity_to_prevent_sleep();

    k_work_reschedule(&anti_idle_work, K_MSEC(ANTI_IDLE_INTERVAL_MS));
}

#if IS_ENABLED(CONFIG_SETTINGS)

#if IS_ENABLED(CONFIG_ZMK_ANTI_IDLE_SHARED_ENDPOINT_CONFIGURATION)
#define ANTI_IDLE_SETTINGS_NAME "anti-idle/shared"
#else
#define ANTI_IDLE_SETTINGS_NAME "anti-idle/endpoints"
#endif // IS_ENABLED(CONFIG_ZMK_ANTI_IDLE_SHARED_ENDPOINT_CONFIGURATION)

static int anti_idle_settings_load_cb(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    const char *next;
    int rc;

    if (settings_name_steq(name, "state", &next) && !next) {
        if (len != sizeof(state)) {
            return -EINVAL;
        }

        rc = read_cb(cb_arg, &state, sizeof(state));
        return MIN(rc, 0);
    }

    return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(anti_idle, ANTI_IDLE_SETTINGS_NAME, NULL, anti_idle_settings_load_cb, NULL, NULL);

static void zmk_anti_idle_save_state_work(struct k_work *_work) {
    settings_save_one(ANTI_IDLE_SETTINGS_NAME "/state", &state, sizeof(state));
    LOG_DBG("anti-idle %s state saved", ANTI_IDLE_SETTINGS_NAME);
}

static struct k_work_delayable anti_idle_save_work;

#endif // IS_ENABLED(CONFIG_SETTINGS)

static int anti_idle_init() {
#if IS_ENABLED(CONFIG_SETTINGS)
    k_work_init_delayable(&anti_idle_save_work, zmk_anti_idle_save_state_work);
#endif

    k_work_init_delayable(&anti_idle_work, anti_idle_handler);
    k_work_schedule(&anti_idle_work, K_MSEC(ANTI_IDLE_INTERVAL_MS));
    return 0;
}

int zmk_anti_idle_save_state(void) {
#if IS_ENABLED(CONFIG_SETTINGS)
    int ret = k_work_reschedule(&anti_idle_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
    return MIN(ret, 0);
#else
    return 0;
#endif
}

int zmk_anti_idle_on(void) {
    int err = zmk_anti_idle_set_state(true);
    if (err) {
        LOG_ERR("Failed to set anti-idle state to on");
        return err;
    }

    return zmk_anti_idle_save_state();
}

int zmk_anti_idle_off(void) {
    int err = zmk_anti_idle_set_state(false);
    if (err) {
        LOG_ERR("Failed to set anti-idle state to off");
        return err;
    }

    return zmk_anti_idle_save_state();
}

int zmk_anti_idle_toggle(void) {
    bool current_state;
    int err = zmk_anti_idle_get_state(&current_state);
    if (err) {
        LOG_ERR("Failed to get anti-idle state");
        return err;
    }

    return zmk_anti_idle_set_state(!current_state);
}

SYS_INIT(anti_idle_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

static int activity_event_listener(const zmk_event_t *eh) {
    int ret = k_work_reschedule(&anti_idle_work, K_MSEC(ANTI_IDLE_INTERVAL_MS));
    return MIN(ret, 0);
}

ZMK_LISTENER(anti_idle_activity, activity_event_listener);
ZMK_SUBSCRIPTION(anti_idle_activity, zmk_position_state_changed);
ZMK_SUBSCRIPTION(anti_idle_activity, zmk_sensor_event);

#endif // !DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
