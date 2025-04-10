# Flow Desktop

Flow Desktop is a lightweight X11 desktop environment written in C, utilizing Openbox as its window manager. It provides a simple taskbar with application launchers and a clock, designed for minimal resource usage while maintaining essential desktop functionality.

## Features
- Lightweight taskbar with configurable height
- Application launchers (up to 5) with customizable commands
- System clock display
- Openbox window manager integration
- Override-redirect taskbar for compatibility with other WMs
- Basic error handling for X11 operations

## Requirements
- X11 development libraries (`libx11-dev` on Debian/Ubuntu)
- Openbox window manager
- GCC compiler
- Basic X11 utilities (xterm recommended)

## Installation

### Building from Source
1. Clone this repository or download the source code:
`git clone <repository-url>`
`cd flow-desktop`

2. Compile the code:
`gcc -o flow-desktop flow-desktop.c -lX11`

3. Run the desktop:
`./flow-desktop`
Or start it with X:
`startx ./flow-desktop`

### Using Pre-built Binary
1. Download the latest release from the [Releases](/releases) page
2. Make it executable:
`chmod +x flow-desktop`
3. Run it:
`./flow-desktop`
Or with X:
`startx ./flow-desktop`

## Dependencies Installation
On Debian/Ubuntu:
`sudo apt-get install libx11-dev openbox xterm pcmanfm`

On Fedora:
`sudo dnf install libX11-devel openbox xterm pcmanfm`

## Usage
- The taskbar appears at the bottom of the screen
- Default launchers:
  - "Term" - Opens xterm
  - "Web" - Opens default browser to Google
  - "Files" - Opens PCManFM file manager
- Click launcher icons to start applications
- Clock updates in real-time

## Configuration
The following constants can be modified in the source code before compilation:
- `TASKBAR_HEIGHT` - Height of the taskbar (default: 30px)
- `LAUNCHER_SIZE` - Size of launcher icons (default: 24px)
- `LAUNCHER_PADDING` - Spacing between launchers (default: 5px)
- `CLOCK_WIDTH` - Width reserved for clock display (default: 100px)

To add or modify launchers, edit the `add_launcher()` calls in `main()`.

## Troubleshooting
- If the desktop doesn't start, ensure X11 is installed and try running with `startx`
- If launchers don't work, verify the corresponding applications are installed
- Check terminal output for X11-related errors

## License
This project is released under the MIT License. See LICENSE file for details.

## Contributing
Pull requests are welcome! For major changes, please open an issue first to discuss your proposed changes.
