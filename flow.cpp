#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <dirent.h>
#include <unistd.h>
#include <ctime>
#include <gio/gio.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

// Constants for dimensions
const int HEIGHT = 40;
const int CLOCK_WIDTH = 120;
const int BUTTON_WIDTH = 80;
const int BUTTON_HEIGHT = 30;
const int APP_MENU_HEIGHT = 400;
const int APP_MENU_WIDTH = 300;
const int SETTINGS_WIDTH = 300;
const int SETTINGS_HEIGHT = 200;
const int VOL_WIDTH = 200;
const int VOL_HEIGHT = 60;

//
// The Desktop class encapsulates our desktop environment.
// It handles setting up the taskbar, buttons, wallpaper, and events.
//
class Desktop {
public:
    Desktop();
    ~Desktop();
    bool init();
    void run();
    void cleanup();
    
private:
    xcb_connection_t* conn;
    xcb_screen_t* screen;
    xcb_window_t root, taskbar;
    xcb_window_t app_button, terminal_button, settings_button, notif_button,
                 theme_button, about_button, logout_button, clock_win;
    xcb_window_t app_menu, settings_win, volume_win;
    xcb_gcontext_t gc;

    // Configuration values – default wallpaper and theme color (as used in the clock drawing)
    std::string wallpaperPath;
    uint32_t themeColor;

    // AppEntry for holding an app’s name, desktop file path, and its y coordinate in the menu.
    struct AppEntry {
        std::string name;
        std::string path;
        int y;
    };
    std::vector<AppEntry> appEntries;

    // Methods
    void setupCursor();
    void loadConfig();
    void setWallpaper();
    void drawText(xcb_window_t win, int x, int y, const std::string &txt, uint32_t color);
    void launchApp(const std::string &desktopFile);
    void launchTerminal();
    void showAppMenu();
    void handleAppMenuClick(int click_y);
    void showSettings();
    void showVolume();
    void changeVolume(const std::string &cmd);
    void grabKeys();
    void drawClock();
    void createTaskbar();
    void processEvent(xcb_generic_event_t* e);
    std::string getTimeString();
};

//
// Constructor: set defaults for our configuration and initialize members.
//
Desktop::Desktop() 
    : conn(nullptr), screen(nullptr), root(0), taskbar(0),
      app_button(0), terminal_button(0), settings_button(0), notif_button(0),
      theme_button(0), about_button(0), logout_button(0), clock_win(0),
      app_menu(0), settings_win(0), volume_win(0), gc(0),
      wallpaperPath("file:///usr/share/backgrounds/default.jpg"),
      themeColor(0x333333) // initial theme color for backgrounds
{}

//
// Destructor – ensure cleanup is called.
//
Desktop::~Desktop() {
    cleanup();
}

//
// init() connects to X, loads config values, creates the taskbar with several buttons,
// sets up the cursor and wallpaper, and grabs the key events we need.
//
bool Desktop::init() {
    conn = xcb_connect(nullptr, nullptr);
    if (xcb_connection_has_error(conn)) {
        std::cerr << "Cannot connect to X server" << std::endl;
        return false;
    }
    screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    root = screen->root;

    loadConfig();
    createTaskbar();
    setupCursor();
    setWallpaper();
    grabKeys();

    return true;
}

//
// loadConfig() attempts to open ~/.config/mydesktop.conf and parse key=value pairs.
// For example:
//   wallpaper=/my/new/wallpaper.jpg
//   themeColor=0x444444
//
void Desktop::loadConfig() {
    const char* home = getenv("HOME");
    if (!home)
        return;
    std::string configPath = std::string(home) + "/.config/mydesktop.conf";
    std::ifstream infile(configPath);
    if (infile.is_open()) {
        std::string line;
        while (getline(infile, line)) {
            size_t pos = line.find('=');
            if (pos == std::string::npos)
                continue;
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            if (key == "wallpaper") {
                wallpaperPath = "file://" + value;
            } else if (key == "themeColor") {
                themeColor = std::stoul(value, nullptr, 16);
            }
        }
        infile.close();
    }
}

//
// setupCursor() creates and sets a left-pointer cursor for the root window.
//
void Desktop::setupCursor() {
    xcb_cursor_t cursor = xcb_generate_id(conn);
    xcb_create_glyph_cursor(conn, cursor, screen->default_colormap,
                            screen->default_colormap, XC_left_ptr, 0,
                            0, 0, 0, 0xFFFF, 0xFFFF, 0xFFFF);
    xcb_change_window_attributes(conn, root, XCB_CW_CURSOR, &cursor);
    xcb_free_cursor(conn, cursor);
    xcb_flush(conn);
}

//
// setWallpaper() uses GSettings to set the desktop background.
// (You can later extend this to support themes dynamically.)
//
void Desktop::setWallpaper() {
    GSettings *settings = g_settings_new("org.gnome.desktop.background");
    g_settings_set_string(settings, "picture-uri", wallpaperPath.c_str());
    g_object_unref(settings);
}

//
// drawText() is a thin wrapper around xcb_image_text_8 so that we can use C++ strings.
//
void Desktop::drawText(xcb_window_t win, int x, int y, const std::string &txt, uint32_t color) {
    xcb_change_gc(conn, gc, XCB_GC_FOREGROUND, &color);
    xcb_image_text_8(conn, txt.size(), win, gc, x, y, txt.c_str());
    xcb_flush(conn);
}

//
// launchApp() takes a desktop file path, loads it with GDesktopAppInfo, and launches the application.
//
void Desktop::launchApp(const std::string &desktopFile) {
    GDesktopAppInfo *app = g_desktop_app_info_new_from_filename(desktopFile.c_str());
    if (app) {
        g_app_info_launch(G_APP_INFO(app), nullptr, nullptr, nullptr);
        g_object_unref(app);
    }
}

//
// launchTerminal() simply forks and launches "xterm".
// You might later add custom terminal support here.
//
void Desktop::launchTerminal() {
    if (fork() == 0) {
        execlp("xterm", "xterm", nullptr);
        exit(1);
    }
}

//
// showAppMenu() creates (or remaps) a window containing the list of installed applications
// (by scanning XDG applications directories). Each entry is drawn and saved in an STL vector.
//
void Desktop::showAppMenu() {
    if (app_menu) {
        xcb_map_window(conn, app_menu);
        xcb_flush(conn);
        return;
    }

    uint32_t values[] = {0x222222, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS};
    int menu_x = 100, menu_y = 100;
    app_menu = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, app_menu, root,
                      menu_x, menu_y, APP_MENU_WIDTH, APP_MENU_HEIGHT,
                      2, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, values);
    xcb_map_window(conn, app_menu);

    appEntries.clear();
    int y_offset = 20;
    const char *xdg_data_dirs = getenv("XDG_DATA_DIRS");
    if (!xdg_data_dirs)
        xdg_data_dirs = "/usr/share:/usr/local/share";
    std::string dirsString(xdg_data_dirs);
    std::istringstream iss(dirsString);
    std::string token;
    while (std::getline(iss, token, ':') && y_offset < APP_MENU_HEIGHT - 20) {
        std::string path = token + "/applications";
        DIR *d = opendir(path.c_str());
        if (d) {
            struct dirent *entry;
            while ((entry = readdir(d)) && y_offset < APP_MENU_HEIGHT - 20) {
                std::string entryName(entry->d_name);
                if (entryName.find(".desktop") != std::string::npos) {
                    std::string full_path = path + "/" + entryName;
                    GDesktopAppInfo *app = g_desktop_app_info_new_from_filename(full_path.c_str());
                    if (app) {
                        const char *name = g_app_info_get_name(G_APP_INFO(app));
                        drawText(app_menu, 10, y_offset, name, 0xFFFFFF);
                        appEntries.push_back({ name, full_path, y_offset });
                        y_offset += 20;
                        g_object_unref(app);
                    }
                }
            }
            closedir(d);
        }
    }
    xcb_flush(conn);
}

//
// When a click occurs within the app menu, handleAppMenuClick()
// decides which application was selected by comparing the click y‑coordinate.
//
void Desktop::handleAppMenuClick(int click_y) {
    for (const auto &entry : appEntries) {
        int entry_top = entry.y - 15;
        int entry_bottom = entry.y + 5;
        if (click_y >= entry_top && click_y <= entry_bottom) {
            launchApp(entry.path);
            xcb_unmap_window(conn, app_menu);
            xcb_flush(conn);
            break;
        }
    }
}

//
// showSettings() creates a basic settings window (currently a stub).
//
void Desktop::showSettings() {
    if (settings_win) {
        xcb_map_window(conn, settings_win);
        xcb_flush(conn);
        return;
    }

    uint32_t values[] = {0x444444, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS};
    settings_win = xcb_generate_id(conn);
    int win_x = 200, win_y = 200;
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, settings_win, root,
                      win_x, win_y, SETTINGS_WIDTH, SETTINGS_HEIGHT,
                      2, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, values);
    xcb_map_window(conn, settings_win);
    drawText(settings_win, 10, 20, "Settings (Coming Soon)", 0xFFFFFF);
    xcb_flush(conn);
}

//
// showVolume() creates a small window showing volume info (with the suggestion to use keys).
//
void Desktop::showVolume() {
    if (volume_win) {
        xcb_map_window(conn, volume_win);
        xcb_flush(conn);
        return;
    }

    uint32_t values[] = {0x333355, XCB_EVENT_MASK_EXPOSURE};
    volume_win = xcb_generate_id(conn);
    int win_x = 250, win_y = 150;
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, volume_win, root,
                      win_x, win_y, VOL_WIDTH, VOL_HEIGHT,
                      2, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, values);
    xcb_map_window(conn, volume_win);
    drawText(volume_win, 10, 20, "Volume: Use keys", 0xFFFFFF);
    xcb_flush(conn);
}

//
// changeVolume() forks and calls a shell command to adjust the system volume.
//
void Desktop::changeVolume(const std::string &cmd) {
    if (fork() == 0) {
        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        exit(1);
    }
}

//
// grabKeys() grabs some global key events (mod key and volume keys).
//
void Desktop::grabKeys() {
    xcb_grab_key(conn, 1, root, XCB_MOD_MASK_4, XCB_NO_SYMBOL,
                 XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_key(conn, 1, root, XCB_MOD_MASK_ANY, 0x1008FF11,
                 XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC); // Vol Down
    xcb_grab_key(conn, 1, root, XCB_MOD_MASK_ANY, 0x1008FF13,
                 XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC); // Vol Up
    xcb_grab_key(conn, 1, root, XCB_MOD_MASK_ANY, 0x1008FF12,
                 XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC); // Mute
    xcb_flush(conn);
}

//
// getTimeString() returns the current time as a string in HH:MM:SS format.
//
std::string Desktop::getTimeString() {
    time_t t = time(nullptr);
    struct tm *tm = localtime(&t);
    char buf[64];
    strftime(buf, sizeof(buf), "%H:%M:%S", tm);
    return std::string(buf);
}

//
// drawClock() clears the clock window and redraws the current time.
//
void Desktop::drawClock() {
    std::string timeStr = getTimeString();
    uint32_t color = 0x333333;
    xcb_change_gc(conn, gc, XCB_GC_FOREGROUND, &color);
    xcb_rectangle_t rect = {0, 0, (uint16_t)CLOCK_WIDTH, 30};
    xcb_poly_fill_rectangle(conn, clock_win, gc, 1, &rect);
    drawText(clock_win, 10, 20, timeStr, 0xFFFFFF);
    xcb_flush(conn);
}

//
// createTaskbar() sets up the taskbar window and creates individual buttons:
//
// - Apps, Terminal, Settings, Volume, Theme, About, and Logout.
// - The clock window is placed at the far right.
//
void Desktop::createTaskbar() {
    int width = static_cast<int>(screen->width_in_pixels * 0.8);
    int x = (screen->width_in_pixels - width) / 2;
    int y = screen->height_in_pixels - HEIGHT - 10;

    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_OVERRIDE_REDIRECT;
    uint32_t values[] = {0x333333, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS, 1};

    taskbar = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, taskbar, root,
                      x, y, width, HEIGHT, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      mask, values);
    xcb_map_window(conn, taskbar);

    int margin = 10;
    int current_x = 10;

    // "Apps" button
    values[0] = 0x555555;
    app_button = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, app_button, taskbar,
                      current_x, 5, BUTTON_WIDTH, BUTTON_HEIGHT, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      mask, values);
    xcb_map_window(conn, app_button);
    current_x += BUTTON_WIDTH + margin;

    // "Terminal" button
    terminal_button = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, terminal_button, taskbar,
                      current_x, 5, BUTTON_WIDTH, BUTTON_HEIGHT, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      mask, values);
    xcb_map_window(conn, terminal_button);
    current_x += BUTTON_WIDTH + margin;

    // "Settings" button
    settings_button = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, settings_button, taskbar,
                      current_x, 5, BUTTON_WIDTH, BUTTON_HEIGHT, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      mask, values);
    xcb_map_window(conn, settings_button);
    current_x += BUTTON_WIDTH + margin;

    // "Volume" button
    notif_button = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, notif_button, taskbar,
                      current_x, 5, BUTTON_WIDTH, BUTTON_HEIGHT, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      mask, values);
    xcb_map_window(conn, notif_button);
    current_x += BUTTON_WIDTH + margin;

    // "Theme" button to toggle the theme color
    theme_button = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, theme_button, taskbar,
                      current_x, 5, BUTTON_WIDTH, BUTTON_HEIGHT, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      mask, values);
    xcb_map_window(conn, theme_button);
    current_x += BUTTON_WIDTH + margin;

    // "About" button to display app info
    about_button = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, about_button, taskbar,
                      current_x, 5, BUTTON_WIDTH, BUTTON_HEIGHT, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      mask, values);
    xcb_map_window(conn, about_button);
    current_x += BUTTON_WIDTH + margin;

    // "Logout" button
    logout_button = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, logout_button, taskbar,
                      current_x, 5, BUTTON_WIDTH, BUTTON_HEIGHT, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      mask, values);
    xcb_map_window(conn, logout_button);

    // Clock window positioned at the far right
    values[0] = 0x333333;
    values[1] = XCB_EVENT_MASK_EXPOSURE;
    clock_win = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, clock_win, taskbar,
                      width - CLOCK_WIDTH - 10, 5, CLOCK_WIDTH, 30, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      mask, values);
    xcb_map_window(conn, clock_win);

    gc = xcb_generate_id(conn);
    xcb_create_gc(conn, gc, taskbar, 0, nullptr);
    xcb_flush(conn);
}

//
// processEvent() handles the XCB events. Depending on the event type, it calls
// various helper methods. (It also shows our new “Theme” and “About” functionality.)
//
void Desktop::processEvent(xcb_generic_event_t* e) {
    switch (e->response_type & ~0x80) {
        case XCB_KEY_PRESS: {
            auto* ke = reinterpret_cast<xcb_key_press_event_t*>(e);
            if (ke->detail == XCB_NO_SYMBOL && (ke->state & XCB_MOD_MASK_4)) {
                showAppMenu();
            } else if (ke->detail == 0x1008FF13) {
                changeVolume("pactl set-sink-volume @DEFAULT_SINK@ +5%");
            } else if (ke->detail == 0x1008FF11) {
                changeVolume("pactl set-sink-volume @DEFAULT_SINK@ -5%");
            } else if (ke->detail == 0x1008FF12) {
                changeVolume("pactl set-sink-mute @DEFAULT_SINK@ toggle");
            }
            break;
        }
        case XCB_EXPOSE: {
            auto* ee = reinterpret_cast<xcb_expose_event_t*>(e);
            if (ee->window == app_button)
                drawText(app_button, 10, 20, "Apps", 0xFFFFFF);
            else if (ee->window == terminal_button)
                drawText(terminal_button, 5, 20, "Term", 0xFFFFFF);
            else if (ee->window == settings_button)
                drawText(settings_button, 5, 20, "Set", 0xFFFFFF);
            else if (ee->window == notif_button)
                drawText(notif_button, 5, 20, "Vol", 0xFFFFFF);
            else if (ee->window == theme_button)
                drawText(theme_button, 5, 20, "Theme", 0xFFFFFF);
            else if (ee->window == about_button)
                drawText(about_button, 5, 20, "About", 0xFFFFFF);
            else if (ee->window == logout_button)
                drawText(logout_button, 5, 20, "Logout", 0xFFFFFF);
            else if (ee->window == clock_win)
                drawClock();
            break;
        }
        case XCB_BUTTON_PRESS: {
            auto* be = reinterpret_cast<xcb_button_press_event_t*>(e);
            if (be->event == app_button)
                showAppMenu();
            else if (be->event == terminal_button)
                launchTerminal();
            else if (be->event == settings_button)
                showSettings();
            else if (be->event == notif_button)
                showVolume();
            else if (be->event == theme_button) {
                // Toggle the theme color value for demonstration.
                themeColor = (themeColor == 0x333333) ? 0x444444 : 0x333333;
                drawClock();
            } else if (be->event == about_button) {
                // Create a simple "About" window.
                if (settings_win)
                    xcb_destroy_window(conn, settings_win);
                settings_win = 0;
                uint32_t values[] = {0x444444, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS};
                settings_win = xcb_generate_id(conn);
                int win_x = 300, win_y = 300;
                xcb_create_window(conn, XCB_COPY_FROM_PARENT, settings_win, root,
                                  win_x, win_y, SETTINGS_WIDTH, SETTINGS_HEIGHT,
                                  2, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                                  XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, values);
                xcb_map_window(conn, settings_win);
                drawText(settings_win, 10, 20, "Enhanced Desktop v1.0\nCreated in C++", 0xFFFFFF);
                xcb_flush(conn);
            } else if (be->event == logout_button) {
                cleanup();
                exit(0);
            } else if (be->event == app_menu) {
                handleAppMenuClick(be->event_y);
            }
            break;
        }
    }
}

//
// run() enters the main event loop. After each event, it updates the clock.
// A short sleep is introduced to reduce CPU usage.
//
void Desktop::run() {
    xcb_generic_event_t* e;
    while ((e = xcb_wait_for_event(conn))) {
        processEvent(e);
        free(e);
        drawClock();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

//
// cleanup() destroys all the windows, frees the graphics context,
// and disconnects the XCB connection.
//
void Desktop::cleanup() {
    if (app_menu) {
        xcb_destroy_window(conn, app_menu);
        app_menu = 0;
    }
    if (settings_win) {
        xcb_destroy_window(conn, settings_win);
        settings_win = 0;
    }
    if (volume_win) {
        xcb_destroy_window(conn, volume_win);
        volume_win = 0;
    }
    if (taskbar)
        xcb_destroy_window(conn, taskbar);
    if (app_button)
        xcb_destroy_window(conn, app_button);
    if (terminal_button)
        xcb_destroy_window(conn, terminal_button);
    if (settings_button)
        xcb_destroy_window(conn, settings_button);
    if (notif_button)
        xcb_destroy_window(conn, notif_button);
    if (theme_button)
        xcb_destroy_window(conn, theme_button);
    if (about_button)
        xcb_destroy_window(conn, about_button);
    if (logout_button)
        xcb_destroy_window(conn, logout_button);
    if (clock_win)
        xcb_destroy_window(conn, clock_win);
    if (gc)
        xcb_free_gc(conn, gc);
    if (conn)
        xcb_disconnect(conn);
}

//
// main() creates a Desktop, initializes it and enters the event loop.
//
int main() {
    Desktop desktop;
    if (!desktop.init()) {
        return 1;
    }
    desktop.run();
    return 0;
}
