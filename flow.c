// flow.c - Lightweight X11 desktop environment using XCB

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

xcb_connection_t *conn;
xcb_screen_t *screen;
xcb_window_t root, taskbar, app_button, notif_button, logout_button, clock_win;
xcb_window_t app_menu = 0, settings_win = 0, volume_win = 0;
xcb_gcontext_t gc;

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

void show_app_menu() {
    if (app_menu) {
        xcb_map_window(conn, app_menu);
        xcb_flush(conn);
        return;
    }

    uint32_t values[] = {0x222222, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS};
    app_menu = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, app_menu, root, 100, 100, APP_MENU_WIDTH, APP_MENU_HEIGHT,
                      2, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, values);
    xcb_map_window(conn, app_menu);

    const char *xdg_data_dirs = getenv("XDG_DATA_DIRS");
    if (!xdg_data_dirs) xdg_data_dirs = "/usr/share:/usr/local/share";
    char *dirs = strdup(xdg_data_dirs);
    char *dir = strtok(dirs, ":");
    int y = 20;

    while (dir && y < APP_MENU_HEIGHT) {
        char path[512];
        snprintf(path, sizeof(path), "%s/applications", dir);
        DIR *d = opendir(path);
        if (d) {
            struct dirent *entry;
            while ((entry = readdir(d)) && y < APP_MENU_HEIGHT) {
                if (strstr(entry->d_name, ".desktop")) {
                    char full_path[512];
                    snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
                    GDesktopAppInfo *app = g_desktop_app_info_new_from_filename(full_path);
                    if (app) {
                        const char *name = g_app_info_get_name(G_APP_INFO(app));
                        draw_text(app_menu, 10, y, name, 0xFFFFFF);
                        y += 20;
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

void show_settings() {
    if (settings_win) {
        xcb_map_window(conn, settings_win);
        xcb_flush(conn);
        return;
    }

    uint32_t values[] = {0x444444, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS};
    settings_win = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, settings_win, root, 200, 200, SETTINGS_WIDTH, SETTINGS_HEIGHT,
                      2, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, values);
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
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, volume_win, root, 250, 150, VOL_WIDTH, VOL_HEIGHT,
                      2, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, values);
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
    xcb_grab_key(conn, 1, root, XCB_MOD_MASK_4, XCB_NO_SYMBOL, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_key(conn, 1, root, XCB_MOD_MASK_ANY, 0x1008FF11, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC); // Vol Down
    xcb_grab_key(conn, 1, root, XCB_MOD_MASK_ANY, 0x1008FF13, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC); // Vol Up
    xcb_grab_key(conn, 1, root, XCB_MOD_MASK_ANY, 0x1008FF12, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC); // Mute
    xcb_flush(conn);
}

void draw_clock() {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char buf[64];
    strftime(buf, sizeof(buf), "%H:%M:%S", tm);

    uint32_t color = 0x333333;
    xcb_change_gc(conn, gc, XCB_GC_FOREGROUND, &color);
    xcb_poly_fill_rectangle(conn, clock_win, gc, 1, &(xcb_rectangle_t){0, 0, CLOCK_WIDTH, 30});
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
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, taskbar, root, x, y, width, HEIGHT, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);
    xcb_map_window(conn, taskbar);

    // Create buttons and clock
    values[0] = 0x555555;
    values[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS;

    app_button = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, app_button, taskbar, 10, 5, BUTTON_WIDTH, BUTTON_HEIGHT,
                      0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);
    xcb_map_window(conn, app_button);

    notif_button = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, notif_button, taskbar, 100, 5, BUTTON_WIDTH, BUTTON_HEIGHT,
                      0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);
    xcb_map_window(conn, notif_button);

    logout_button = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, logout_button, taskbar, 190, 5, BUTTON_WIDTH, BUTTON_HEIGHT,
                      0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);
    xcb_map_window(conn, logout_button);

    values[0] = 0x333333;
    values[1] = XCB_EVENT_MASK_EXPOSURE;
    clock_win = xcb_generate_id(conn);
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, clock_win, taskbar, width - CLOCK_WIDTH - 10, 5, CLOCK_WIDTH, 30,
                      0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);
    xcb_map_window(conn, clock_win);

    gc = xcb_generate_id(conn);
    xcb_create_gc(conn, gc, taskbar, 0, NULL);
    xcb_flush(conn);
}

void cleanup() {
    if (app_menu) xcb_destroy_window(conn, app_menu);
    if (settings_win) xcb_destroy_window(conn, settings_win);
    if (volume_win) xcb_destroy_window(conn, volume_win);
    xcb_destroy_window(conn, taskbar);
    xcb_destroy_window(conn, app_button);
    xcb_destroy_window(conn, notif_button);
    xcb_destroy_window(conn, logout_button);
    xcb_destroy_window(conn, clock_win);
    xcb_free_gc(conn, gc);
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
            if (ee->window == app_button) draw_text(app_button, 10, 20, "Apps", 0xFFFFFF);
            else if (ee->window == notif_button) draw_text(notif_button, 5, 20, "Notify", 0xFFFFFF);
            else if (ee->window == logout_button) draw_text(logout_button, 5, 20, "Logout", 0xFFFFFF);
            else if (ee->window == clock_win) draw_clock();
            break;
        }
        case XCB_BUTTON_PRESS: {
            xcb_button_press_event_t *be = (xcb_button_press_event_t *)e;
            if (be->event == app_button) show_app_menu();
            else if (be->event == notif_button || be->event == volume_button) show_volume();
            else if (be->event == logout_button) {
                cleanup();
                exit(0);
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
