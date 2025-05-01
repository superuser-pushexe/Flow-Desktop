# Flow Desktop 🚀

**Flow Desktop** is a sleek, lightweight desktop environment for modern Linux systems, built with X11 (via XCB) and GTK4. It integrates seamlessly with the Openbox window manager, offering a minimal yet powerful interface with a customizable taskbar, application launchers, system clock, settings UI, and a code editor for developers. Designed for efficiency, Flow Desktop combines simplicity with modern features, making it ideal for low-resource systems or users who love a clean desktop experience. 🌟

---

## ✨ Features

- **Customizable Taskbar** 📏
  - Adjustable size and position, built with XCB for smooth X11 integration.
  - Override-redirect support ensures compatibility with various window managers.
- **Application Menu** 📂
  - Dynamically loads `.desktop` files from `XDG_DATA_DIRS` for easy app launching.
  - Supports modern desktop standards with `GDesktopAppInfo`.
- **System Clock** ⏰
  - Real-time clock display with efficient updates.
- **Settings UI** ⚙️
  - GTK4-based interface for managing wallpapers and viewing system info.
  - File chooser for selecting wallpapers, integrated with `GSettings`.
- **Flow Builder** 💻
  - A GTK4-based IDE with a source code editor (`GtkSourceView5`) and terminal (`VTE`).
  - Automatically sets up Flow Desktop programs via Git cloning with `libgit2`.
- **Volume Controls** 🔊
  - Keyboard shortcuts for volume adjustment and muting (PulseAudio-compatible).
- **Modern and Secure** 🔒
  - Uses XCB instead of Xlib for better performance and maintainability.
  - Safe `.desktop` parsing and command execution with GIO.
  - Robust error handling for X11 and GTK operations.
- **Wayland-Ready (Partial)** 🌍
  - GTK4 components (Settings and Builder) support Wayland natively.
  - Taskbar is X11-based but can be extended for Wayland with `libwayland-client`.

---

## 📋 Requirements

To run Flow Desktop, ensure you have the following installed:

- **X11 Development Libraries** (e.g., `libxcb1-dev`)
- **GTK4** (e.g., `libgtk-4-dev`)
- **GIO** (e.g., `libgio2.0-dev`) for `.desktop` file handling and settings
- **GtkSourceView5** (e.g., `libgtksourceview-5-dev`) for the code editor
- **VTE** (e.g., `libvte-2.91-dev`) for the terminal
- **libgit2** (e.g., `libgit2-dev`) for repository cloning
- **Openbox** window manager (recommended)
- **Meson** and **Ninja** for building
- **GCC** or another C compiler
- Optional: `pulseaudio-utils` for volume controls

On Debian/Ubuntu, install dependencies with:
```bash
sudo apt update
sudo apt install build-essential meson ninja-build libxcb1-dev libgtk-4-dev libgio2.0-dev libgtksourceview-5-dev libvte-2.91-dev libgit2-dev openbox pulseaudio-utils
```

---

## 🛠️ Installation

### Building from Source

1. **Clone the Repository** 📥
   ```bash
   git clone https://github.com/superuser-pushexe/Flow-Desktop.git
   cd Flow-Desktop
   ```

2. **Set Up the Build** 🛠️
   ```bash
   mkdir build && cd build
   meson setup ..
   ```

3. **Compile the Source Code** 🔨
   ```bash
   ninja
   ```

4. **Run Flow Desktop Components** 🚀
   - Start the desktop environment:
     ```bash
     ./flow
     ```
   - Open the settings UI:
     ```bash
     ./flow-settings
     ```
   - Launch the code editor:
     ```bash
     ./flow-builder
     ```

   **Note**: Ensure Openbox or another window manager is running before starting `flow`.

---

## ⚙️ Configuration

Customize Flow Desktop by editing the source files and recompiling:

- **Taskbar** (`flow.c`): Adjust `WIDTH_RATIO`, `HEIGHT`, and button positions.
- **Application Menu** (`flow.c`): Modify the `show_app_menu` function to filter or style apps.
- **Settings** (`flow-settings.c`): Add new tabs or settings to the GTK4 notebook.
- **Builder** (`flow-builder.c`): Customize the editor or terminal behavior.

After changes, recompile:
```bash
cd build
ninja
```

---

## 🎮 Usage

1. **Taskbar** 🖱️
   - Launch applications via the "Apps" button, which displays a menu of `.desktop` files.
   - Use the "Notify" or "Volume" buttons to open the volume control window.
   - Click "Logout" to exit Flow Desktop.
   - Press `Super` (Windows key) to open the application menu.
   - Use multimedia keys for volume control (requires PulseAudio).

2. **Settings UI** 🛠️
   - Open `flow-settings` to change wallpapers or view system info (memory usage).
   - Select wallpapers via a modern file chooser.

3. **Flow Builder** 💻
   - Open `flow-builder` to edit code or run commands in a terminal.
   - Automatically downloads Flow Desktop programs on first run.

The taskbar, settings, and builder are designed to be lightweight and intuitive, perfect for developers and minimalists alike! 🌈

---

## 🤝 Contributing

We love contributions! Whether it's fixing bugs 🐛, adding features ✨, or improving documentation 📚, your help makes Flow Desktop better. To contribute:

1. Fork the repository on [GitHub](https://github.com/superuser-pushexe/Flow-Desktop).
2. Create a branch for your changes:
   ```bash
   git checkout -b my-feature
   ```
3. Commit your changes and push:
   ```bash
   git commit -m "Add cool feature"
   git push origin my-feature
   ```
4. Open a pull request with a clear description.

Please follow the [Contributor Covenant Code of Conduct](CODE_OF_CONDUCT.md).

---

## 📜 License

Flow Desktop is licensed under the [GNU General Public License v3.0](LICENSE). See the [LICENSE](LICENSE) file for details.

---

## 🌐 More Information

Visit the [Flow Desktop GitHub repository](https://github.com/superuser-pushexe/Flow-Desktop) for updates, issues, and community discussions. Let’s build a lightweight desktop that flows! 🌊
