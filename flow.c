// flow-desktop.c - KDE-style lightweight X11 desktop environment

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <X11/cursorfont.h>  // Include for cursor handling

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

Display *display;
Window root, taskbar, app_button, notif_button, logout_button, clock_win, app_menu = 0;
Window settings_button, settings_win = 0, volume_button, volume_win = 0;
GC gc;
int screen;

// Function to restore the cursor
void setup_cursor() {
    Cursor cursor = XCreateFontCursor(display, XC_left_ptr);
    XDefineCursor(display, root, cursor);
    XFlush(display);
}

// Function to restore Nitrogen wallpaper
void set_wallpaper() {
    if (fork() == 0) {
        execlp("nitrogen", "nitrogen", "--restore", NULL);
        execlp("feh", "feh", "--bg-fill", "/usr/share/backgrounds/default.jpg", NULL);
        exit(1);
    }
}

void draw_text(Window win, int x, int y, const char* txt, unsigned long color) {
    XSetForeground(display, gc, color);
    XDrawImageString(display, win, gc, x, y, txt, strlen(txt));
}

void launch_app(const char *cmd) {
    if (fork() == 0) {
        setsid();
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        exit(1);
    }
}

void show_app_menu() {
    if (app_menu) {
        XMapRaised(display, app_menu);
        return;
    }
    app_menu = XCreateSimpleWindow(display, root, 100, 100, APP_MENU_WIDTH, APP_MENU_HEIGHT, 2,
                                    BlackPixel(display, screen), 0x222222);
    XSelectInput(display, app_menu, ExposureMask | ButtonPressMask);
    XMapWindow(display, app_menu);

    DIR *d = opendir("/usr/share/applications");
    struct dirent *dir;
    int y = 20;
    if (d) {
        while ((dir = readdir(d)) && y < APP_MENU_HEIGHT) {
            if (strstr(dir->d_name, ".desktop")) {
                char name[256], exec[512] = "", line[1024], path[512];
                snprintf(path, sizeof(path), "/usr/share/applications/%s", dir->d_name);
                FILE *f = fopen(path, "r");
                if (f) {
                    while (fgets(line, sizeof(line), f)) {
                        if (strncmp(line, "Name=", 5) == 0) sscanf(line, "Name=%255[^\n]", name);
                        else if (strncmp(line, "Exec=", 5) == 0) sscanf(line, "Exec=%511[^\n]", exec);
                    }
                    fclose(f);
                    draw_text(app_menu, 10, y, name, 0xFFFFFF);
                    y += 20;
                }
            }
        }
        closedir(d);
    }
}

void show_settings() {
    if (settings_win) {
        XMapRaised(display, settings_win);
        return;
    }
    settings_win = XCreateSimpleWindow(display, root, 200, 200, SETTINGS_WIDTH, SETTINGS_HEIGHT, 2,
                                       BlackPixel(display, screen), 0x444444);
    XSelectInput(display, settings_win, ExposureMask | ButtonPressMask);
    XMapWindow(display, settings_win);
    draw_text(settings_win, 10, 20, "Settings (Coming Soon)", 0xFFFFFF);
}

void show_volume() {
    if (volume_win) {
        XMapRaised(display, volume_win);
        return;
    }
    volume_win = XCreateSimpleWindow(display, root, 250, 150, VOL_WIDTH, VOL_HEIGHT, 2,
                                     BlackPixel(display, screen), 0x333355);
    XSelectInput(display, volume_win, ExposureMask);
    XMapWindow(display, volume_win);
    draw_text(volume_win, 10, 20, "Volume: use keys", 0xFFFFFF);
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

void draw_clock() {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char buf[64];
    strftime(buf, sizeof(buf), "%H:%M:%S", tm);
    XSetForeground(display, gc, 0x333333);
    XFillRectangle(display, clock_win, gc, 0, 0, CLOCK_WIDTH, 30);
    draw_text(clock_win, 10, 20, buf, 0xFFFFFF);
}

void event_loop() {
    XEvent e;
    while (1) {
        while (XPending(display)) {
            XNextEvent(display, &e);
            if (e.type == KeyPress) {
                KeySym ks = XKeycodeToKeysym(display, e.xkey.keycode, 0);
                if (ks == XK_Super_L) show_app_menu();
                else if (ks == 0x1008FF13) change_volume("pactl set-sink-volume @DEFAULT_SINK@ +5%");
                else if (ks == 0x1008FF11) change_volume("pactl set-sink-volume @DEFAULT_SINK@ -5%");
                else if (ks == 0x1008FF12) change_volume("pactl set-sink-mute @DEFAULT_SINK@ toggle");
            } else if (e.type == Expose) {
                if (e.xexpose.window == app_button) draw_text(app_button, 10, 20, "Apps", 0xFFFFFF);
                if (e.xexpose.window == notif_button) draw_text(notif_button, 5, 20, "Notify", 0xFFFFFF);
                if (e.xexpose.window == logout_button) draw_text(logout_button, 5, 20, "Logout", 0xFFFFFF);
                if (e.xexpose.window == settings_button) draw_text(settings_button, 5, 20, "Settings", 0xFFFFFF);
                if (e.xexpose.window == volume_button) draw_text(volume_button, 5, 20, "Volume", 0xFFFFFF);
                if (e.xexpose.window == clock_win) draw_clock();
            } else if (e.type == ButtonPress) {
                if (e.xbutton.window == app_button) show_app_menu();
                else if (e.xbutton.window == notif_button) show_volume();
                else if (e.xbutton.window == settings_button) show_settings();
                else if (e.xbutton.window == logout_button) exit(0);
                else if (e.xbutton.window == volume_button) show_volume();
            }
        }
        draw_clock();
        sleep(1);
    }
}

void create_taskbar() {
    int dw = DisplayWidth(display, screen);
    int dh = DisplayHeight(display, screen);
    int width = dw * WIDTH_RATIO;
    int x = (dw - width) / 2;
    int y = dh - HEIGHT - 10;

    XSetWindowAttributes attr;
    attr.override_redirect = True;
    attr.background_pixel = 0x333333;
    attr.event_mask = ExposureMask | ButtonPressMask;
    taskbar = XCreateWindow(display, root, x, y, width, HEIGHT, 0,
                            CopyFromParent, InputOutput, CopyFromParent,
                            CWOverrideRedirect | CWBackPixel | CWEventMask, &attr);
    XMapWindow(display, taskbar);

    app_button = XCreateSimpleWindow(display, taskbar, 10, 5, BUTTON_WIDTH, BUTTON_HEIGHT, 0,
                                     BlackPixel(display, screen), 0x555555);
    XSelectInput(display, app_button, ExposureMask | ButtonPressMask);
    XMapWindow(display, app_button);

    notif_button = XCreateSimpleWindow(display, taskbar, 100, 5, BUTTON_WIDTH, BUTTON_HEIGHT, 0,
                                       BlackPixel(display, screen), 0x555555);
    XSelectInput(display, notif_button, ExposureMask | ButtonPressMask);
    XMapWindow(display, notif_button);

    logout_button = XCreateSimpleWindow(display, taskbar, 190, 5, BUTTON_WIDTH, BUTTON_HEIGHT, 0,
                                        BlackPixel(display, screen), 0x555555);
    XSelectInput(display, logout_button, ExposureMask | ButtonPressMask);
    XMapWindow(display, logout_button);

    clock_win = XCreateSimpleWindow(display, taskbar, width - CLOCK_WIDTH - 10, 5, CLOCK_WIDTH, 30, 0,
                                    BlackPixel(display, screen), 0x333333);
    XSelectInput(display, clock_win, ExposureMask);
    XMapWindow(display, clock_win);

    settings_button = XCreateSimpleWindow(display, taskbar, 280, 5, BUTTON_WIDTH, BUTTON_HEIGHT, 0,
                                          BlackPixel(display, screen), 0x555555);
    XSelectInput(display, settings_button, ExposureMask | ButtonPressMask);
    XMapWindow(display, settings_button);

    volume_button = XCreateSimpleWindow(display, taskbar, 370, 5, BUTTON_WIDTH, BUTTON_HEIGHT, 0,
                                        BlackPixel(display, screen), 0x555555);
    XSelectInput(display, volume_button, ExposureMask | ButtonPressMask);
    XMapWindow(display, volume_button);

    gc = XCreateGC(display, taskbar, 0, NULL);
}

int main() {
    display = XOpenDisplay(NULL);
    screen = DefaultScreen(display);
    root = RootWindow(display, screen);
    create_taskbar();
    setup_cursor(); // Restore the cursor
    set_wallpaper(); // Restore the wallpaper
    grab_keys();
    event_loop();
    return 0;
}
