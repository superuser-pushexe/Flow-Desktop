// flow.c - Enhanced lightweight X11 desktop environment using XCB

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <gio/gio.h>
#include <sys/stat.h>

#define WIDTH_RATIO 0.8
#define HEIGHT 40
#define CLOCK_WIDTH 120
#define BUTTON_WIDTH 80
#define BUTTON_HEIGHT 30
#define APP_MENU_HEIGHT 400
#define APP_MENU_WIDTH 300
#define SETTINGS_WIDTH 300
#define SETTINGS_HEIGHT 200
#define VOL_WIDTH 200
#define VOL_HEIGHT 60

// Global connection and window variables
xcb_connection_t *conn;
xcb_screen_t *screen;
xcb_window_t root, taskbar, app_button, terminal_button, settings_button, notif_button, logout_button, clock_win;
xcb_window_t app_menu = 0, settings_win = 0, volume_win = 0;
xcb_gcontext_t gc;

// Structure for storing app entries in the app menu
typedef struct {
    char name[256];
    char path[512];
    int y; // vertical coordinate where this app’s name is drawn
} AppEntry;

AppEntry *app_entries = NULL;
int app_entry_count = 0;

void setup_cursor() {
    xcb_cursor_t cursor = xcb_generate_id(conn);
    xcb_create_glyph_cursor(conn, cursor, screen->default_colormap,
                           screen->default_colormap, XC_left_ptr, 0, 0, 0, 0xFFFF, 0xFFFF, 0xFFFF);
    xcb_change_window_attributes(conn, root, XCB_CW_CURSOR, &cursor);
    xcb_free_cursor(conn, cursor);
    xcb_flush(conn);
}

void set_wallpaper() {
    GSettings *settings = g_settings_new("org.gnome.desktop.background");
    g_settings_set_string(settings, "picture-uri", "file:///usr/share/backgrounds/default.jpg");
    g_object_unref(settings);
}

void draw_text(xcb_window_t win, int x, int y, const char *txt, uint32_t color) {
    xcb_change_gc(conn, gc, XCB_GC_FOREGROUND, &color);
    xcb_image_text_8(conn, strlen(txt), win, gc, x, y, txt);
    xcb_flush(conn);
}

void launch_app(const char *desktop_file) {
    GDesktopAppInfo *app = g_desktop_app_info_new_from_filename(desktop_file);
    if (app) {
        g_app_info_launch(G_APP_INFO(app), NULL, NULL, NULL);
        g_object_unref(app);
    }
}

// New helper: launch a terminal emulator (using "xterm" as an example)
void launch_terminal() {
    if (fork() == 0) {
         execlp("xterm", "xterm", NULL);
         exit(1);
    }
}

// Improved App Menu: build a list of apps and draw them so clicks can be handled.
void show_app_menu() {
    if (app_menu) {
        xcb_map_window(conn, app_menu);
        xcb_flush(conn);
        return;
    }

    uint32_t values[] = {0x222222, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS};
    int menu_x = 100, menu_y = 100; // fixed position for the menu
    app_menu = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, app_menu, root,
                      menu_x, menu_y, APP_MENU_WIDTH, APP_MENU_HEIGHT,
                      2, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, values);
    xcb_map_window(conn, app_menu);

    // Clear any previous app entries.
    if (app_entries) {
        free(app_entries);
        app_entries = NULL;
        app_entry_count = 0;
    }

    // Fetch directories from XDG_DATA_DIRS.
    const char *xdg_data_dirs = getenv("XDG_DATA_DIRS");
    if (!xdg_data_dirs) xdg_data_dirs = "/usr/share:/usr/local/share";
    char *dirs = strdup(xdg_data_dirs);
    char *dir = strtok(dirs, ":");
    int y_offset = 20;
    int capacity = 50;
    app_entries = malloc(sizeof(AppEntry) * capacity);
    if (!app_entries) {
         fprintf(stderr, "Memory allocation failed\n");
         free(dirs);
         return;
    }

    while (dir && y_offset < APP_MENU_HEIGHT - 20) {
         char path[512];
         snprintf(path, sizeof(path), "%s/applications", dir);
         DIR *d = opendir(path);
         if (d) {
              struct dirent *entry;
              while ((entry = readdir(d)) && y_offset < APP_MENU_HEIGHT - 20) {
                  if (strstr(entry->d_name, ".desktop")) {
                       char full_path[512];
                       snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
                       GDesktopAppInfo *app = g_desktop_app_info_new_from_filename(full_path);
                       if (app) {
                           const char *name = g_app_info_get_name(G_APP_INFO(app));
                           draw_text(app_menu, 10, y_offset, name, 0xFFFFFF);
                           // Save the app entry into our list.
                           if (app_entry_count >= capacity) {
                                capacity *= 2;
                                app_entries = realloc(app_entries, sizeof(AppEntry) * capacity);
                           }
                           strncpy(app_entries[app_entry_count].name, name, sizeof(app_entries[app_entry_count].name) - 1);
                           strncpy(app_entries[app_entry_count].path, full_path, sizeof(app_entries[app_entry_count].path) - 1);
                           app_entries[app_entry_count].y = y_offset;
                           app_entry_count++;
                           y_offset += 20;
                           g_object_unref(app);
                       }
                  }
              }
              closedir(d);
         }
         dir = strtok(NULL, ":");
    }
    free(dirs);
    xcb_flush(conn);
}

// New helper: check where the user clicked in the app menu and launch the appropriate app.
void handle_app_menu_click(int click_y) {
    for (int i = 0; i < app_entry_count; i++) {
         // Assume each entry roughly occupies a 20px-high slot.
         int entry_top = app_entries[i].y - 15;
         int entry_bottom = app_entries[i].y + 5;
         if (click_y >= entry_top && click_y <= entry_bottom) {
              launch_app(app_entries[i].path);
              // Hide app menu after launching.
              xcb_unmap_window(conn, app_menu);
              xcb_flush(conn);
              break;
         }
    }
}

void show_settings() {
    if (settings_win) {
        xcb_map_window(conn, settings_win);
        xcb_flush(conn);
        return;
    }

    uint32_t values[] = {0x444444, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS};
    settings_win = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, settings_win, root, 200, 200,
                      SETTINGS_WIDTH, SETTINGS_HEIGHT, 2, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, values);
    xcb_map_window(conn, settings_win);
    draw_text(settings_win, 10, 20, "Settings (Coming Soon)", 0xFFFFFF);
    xcb_flush(conn);
}

void show_volume() {
    if (volume_win) {
        xcb_map_window(conn, volume_win);
        xcb_flush(conn);
        return;
    }

    uint32_t values[] = {0x333355, XCB_EVENT_MASK_EXPOSURE};
    volume_win = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, volume_win, root, 250, 150,
                      VOL_WIDTH, VOL_HEIGHT, 2, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, values);
    xcb_map_window(conn, volume_win);
    draw_text(volume_win, 10, 20, "Volume: use keys", 0xFFFFFF);
    xcb_flush(conn);
}

void change_volume(const char *cmd) {
    if (fork() == 0) {
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        exit(1);
    }
}

void grab_keys() {
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

void draw_clock() {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char buf[64];
    strftime(buf, sizeof(buf), "%H:%M:%S", tm);

    uint32_t color = 0x333333;
    xcb_change_gc(conn, gc, XCB_GC_FOREGROUND, &color);
    // Clear the clock window background.
    xcb_poly_fill_rectangle(conn, clock_win, gc, 1,
                            &(xcb_rectangle_t){0, 0, CLOCK_WIDTH, 30});
    draw_text(clock_win, 10, 20, buf, 0xFFFFFF);
    xcb_flush(conn);
}

void create_taskbar() {
    int width = screen->width_in_pixels * WIDTH_RATIO;
    int x = (screen->width_in_pixels - width) / 2;
    int y = screen->height_in_pixels - HEIGHT - 10;

    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_OVERRIDE_REDIRECT;
    uint32_t values[] = {0x333333, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS, 1};

    taskbar = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, taskbar, root,
                      x, y, width, HEIGHT, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, mask, values);
    xcb_map_window(conn, taskbar);

    // Create several buttons with a little margin between them.
    int margin = 10;
    int current_x = 10;

    // "Apps" button.
    values[0] = 0x555555;
    app_button = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, app_button, taskbar,
                      current_x, 5, BUTTON_WIDTH, BUTTON_HEIGHT, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);
    xcb_map_window(conn, app_button);
    current_x += BUTTON_WIDTH + margin;

    // New "Terminal" button.
    terminal_button = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, terminal_button, taskbar,
                      current_x, 5, BUTTON_WIDTH, BUTTON_HEIGHT, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);
    xcb_map_window(conn, terminal_button);
    current_x += BUTTON_WIDTH + margin;

    // New "Settings" button.
    settings_button = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, settings_button, taskbar,
                      current_x, 5, BUTTON_WIDTH, BUTTON_HEIGHT, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);
    xcb_map_window(conn, settings_button);
    current_x += BUTTON_WIDTH + margin;

    // Reuse "Notify" button for volume control.
    notif_button = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, notif_button, taskbar,
                      current_x, 5, BUTTON_WIDTH, BUTTON_HEIGHT, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);
    xcb_map_window(conn, notif_button);
    current_x += BUTTON_WIDTH + margin;

    // "Logout" button.
    logout_button = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, logout_button, taskbar,
                      current_x, 5, BUTTON_WIDTH, BUTTON_HEIGHT, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);
    xcb_map_window(conn, logout_button);

    // Clock window positioned at the far right.
    values[0] = 0x333333;
    values[1] = XCB_EVENT_MASK_EXPOSURE;
    clock_win = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, clock_win, taskbar,
                      width - CLOCK_WIDTH - 10, 5, CLOCK_WIDTH, 30, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);
    xcb_map_window(conn, clock_win);

    gc = xcb_generate_id(conn);
    xcb_create_gc(conn, gc, taskbar, 0, NULL);
    xcb_flush(conn);
}

void cleanup() {
    if (app_menu) {
         xcb_destroy_window(conn, app_menu);
         app_menu = 0;
    }
    if (settings_win) xcb_destroy_window(conn, settings_win);
    if (volume_win) xcb_destroy_window(conn, volume_win);
    xcb_destroy_window(conn, taskbar);
    xcb_destroy_window(conn, app_button);
    xcb_destroy_window(conn, terminal_button);
    xcb_destroy_window(conn, settings_button);
    xcb_destroy_window(conn, notif_button);
    xcb_destroy_window(conn, logout_button);
    xcb_destroy_window(conn, clock_win);
    xcb_free_gc(conn, gc);
    if(app_entries) {
         free(app_entries);
         app_entries = NULL;
         app_entry_count = 0;
    }
    xcb_disconnect(conn);
}

int main() {
    conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn)) {
        fprintf(stderr, "Cannot connect to X server\n");
        return 1;
    }

    screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    root = screen->root;

    create_taskbar();
    setup_cursor();
    set_wallpaper();
    grab_keys();

    xcb_generic_event_t *e;
    while ((e = xcb_wait_for_event(conn))) {
        switch (e->response_type & ~0x80) {
        case XCB_KEY_PRESS: {
            xcb_key_press_event_t *ke = (xcb_key_press_event_t *)e;
            if (ke->detail == XCB_NO_SYMBOL && (ke->state & XCB_MOD_MASK_4)) {
                show_app_menu();
            } else if (ke->detail == 0x1008FF13) {
                change_volume("pactl set-sink-volume @DEFAULT_SINK@ +5%");
            } else if (ke->detail == 0x1008FF11) {
                change_volume("pactl set-sink-volume @DEFAULT_SINK@ -5%");
            } else if (ke->detail == 0x1008FF12) {
                change_volume("pactl set-sink-mute @DEFAULT_SINK@ toggle");
            }
            break;
        }
        case XCB_EXPOSE: {
            xcb_expose_event_t *ee = (xcb_expose_event_t *)e;
            if (ee->window == app_button)
                draw_text(app_button, 10, 20, "Apps", 0xFFFFFF);
            else if (ee->window == terminal_button)
                draw_text(terminal_button, 5, 20, "Term", 0xFFFFFF);
            else if (ee->window == settings_button)
                draw_text(settings_button, 5, 20, "Settings", 0xFFFFFF);
            else if (ee->window == notif_button)
                draw_text(notif_button, 5, 20, "Vol", 0xFFFFFF);
            else if (ee->window == logout_button)
                draw_text(logout_button, 5, 20, "Logout", 0xFFFFFF);
            else if (ee->window == clock_win)
                draw_clock();
            break;
        }
        case XCB_BUTTON_PRESS: {
            xcb_button_press_event_t *be = (xcb_button_press_event_t *)e;
            if (be->event == app_button)
                show_app_menu();
            else if (be->event == terminal_button)
                launch_terminal();
            else if (be->event == settings_button)
                show_settings();
            else if (be->event == notif_button)
                show_volume();
            else if (be->event == logout_button) {
                cleanup();
                exit(0);
            }
            else if (be->event == app_menu) {
                // When clicking in the app menu, check the y-coordinate to determine selection.
                handle_app_menu_click(be->event_y);
            }
            break;
        }
        }
        free(e);
        draw_clock();
    }

    cleanup();
    return 0;
}
