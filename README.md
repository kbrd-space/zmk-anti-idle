# zmk-anti-idle

A ZMK (Zephyr Mechanical Keyboard) module that actively prevents idle timeouts and keeps the device connected to the keyboard awake by periodically sending configurable actions (such as mouse movements or keypresses). This is useful for avoiding sleep due to inactivity.

## Features

- The anti-idle module prevents idle timeouts by periodically sending user-configurable actions (bindings) to the host (e.g., mouse movement, keypress)
- Allowing you to enable, disable, or toggle anti-idle mode via keymap bindings
- Optionally running only when a device is connected (USB/BLE), depending on configuration
- Supporting shared or per-endpoint state (see Kconfig options)
- Persisting state across reboots if Zephyr settings are enabled
- Easy integration with custom keymaps and behaviors through device tree overlays and Kconfig

## Module Registration

To use this module in your ZMK build, add it to your `west.yml` manifest file:

```yml
manifest:
  remotes:
    - name: kbrd-space
      url-base: https://github.com/kbrd-space
  projects:
    - name: zmk-anti-idle
      remote: kbrd-space
      revision: main
```


## Usage

### Basic Setup

1. Add the anti-idle module to your ZMK configuration (see Module Registration above).
2. Include the necessary device tree includes in your keymap:
   ```dts
   #include <behaviors/anti_idle_mode.dtsi>
   #include <dt-bindings/anti_idle_mode.h>
   ```
3. Configure the anti-idle behavior in your keymap overlay:
   ```dts
   / {
       anti_idle {
           compatible = "zmk,anti-idle-generic";
           interval-ms = <60000>; // How often to send actions (ms)
           bindings = <&mmv MOVE_DOWN &mmv MOVE_UP>; // Actions to perform (e.g., mouse move)
           tap_ms = <40>;   // Duration of each action (ms)
           wait_ms = <200>; // Delay between actions (ms)
       };
   };
   ```
4. Add anti-idle controls to your keymap layers:
   ```dts
   &aim AIM_ON   // Enable anti-idle mode
   &aim AIM_OFF  // Disable anti-idle mode
   &aim AIM_TOG  // Toggle anti-idle mode
   ```
5. Build and flash your firmware as usual.


### Configuration Options

- **`interval-ms`**: Time between anti-idle actions (default: 30000ms)
- **`bindings`**: Actions to perform (mouse movements, key presses, etc.)
- **`tap_ms`**: Duration of each action (default: 40ms)
- **`wait_ms`**: Delay between actions (default: 200ms)


## Kconfig Options

- `CONFIG_ZMK_ANTI_IDLE_ENABLE_ONLY_CONNECTED`: Only run anti-idle when a device is connected (USB/BLE). Default: enabled.
- `CONFIG_ZMK_ANTI_IDLE_SHARED_ENDPOINT_CONFIGURATION`: Share anti-idle state across all endpoints (profiles). Default: disabled (per-endpoint state).

## Project Structure

- `src/` - Source code for the anti-idle behavior
- `dts/` - Device tree bindings and overlays
- `tests/` - Automated test scenarios
- `scripts/` - Helper scripts for testing and setup
- `zephyr/` - Zephyr module configuration


## Development & Testing

To run tests:

```sh
./scripts/init-tests.sh
./scripts/run-tests.sh
```


## License

This project is licensed under the terms of the LICENSE file in this repository.
