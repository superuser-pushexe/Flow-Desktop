#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#define APP_MENU_WIDTH 300
#define APP_MENU_HEIGHT 400
#define SETTINGS_WIDTH 300
#define SETTINGS_HEIGHT 200
#define VOLUME_WIN_WIDTH 200
#define VOLUME_WIN_HEIGHT 100

extern Display *display;
extern Window root;
extern int screen;
extern Window taskbar_win, app_menu_button, notification_button, logout_button, clock_win, app_menu_win;
extern GC gc;
extern Window settings_button, settings_win, volume_button, volume_win;

void draw_text(Window win, int x, int y, const char* text, unsigned long color) {
    XSetForeground(display, gc, color);
    XDrawImageString(display, win, gc, x, y, text, strlen(text));
}

void show_app_menu(void) {
    if (app_menu_win) {
        XMapRaised(display, app_menu_win);
        return;
    }
    app_menu_win = XCreateSimpleWindow(display, root, 100, 100, APP_MENU_WIDTH, APP_MENU_HEIGHT, 2,
                                       BlackPixel(display, screen), 0x202020);
    XSelectInput(display, app_menu_win, ExposureMask | ButtonPressMask);
    XMapWindow(display, app_menu_win);

    DIR *d;
    struct dirent *dir;
    int y_offset = 20;
    d = opendir("/usr/share/applications");
    if (d) {
        while ((dir = readdir(d)) != NULL && y_offset < APP_MENU_HEIGHT) {
            if (strstr(dir->d_name, ".desktop")) {
                char name[256];
                strncpy(name, dir->d_name, sizeof(name));
                name[strlen(name) - 8] = '\0';
                draw_text(app_menu_win, 10, y_offset, name, 0xFFFFFF);
                y_offset += 20;
            }
        }
        closedir(d);
    }
}

void show_settings(void) {
    if (settings_win) {
        XMapRaised(display, settings_win);
        return;
    }
    settings_win = XCreateSimpleWindow(display, root, 150, 150, SETTINGS_WIDTH, SETTINGS_HEIGHT, 2,
                                       BlackPixel(display, screen), 0x444444);
    XSelectInput(display, settings_win, ExposureMask | ButtonPressMask);
    XMapWindow(display, settings_win);
    draw_text(settings_win, 10, 20, "Settings Menu (placeholder)", 0xFFFFFF);
}

void show_notifications(void) {
    Window notif_win = XCreateSimpleWindow(display, root, 150, 150, 200, 100, 2,
                                           BlackPixel(display, screen), 0x444466);
    XSelectInput(display, notif_win, ExposureMask | ButtonPressMask);
    XMapWindow(display, notif_win);
    draw_text(notif_win, 10, 20, "New Notification", 0xFFFFFF);
}

void show_volume(void) {
    if (volume_win) {
        XMapRaised(display, volume_win);
        return;
    }
    volume_win = XCreateSimpleWindow(display, root, 200, 100, VOLUME_WIN_WIDTH, VOLUME_WIN_HEIGHT, 2,
                                     BlackPixel(display, screen), 0x333333);
    XSelectInput(display, volume_win, ExposureMask | ButtonPressMask);
    XMapWindow(display, volume_win);
    draw_text(volume_win, 10, 20, "Volume Control (placeholder)", 0xFFFFFF);
}

void change_volume(const char *cmd) {
    if (fork() == 0) {
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        exit(1);
    }
}

void grab_keys() {
    XGrabKey(display, XKeysymToKeycode(display, XK_Super_L), AnyModifier, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, 0x1008FF11), AnyModifier, root, True, GrabModeAsync, GrabModeAsync); // Vol Down
    XGrabKey(display, XKeysymToKeycode(display, 0x1008FF13), AnyModifier, root, True, GrabModeAsync, GrabModeAsync); // Vol Up
    XGrabKey(display, XKeysymToKeycode(display, 0x1008FF12), AnyModifier, root, True, GrabModeAsync, GrabModeAsync); // Mute
}

void event_loop(void) {
    XEvent ev;
    while (1) {
        while (XPending(display)) {
            XNextEvent(display, &ev);
            if (ev.type == KeyPress) {
                if (ev.xkey.keycode == XKeysymToKeycode(display, XK_Super_L)) {
                    show_app_menu();
                } else if (ev.xkey.keycode == XKeysymToKeycode(display, 0x1008FF11)) {
                    change_volume("pactl set-sink-volume @DEFAULT_SINK@ -5%");
                } else if (ev.xkey.keycode == XKeysymToKeycode(display, 0x1008FF13)) {
                    change_volume("pactl set-sink-volume @DEFAULT_SINK@ +5%");
                } else if (ev.xkey.keycode == XKeysymToKeycode(display, 0x1008FF12)) {
                    change_volume("pactl set-sink-mute @DEFAULT_SINK@ toggle");
                }
            }
            if (ev.type == ButtonPress) {
                if (ev.xbutton.window == app_menu_button) show_app_menu();
                else if (ev.xbutton.window == notification_button) show_notifications();
                else if (ev.xbutton.window == logout_button) lightdm_logout();
                else if (ev.xbutton.window == settings_button) show_settings();
                else if (ev.xbutton.window == volume_button) show_volume();
            }
            if (ev.type == Expose) {
                if (ev.xexpose.window == app_menu_button)
                    draw_text(app_menu_button, 10, 20, "Apps", 0xFFFFFF);
                else if (ev.xexpose.window == notification_button)
                    draw_text(notification_button, 5, 20, "Notify", 0xFFFFFF);
                else if (ev.xexpose.window == logout_button)
                    draw_text(logout_button, 5, 20, "Logout", 0xFFFFFF);
                else if (ev.xexpose.window == settings_button)
                    draw_text(settings_button, 5, 20, "Settings", 0xFFFFFF);
                else if (ev.xexpose.window == clock_win) draw_clock();
            }
        }
        draw_clock();
        sleep(1);
    }
}

void create_taskbar(void) {
    int display_width = DisplayWidth(display, screen);
    int display_height = DisplayHeight(display, screen);
    int taskbar_width = (int)(display_width * 0.8);
    int taskbar_x = (display_width - taskbar_width) / 2;
    int taskbar_y = display_height - 40 - 10;

    XSetWindowAttributes wa;
    wa.override_redirect = True;
    wa.background_pixel = 0x333333;
    wa.event_mask = ExposureMask | ButtonPressMask;

    taskbar_win = XCreateWindow(display, root, taskbar_x, taskbar_y,
                                taskbar_width, 40,
                                0, CopyFromParent, InputOutput,
                                CopyFromParent, CWOverrideRedirect | CWBackPixel | CWEventMask, &wa);
    XMapWindow(display, taskbar_win);

    app_menu_button = XCreateSimpleWindow(display, taskbar_win, 10, 5, 80, 30,
                                          0, BlackPixel(display, screen), 0x555555);
    XSelectInput(display, app_menu_button, ButtonPressMask | ExposureMask);
    XMapWindow(display, app_menu_button);

    notification_button = XCreateSimpleWindow(display, taskbar_win, 100, 5, 80, 30,
                                              0, BlackPixel(display, screen), 0x555555);
    XSelectInput(display, notification_button, ButtonPressMask | ExposureMask);
    XMapWindow(display, notification_button);

    logout_button = XCreateSimpleWindow(display, taskbar_win, 190, 5, 80, 30,
                                        0, BlackPixel(display, screen), 0x555555);
    XSelectInput(display, logout_button, ButtonPressMask | ExposureMask);
    XMapWindow(display, logout_button);

    clock_win = XCreateSimpleWindow(display, taskbar_win,
                                    taskbar_width - 120 - 10, 5,
                                    120, 30,
                                    0, BlackPixel(display, screen), 0x333333);
    XSelectInput(display, clock_win, ExposureMask);
    XMapWindow(display, clock_win);

    gc = XCreateGC(display, taskbar_win, 0, NULL);

    settings_button = XCreateSimpleWindow(display, taskbar_win, 280, 5, 80, 30,
                                          0, BlackPixel(display, screen), 0x555555);
    XSelectInput(display, settings_button, ButtonPressMask | ExposureMask);
    XMapWindow(display, settings_button);

    volume_button = XCreateSimpleWindow(display, taskbar_win, 370, 5, 80, 30,
                                        0, BlackPixel(display, screen), 0x555555);
    XSelectInput(display, volume_button, ButtonPressMask | ExposureMask);
    XMapWindow(display, volume_button);
}

int main(void) {
    display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Unable to open X display\n");
        exit(1);
    }

    screen = DefaultScreen(display);
    root = DefaultRootWindow(display);

    create_taskbar();
    grab_keys();
    event_loop();

    XCloseDisplay(display);
    return 0;
}
