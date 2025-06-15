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
#include <zephyr/pm/device.h>
#include <zephyr/device.h>
#include <zephyr/sys/dlist.h>

#include <zmk/behavior.h>
#include <zmk/behavior_queue.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/endpoints_types.h>
#include <zmk/event_manager.h>
#include <zmk/hid.h>
#include <zmk/matrix.h>
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

static struct anti_idle_state state = { .on = false };

static struct k_work_delayable anti_idle_work;

#if IS_ENABLED(CONFIG_ZMK_ANTI_IDLE_ENABLE_ONLY_CONNECTED)
static bool anti_idle_is_endpoint_connected(void) {
    struct zmk_endpoint_instance endpoint_instance = zmk_endpoints_selected();

    switch (endpoint_instance.transport) {
        case ZMK_TRANSPORT_USB:
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
            bool usb_is_connected = zmk_usb_is_powered();
            return usb_is_connected;
#else
            break;
#endif
        case ZMK_TRANSPORT_BLE:
#if IS_ENABLED(CONFIG_ZMK_BLE)
            bool ble_is_connected = zmk_ble_active_profile_is_connected();
            return ble_is_connected;
#else
            break;
#endif
    }

    LOG_ERR("Unsupported transport type: %d", endpoint_instance.transport);
    return false;
}
#endif // !(IS_ENABLED(CONFIG_ZMK_ANTI_IDLE_ENABLE_ONLY_CONNECTED))

static void anti_idle_handler(struct k_work *work) {
    if (!state.on) {
        LOG_DBG("anti-idle is off, skipping execution");
        return;
    }

#if IS_ENABLED(CONFIG_ZMK_ANTI_IDLE_ENABLE_ONLY_CONNECTED)
    if (!anti_idle_is_endpoint_connected()) {
        LOG_DBG("endpoint is not connected, skipping execution");
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

    k_work_schedule(&anti_idle_work, K_MSEC(ANTI_IDLE_INTERVAL_MS));
}

#if IS_ENABLED(CONFIG_SETTINGS)

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

SETTINGS_STATIC_HANDLER_DEFINE(anti_idle, "anti-idle", NULL, anti_idle_settings_load_cb, NULL, NULL);

static void zmk_anti_idle_save_state_work(struct k_work *_work) {
    settings_save_one("anti-idle/state", &state, sizeof(state));
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

int zmk_anti_idle_get_state(bool *on_off) {
    *on_off = state.on;
    return 0;
}

int zmk_anti_idle_on(void) {
    LOG_DBG("enable");
    state.on = true;
    return zmk_anti_idle_save_state();
}

int zmk_anti_idle_off(void) {
    LOG_DBG("disable");
    state.on = false;
    return zmk_anti_idle_save_state();
}

int zmk_anti_idle_toggle(void) {
    return state.on ? zmk_anti_idle_off() : zmk_anti_idle_on();
}

SYS_INIT(anti_idle_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif // !DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
