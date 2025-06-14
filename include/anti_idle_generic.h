/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

int zmk_anti_idle_toggle(void);
int zmk_anti_idle_get_state(bool *state);
int zmk_anti_idle_on(void);
int zmk_anti_idle_off(void);
