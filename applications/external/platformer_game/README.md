# Flipper Platformer Game

This is a simple platformer game developed for the Flipper Zero. The game allows players to control a character as they navigate through a level filled with obstacles and platforms, aiming to reach the finish line.

## Table of Contents

- [Features](#features)
- [Gameplay](#gameplay)
- [Controls](#controls)
- [Building & Running The Application](#building--running-the-application)
- [Game Structure](#game-structure)
- [License](#license)
- [Contributing](#contributing)

## Features

- **Platformer Mechanics**: Includes jumping, gravity, and collision detection.
- **Multiple Game States**: Play, Pause, Win, and Lose states with corresponding menus.
- **Basic Level Design**: One predefined level with obstacles, platforms, and a finish line.
- **Timer**: Tracks the time taken to complete the level.

## Gameplay

The objective of the game is to guide the character to the finish line as fast as possible without dying. The game ends when the player reaches the finish line (win) or falls off the screen (lose).

## Controls

- **Up**: Jump (x2 for Double Jump)
- **Left**: Move left
- **Right**: Move right
- **OK**: Select menu option
- **Back**: Pause game / Return to previous menu

### In-Game Menu Options

- **Pause Menu**: 
  - Continue
  - Restart
  - Exit
- **Win/Lose Menu**:
  - Restart
  - Exit

## Building & Running The Application

1. Run `ufbt` in the root directory (the one with the `application.fam` file in it). This will build your application and place the resulting binary in the `dist` subdirectory.
2. You can upload and start your application on a Flipper attached over USB using:
    ```sh
    ufbt launch
    ```
3. See [https://github.com/flipperdevices/flipperzero-ufbt](https://github.com/flipperdevices/flipperzero-ufbt) for more info.

### VS Code Integration

If you are using Visual Studio Code, you can integrate the build and launch process directly into the IDE:

1. Run `ufbt vscode_dist` in the root directory of your application. This will set up VS Code configurations for building and debugging.
2. Open the project in VS Code (`File > Open Folder...`).
3. Use the provided build (`Ctrl+Shift+B`) configurations to build and launch the app on your connected Flipper device.

You can also use the following VS Code task to build and launch the app directly:

```json
{
    "label": "Launch App on Flipper",
    "group": "build",
    "type": "shell",
    "command": "ufbt launch"
}
```

## Game Structure

The game consists of the following key components:

- **Character**: Represents the player's character, including position, velocity, and jumping state.
- **Game Context**: Manages the overall game state, input handling, and screen updates.
- **Level**: A predefined array that defines the level layout, including ground, platforms, and the finish line.
- **Menus**: Simple text-based menus for pause, win, and lose screens.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! If you have ideas for improvements or bug fixes, feel free to open an issue or submit a pull request.
