# Dolphin-Counter Enhancement

This project enhances the Dolphin-Counter originally developed by [Krulknul](https://github.com/Krulknul). The new version now saves the counter value and can read it again after restarting the Flipper Zero.

## Tutorial

### 1. Beginners

Simply drop the `.fap` file into the `apps` directory on your Flipper Zero SD card using qFlipper or manually.

### 2. Professionals

If you're experienced, you know what to do with the counter folder. Just drop it inside `user_applications` and compile it with [`fbt`](https://github.com/flipperdevices/flipperzero-firmware/blob/dev/documentation/fbt.md) or [`ufbt`](https://pypi.org/project/ufbt/).

## Code Explanation

### Includes and Definitions

The code begins by including necessary libraries like `furi.h`, `gui/gui.h`, `input/input.h`, and others. These libraries provide essential functions for handling input, graphics, notifications, and storage. Several constants are defined, such as `MAX_COUNT`, `NUM_WIDTH`, and file paths, which are used throughout the program to control the behavior of the counter.

### Struct Definition

The `Counter` struct is defined to hold the state of the counter application. It includes fields like `input_queue`, `view_port`, `gui`, `mutex`, and `notifications` to manage different aspects of the application. Additionally, it stores the current `count`, a `pressed` state, a `boxtimer`, and a `vibro` flag to manage the vibration feature.

### Save and Load Counter Value

The functions `save_counter_value()` and `load_counter_value()` are implemented to persist the counter value to a file on the device's storage. This allows the counter to remember its state across restarts. The `save_counter_value()` function opens the file in write mode and saves the current count. Conversely, `load_counter_value()` reads the saved value from the file when the application starts.

### State Management

The `state_free()` function is responsible for cleaning up the application's resources when it exits. It removes the view port from the GUI, closes the mutex, and frees the memory allocated for the `Counter` struct.

### Input and Rendering Callbacks

The `input_callback()` function handles user input. It checks for short and long presses and enqueues them for processing. The `render_callback()` function is responsible for drawing the counter on the screen. It displays the counter value and updates the graphics based on the user's interaction.

### Initialization and Main Loop

The `state_init()` function initializes the `Counter` struct and sets up the input and rendering callbacks. It also loads the saved counter value, ensuring that the counter starts with the correct number after a restart.

The `texcounterapp()` function is the main loop of the application. It continuously processes user input and updates the counter value accordingly. It also manages the vibration feature, allowing the user to toggle it on and off. When the user exits the application, the state is cleaned up using `state_free()`.

This enhanced Dolphin-Counter allows users to increment or decrement the counter, save the current value, and continue from where they left off after restarting their Flipper Zero.

## Credits

- Original Dolphin-Counter by [Krulknul](https://github.com/Krulknul): [Dolphin Counter Repository](https://github.com/Krulknul/dolphin-counter)
- [fbt](https://github.com/flipperdevices/flipperzero-firmware/blob/dev/documentation/fbt.md) for building the Flipper Zero applications.
- [ufbt](https://pypi.org/project/ufbt/) for an alternative Flipper Zero build tool.
