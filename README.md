# Flow Desktop 🚀

**Flow Desktop** is a sleek, modern, and ultra-lightweight desktop environment for Linux systems. Built in modern C++ using XCB and GSettings, Flow Desktop offers a minimalist yet powerful interface ideal for low-resource systems or users who crave efficiency and customization.

- - -

## ✨ Features

*   **Modern Taskbar** 📏
    *   Customizable and efficient taskbar built with XCB.
    *   Multiple clickable buttons: **Apps**, **Terminal**, **Settings**, **Volume**, **Theme**, **About**, and **Logout**.
    *   Smooth integration with any lightweight window manager.
*   **Dynamic Application Menu** 📂
    *   Scans your `XDG_DATA_DIRS` for available `.desktop` files.
    *   Displays and launches applications via GDesktopAppInfo based on your click.
*   **Real-Time System Clock** ⏰
    *   Continuously updated clock with a modern design.
*   **Configuration File Support** ⚙️
    *   Customize your desktop with a simple configuration file (`~/.config/mydesktop.conf`):
    
    *   Set your preferred wallpaper.
    *   Choose your theme color.
    
*   **Volume Controls** 🔊
    *   Adjust system volume using keyboard shortcuts and an on-screen volume indicator.
    *   Integrated PulseAudio commands ensure smooth audio management.
*   **Theming and About Info** 🎨
    *   Toggle the desktop’s theme color on the fly.
    *   Open an "About" window for version and credits information.
*   **Lightweight & Extensible** ✨
    *   Written in C++ with modern practices and STL for easy maintenance and extension.
    *   Designed without unnecessary bloat, perfect for minimalist setups.

- - -

## 📋 Requirements

To run Flow Desktop, make sure you have the following installed:

*   **X11 Development Libraries** (e.g., `libxcb1-dev`)
*   **GIO Development Libraries** (for GSettings and desktop file handling, e.g., `libgio2.0-dev`)
*   **C++17** (or later) compatible compiler (e.g., `g++`)
*   **Meson** and **Ninja** for building
*   **Openbox** or another lightweight window manager (recommended)
*   **PulseAudio Utilities** for volume control (optional)

On Debian/Ubuntu, you can install dependencies with:

```
sudo apt update
sudo apt install build-essential meson ninja-build libxcb1-dev libgio2.0-dev openbox pulseaudio-utils g++
```

- - -

## 🛠️ Installation

### Building from Source

1.  **Clone the Repository** 📥
    
    ```
    git clone https://github.com/superuser-pushexe/Flow-Desktop.git
    cd Flow-Desktop
    ```
    
2.  **Set Up the Build Environment** 🛠️
    
    ```
    mkdir build && cd build
    meson setup ..
    ```
    
3.  **Compile the Source Code** 🔨
    
    ```
    ninja
    ```
    
4.  **Launch Flow Desktop** 🚀  
    Start the desktop environment (ensure your window manager is running):
    
    ```
    ./flow
    ```
    

- - -

## ⚙️ Configuration

Flow Desktop can be customized via a simple configuration file:

*   **File:** `~/.config/mydesktop.conf`
*   **Example Content:**
    
    ```
    wallpaper=/path/to/your/wallpaper.jpg
    themeColor=0x444444
    ```
    

Feel free to modify the source code as needed and recompile to adjust the taskbar layout, app menu behavior, or other UI elements.

- - -

## 🎮 Usage

*   **Taskbar Controls:**
    *   **Apps:** Opens the application menu listing all installed `.desktop` entries.
    *   **Terminal:** Launches the default terminal emulator (currently `xterm`).
    *   **Settings:** Opens a basic settings window (stub for future expansion).
    *   **Volume:** Displays the volume control window; use volume keys for adjustments.
    *   **Theme:** Toggles the desktop theme color to refresh the look and feel.
    *   **About:** Displays version and about information regarding Flow Desktop.
    *   **Logout:** Exits Flow Desktop.
*   **Keyboard Shortcuts:**
    *   Press the **Super/Windows key** to open the application menu.
    *   Use keys for volume control (compatible with PulseAudio).

Flow Desktop is designed to be intuitive and responsive, making it an excellent choice for both everyday use and development environments.

- - -

## 🤝 Contributing

Contributions are welcome! Whether you’re fixing bugs, adding new features, or enhancing the documentation:

1.  Fork the repository on [GitHub](https://github.com/superuser-pushexe/Flow-Desktop).
2.  Create a new branch for your feature:
    
    ```
    git checkout -b my-feature
    ```
    
3.  Commit your changes and push them:
    
    ```
    git commit -m "Add new feature"
    git push origin my-feature
    ```
    
4.  Open a pull request with a clear description of your changes.

Please follow the [Contributor Covenant Code of Conduct](CODE_OF_CONDUCT.md) for a welcoming environment.

- - -

## 📜 License

Flow Desktop is released under the [GNU General Public License v3.0](LICENSE). See the LICENSE file for details.

- - -

## 🌐 More Information

Visit the [Flow Desktop GitHub repository](https://github.com/superuser-pushexe/Flow-Desktop) for updates, issue tracking, and community discussions. Let’s build a desktop environment that flows—efficient, modern, and fully customizable!

Enjoy Flow Desktop, and happy coding!
