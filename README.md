
# Flow Desktop

**Flow Desktop** is a lightweight desktop environment for X11 systems, written in C and utilizing Openbox as its window manager. Designed for minimal resource usage, it offers essential desktop functionality through a simple taskbar with application launchers and a clock.

## Features

- **Lightweight Taskbar**: Configurable height for the taskbar to suit user preferences.
- **Application Launchers**: Up to 5 customizable application launchers for quick access to frequently used programs.
- **System Clock**: Displays the current time on the taskbar.
- **Openbox Integration**: Seamless integration with the Openbox window manager.
- **Override-Redirect Taskbar**: Ensures compatibility with other window managers by using override-redirect for the taskbar window.
- **Basic Error Handling**: Handles common X11 operation errors gracefully.

## Requirements

- X11 development libraries (e.g., `libx11-dev` on Debian/Ubuntu)
- Openbox window manager
- GCC compiler
- Basic X11 utilities (e.g., `xterm` recommended)

## Installation

### Building from Source

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/superuser-pushexe/Flow-Desktop.git
   cd Flow-Desktop
   ```

2. **Compile the Source Code**:
   ```bash
   gcc flow.c -o flow-desktop -lX11
   ```

3. **Run Flow Desktop**:
   ```bash
   ./flow-desktop
   ```

   Ensure that Openbox is running before starting Flow Desktop.

## Configuration

Flow Desktop's taskbar and application launchers can be customized by editing the source code. Modify the `flow.c` file to change launcher commands, taskbar height, and other settings. After making changes, recompile the source code as described in the installation section.

## Usage

Upon running `flow-desktop`, a taskbar will appear, displaying up to 5 application launchers and the system clock. Clicking on a launcher will execute the associated command. The taskbar is designed to be minimalistic and efficient, providing essential functionality without unnecessary overhead.

## Contributing

Contributions are welcome! Feel free to submit issues or pull requests to improve Flow Desktop. Whether it's fixing bugs, adding features, or enhancing documentation, your help is appreciated.

## License

This project is licensed under the [GNU General Public License v3.0](LICENSE).


For more information and updates, visit the [Flow Desktop GitHub repository](https://github.com/superuser-pushexe/Flow-Desktop).
