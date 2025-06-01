# zmk-anti-idle

A ZMK (Zephyr Mechanical Keyboard) module to prevent idle timeouts and keep your device active. This project provides an "anti-idle" behavior for ZMK-powered keyboards, useful for scenarios where you want to avoid sleep or disconnects due to inactivity.

## Features

- Prevents idle timeouts on ZMK devices
- Configurable via device tree overlays and Kconfig
- Easy integration with custom keymaps and behaviors

## Usage

1. Add the anti-idle module to your ZMK configuration.
2. Configure the behavior in your keymap or device tree overlay.
3. Build and flash your firmware as usual.

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
