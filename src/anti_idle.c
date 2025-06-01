#include <stdlib.h>

#include <drivers/behavior.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/sys/dlist.h>

#include <zmk/behavior.h>
#include <zmk/behavior_queue.h>
#include <zmk/event_manager.h>
#include <zmk/hid.h>
#include <zmk/matrix.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if !DT_HAS_CHOSEN(zmk_anti_idle)
#error "Anti-idle node is not chosen. Configure anti-idle or disable the feature."
#endif

#define ANTI_IDLE_INSTANCE DT_CHOSEN(zmk_anti_idle)

#define BINDINGS_VAR(_name, inst, prop) \
    static struct zmk_behavior_binding _name[DT_PROP_LEN(inst, prop)] = { \
        LISTIFY(DT_PROP_LEN(inst, prop), ZMK_KEYMAP_EXTRACT_BINDING, (, ), inst) \
    };

#define ANTI_IDLE_INTERVAL_MS DT_PROP(ANTI_IDLE_INSTANCE, interval_ms)
#define ANTI_IDLE_TAP_MS DT_PROP_OR(ANTI_IDLE_INSTANCE, tap_ms, CONFIG_ZMK_MACRO_DEFAULT_TAP_MS)
#define ANTI_IDLE_WAIT_MS DT_PROP_OR(ANTI_IDLE_INSTANCE, wait_ms, CONFIG_ZMK_MACRO_DEFAULT_WAIT_MS)
#define ANTI_IDLE_BINDINGS_COUNT DT_PROP_LEN(ANTI_IDLE_INSTANCE, bindings)
BINDINGS_VAR(bindings, ANTI_IDLE_INSTANCE, bindings);

static struct k_work_delayable anti_idle_work;

static void anti_idle_handler(struct k_work *work) {
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

static int anti_idle_init() {
    k_work_init_delayable(&anti_idle_work, anti_idle_handler);
    k_work_schedule(&anti_idle_work, K_MSEC(ANTI_IDLE_INTERVAL_MS));
    return 0;
}

SYS_INIT(anti_idle_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
