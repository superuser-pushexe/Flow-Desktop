/* flow-desktop.c - X11 desktop with cursor, wallpaper, taskbar, and app menu */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>

#define TASKBAR_HEIGHT 40
#define LAUNCHER_SIZE 36
#define LAUNCHER_PADDING 8
#define CLOCK_WIDTH 120
#define ICON_SIZE 24
#define MENU_WIDTH 300
#define MENU_HEIGHT 400
#define MAX_APPS 50

typedef struct {
    Window win;
    char *command;
    char *icon_name;
} Launcher;

Display *display;
Window root, taskbar_win, app_menu_button, menu_win;
int screen;
Launcher launchers[5];
int launcher_count = 0;
GC gc;
pid_t nitrogen_pid = -1;
char *installed_apps[MAX_APPS];
int app_count = 0;

void setup_cursor();
void set_wallpaper();
void draw_launcher(Launcher *launcher);
void draw_clock();
void add_launcher(const char *icon_name, const char *command);
void create_taskbar();
void handle_launcher_click(Window win);
void show_app_menu();
void event_loop();
void cleanup();
void fetch_installed_apps();

void setup_cursor() {
    Cursor cursor = XCreateFontCursor(display, XC_left_ptr);
    XDefineCursor(display, root, cursor);
    XFlush(display);
}

void set_wallpaper() {
    if (nitrogen_pid > 0) {
        kill(nitrogen_pid, SIGTERM);
        waitpid(nitrogen_pid, NULL, 0);
    }

    nitrogen_pid = fork();
    if (nitrogen_pid == 0) {
        execlp("nitrogen", "nitrogen", "--restore", NULL);
        execlp("feh", "feh", "--bg-fill", "/usr/share/backgrounds/default.jpg", NULL);
        exit(EXIT_FAILURE);
    } else if (nitrogen_pid < 0) {
        perror("Failed to fork for wallpaper");
    }
}

void create_taskbar() {
    XSetWindowAttributes attrs = {
        .override_redirect = True,
        .background_pixel = 0x222222,
        .event_mask = ExposureMask
    };

    taskbar_win = XCreateWindow(display, root,
        0, DisplayHeight(display, screen) - TASKBAR_HEIGHT,
        DisplayWidth(display, screen), TASKBAR_HEIGHT,
        0, CopyFromParent, InputOutput, CopyFromParent,
        CWOverrideRedirect | CWBackPixel | CWEventMask, &attrs);

    XMapWindow(display, taskbar_win);

    app_menu_button = XCreateSimpleWindow(display, taskbar_win,
        10, 5, 80, 30, 0, BlackPixel(display, screen), 0x444444);
    XSelectInput(display, app_menu_button, ButtonPressMask | ExposureMask);
    XMapWindow(display, app_menu_button);
}

void fetch_installed_apps() {
    struct dirent *entry;
    DIR *dp = opendir("/usr/share/applications/");
    if (!dp) {
        perror("Failed to open /usr/share/applications/");
        return;
    }

    while ((entry = readdir(dp)) && app_count < MAX_APPS) {
        if (strstr(entry->d_name, ".desktop")) {
            installed_apps[app_count] = strdup(entry->d_name);
            app_count++;
        }
    }
    closedir(dp);
}

void show_app_menu() {
    menu_win = XCreateSimpleWindow(display, root, 100, 100, MENU_WIDTH, MENU_HEIGHT, 2, BlackPixel(display, screen), WhitePixel(display, screen));
    XSelectInput(display, menu_win, ExposureMask | ButtonPressMask);
    XMapWindow(display, menu_win);
}

void handle_launcher_click(Window win) {
    if (win == app_menu_button) {
        show_app_menu();
    }
    for (int i = 0; i < launcher_count; i++) {
        if (win == launchers[i].win) {
            if (fork() == 0) {
                setsid();
                execlp("/bin/sh", "sh", "-c", launchers[i].command, NULL);
                exit(EXIT_FAILURE);
            }
            break;
        }
    }
}

void event_loop() {
    XEvent ev;
    while (1) {
        while (XPending(display)) {
            XNextEvent(display, &ev);
            switch (ev.type) {
                case Expose:
                    if (ev.xexpose.window == taskbar_win) {
                        draw_clock();
                    }
                    break;
                case ButtonPress:
                    handle_launcher_click(ev.xbutton.window);
                    break;
            }
        }
        draw_clock();
        usleep(100000);
    }
}

void cleanup() {
    if (nitrogen_pid > 0) {
        kill(nitrogen_pid, SIGTERM);
        waitpid(nitrogen_pid, NULL, 0);
    }
    XCloseDisplay(display);
}

int main() {
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Failed to open display\n");
        return EXIT_FAILURE;
    }

    atexit(cleanup);

    screen = DefaultScreen(display);
    root = RootWindow(display, screen);

    setup_cursor();
    gc = XCreateGC(display, root, 0, NULL);

    create_taskbar();
    fetch_installed_apps();

    add_launcher("T", "xterm");
    add_launcher("W", "xdg-open https://google.com");
    add_launcher("F", "pcmanfm");

    event_loop();
    return EXIT_SUCCESS;
}
