/* flow-desktop.c - Robust X11 desktop with Openbox */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>

// Configuration
#define TASKBAR_HEIGHT 30
#define LAUNCHER_SIZE 24
#define LAUNCHER_PADDING 5
#define CLOCK_WIDTH 100

typedef struct {
    Window win;
    char *command;
} Launcher;

Display *display;
Window root, taskbar_win;
int screen;
Launcher launchers[5];
int launcher_count = 0;

void x11_check(int status, const char *msg) {
    if (status == BadAlloc || status == BadWindow) {
        fprintf(stderr, "X11 Error: %s\n", msg);
        exit(EXIT_FAILURE);
    }
}

void launch_openbox() {
    if (fork() == 0) {
        // First try with XDG config
        char *home = getenv("HOME");
        char config_path[256];
        snprintf(config_path, sizeof(config_path), "%s/.config/openbox/autostart", home);
        
        if (access(config_path, F_OK) != -1) {
            execlp("openbox", "openbox", "--config-file", config_path, "--sm-disable", NULL);
        }
        execlp("openbox", "openbox", "--sm-disable", NULL);
        perror("Failed to start Openbox");
        exit(EXIT_FAILURE);
    }
    sleep(1); // Wait for WM to initialize
}

void create_taskbar() {
    XSetWindowAttributes attrs = {
        .override_redirect = True,
        .background_pixel = BlackPixel(display, screen),
        .event_mask = ExposureMask | ButtonPressMask
    };
    
    taskbar_win = XCreateWindow(display, root,
        0, DisplayHeight(display, screen) - TASKBAR_HEIGHT,
        DisplayWidth(display, screen), TASKBAR_HEIGHT,
        0, CopyFromParent, InputOutput, CopyFromParent,
        CWOverrideRedirect | CWBackPixel | CWEventMask, &attrs);
    
    // Set window type as dock
    Atom net_wm_window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom net_wm_window_type_dock = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(display, taskbar_win, net_wm_window_type, XA_ATOM, 32,
                  PropModeReplace, (unsigned char *)&net_wm_window_type_dock, 1);
    
    XMapWindow(display, taskbar_win);
}

void add_launcher(const char *name, const char *command) {
    if (launcher_count >= 5) return;
    
    int x_pos = LAUNCHER_PADDING + (launcher_count * (LAUNCHER_SIZE + LAUNCHER_PADDING));
    
    launchers[launcher_count].win = XCreateSimpleWindow(display, taskbar_win,
        x_pos, (TASKBAR_HEIGHT - LAUNCHER_SIZE)/2,
        LAUNCHER_SIZE, LAUNCHER_SIZE, 0,
        WhitePixel(display, screen), BlackPixel(display, screen));
    
    XSelectInput(display, launchers[launcher_count].win, ExposureMask | ButtonPressMask);
    XStoreName(display, launchers[launcher_count].win, name);
    XMapWindow(display, launchers[launcher_count].win);
    
    launchers[launcher_count].command = strdup(command);
    launcher_count++;
}

void draw_clock(GC gc) {
    time_t now = time(NULL);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%r", localtime(&now));
    
    XDrawString(display, taskbar_win, gc, 
        DisplayWidth(display, screen) - CLOCK_WIDTH, 
        TASKBAR_HEIGHT/2 + 5,
        time_str, strlen(time_str));
}

void draw_taskbar() {
    GC gc = XCreateGC(display, taskbar_win, 0, NULL);
    XSetForeground(display, gc, WhitePixel(display, screen));
    
    // Draw launcher backgrounds
    for (int i = 0; i < launcher_count; i++) {
        XFillRectangle(display, launchers[i].win, gc, 0, 0, LAUNCHER_SIZE, LAUNCHER_SIZE);
    }
    
    draw_clock(gc);
    XFreeGC(display, gc);
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
    XEvent event;
    while (1) {
        XNextEvent(display, &event);
        
        switch (event.type) {
            case Expose:
                if (event.xexpose.window == taskbar_win) {
                    draw_taskbar();
                }
                break;
                
            case ButtonPress:
                handle_launcher_click(event.xbutton.window);
                break;
        }
    }
}

void setup_environment() {
    // Set fallback cursor
    if (system("xsetroot -cursor_name left_ptr") == -1) {
        perror("Failed to set cursor");
    }
    
    // Ensure basic X resources
    putenv("XMODIFIERS=");
    putenv("QT_AUTO_SCREEN_SCALE_FACTOR=1");
}

int main(int argc, char *argv[]) {
    system("nitrogen --restore");  // Set wallpaper using nitrogen

    setup_environment();
    
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Failed to open display. Try: startx %s\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    screen = DefaultScreen(display);
    root = RootWindow(display, screen);
    
    launch_openbox();
    create_taskbar();
    
    // Add default launchers
    add_launcher("Term", "xterm");
    add_launcher("Web", "xdg-open https://google.com");
    add_launcher("Files", "pcmanfm");
    
    event_loop();
    
    XCloseDisplay(display);
    return EXIT_SUCCESS;
}
