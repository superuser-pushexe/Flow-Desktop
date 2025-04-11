/* flow-desktop.c - X11 desktop with cursor, wallpaper, and taskbar */
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

// Configuration
#define TASKBAR_HEIGHT 40
#define LAUNCHER_SIZE 36
#define LAUNCHER_PADDING 8
#define CLOCK_WIDTH 120
#define ICON_SIZE 24

typedef struct {
    Window win;
    char *command;
    char *icon_name;
} Launcher;

// Global variables
Display *display;
Window root, taskbar_win;
int screen;
Launcher launchers[5];
int launcher_count = 0;
GC gc;
pid_t nitrogen_pid = -1;

// Function prototypes
void x11_check(int status, const char *msg);
void setup_cursor();
void set_wallpaper();
void draw_launcher(Launcher *launcher);
void draw_clock();
void add_launcher(const char *icon_name, const char *command);
void create_taskbar();
void handle_launcher_click(Window win);
void event_loop();
void cleanup();

void x11_check(int status, const char *msg) {
    if (status == BadAlloc || status == BadWindow) {
        fprintf(stderr, "X11 Error: %s\n", msg);
        exit(EXIT_FAILURE);
    }
}

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

void draw_launcher(Launcher *launcher) {
    XSetForeground(display, gc, 0x333333);
    XFillRectangle(display, launcher->win, gc, 0, 0, LAUNCHER_SIZE, LAUNCHER_SIZE);
    
    XSetForeground(display, gc, WhitePixel(display, screen));
    XDrawString(display, launcher->win, gc, 
               LAUNCHER_SIZE/2 - 4, LAUNCHER_SIZE/2 + 4,
               launcher->icon_name, 1);
}

void draw_clock() {
    static time_t last_update = 0;
    time_t now = time(NULL);
    
    if (now == last_update) return;
    last_update = now;
    
    XSetForeground(display, gc, 0x333333);
    XFillRectangle(display, taskbar_win, gc, 
                  DisplayWidth(display, screen) - CLOCK_WIDTH, 0,
                  CLOCK_WIDTH, TASKBAR_HEIGHT);
    
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", localtime(&now));
    
    XSetForeground(display, gc, WhitePixel(display, screen));
    XDrawString(display, taskbar_win, gc, 
               DisplayWidth(display, screen) - CLOCK_WIDTH + 10,
               TASKBAR_HEIGHT / 2 + 5,
               time_str, strlen(time_str));
}

void add_launcher(const char *icon_name, const char *command) {
    if (launcher_count >= 5) return;
    
    XSetWindowAttributes attrs = {
        .background_pixel = 0x333333,
        .event_mask = ButtonPressMask | ExposureMask
    };
    
    launchers[launcher_count].win = XCreateWindow(display, taskbar_win,
        LAUNCHER_PADDING + (LAUNCHER_SIZE + LAUNCHER_PADDING) * launcher_count,
        (TASKBAR_HEIGHT - LAUNCHER_SIZE) / 2,
        LAUNCHER_SIZE, LAUNCHER_SIZE,
        0, CopyFromParent, InputOutput, CopyFromParent,
        CWBackPixel | CWEventMask, &attrs);
    
    launchers[launcher_count].command = strdup(command);
    launchers[launcher_count].icon_name = strdup(icon_name);
    
    XSelectInput(display, launchers[launcher_count].win, ExposureMask | ButtonPressMask);
    XMapWindow(display, launchers[launcher_count].win);
    draw_launcher(&launchers[launcher_count]);
    launcher_count++;
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
    
    Atom net_wm_window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom net_wm_window_type_dock = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(display, taskbar_win, net_wm_window_type, XA_ATOM, 32,
                  PropModeReplace, (unsigned char *)&net_wm_window_type_dock, 1);
    
    XMapWindow(display, taskbar_win);
}

void handle_launcher_click(Window win) {
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

int main(int argc, char *argv[]) {
    set_wallpaper();
    
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
    add_launcher("T", "xterm");
    add_launcher("W", "xdg-open https://google.com");
    add_launcher("F", "pcmanfm");
    
    event_loop();
    
    return EXIT_SUCCESS;
}
