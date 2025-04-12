#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define TASKBAR_HEIGHT 40

Display *display;
Window root, taskbar_win, settings_button;
int screen;

/**
 * Function to launch Flow Settings when clicked
 */
void handle_settings_click(Window win) {
    if (win == settings_button) {
        if (fork() == 0) {
            execlp("./flow-settings", "flow-settings", NULL);
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * Creates the taskbar with a Settings button
 */
void create_taskbar() {
    taskbar_win = XCreateSimpleWindow(display, root, 0, DisplayHeight(display, screen) - TASKBAR_HEIGHT,
        DisplayWidth(display, screen), TASKBAR_HEIGHT, 0, CopyFromParent, InputOutput, CopyFromParent, 0, NULL);
    XMapWindow(display, taskbar_win);

    // Create Flow Settings button
    settings_button = XCreateSimpleWindow(display, taskbar_win, 10, 5, 100, 30, 0, BlackPixel(display, screen), 0x444444);
    XSelectInput(display, settings_button, ButtonPressMask | ExposureMask);
    XMapWindow(display, settings_button);
}

/**
 * Main event loop to detect clicks on the settings button
 */
void event_loop() {
    XEvent ev;
    while (1) {
        while (XPending(display)) {
            XNextEvent(display, &ev);
            if (ev.type == ButtonPress) {
                handle_settings_click(ev.xbutton.window);
            }
        }
        usleep(100000);
    }
}

int main() {
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Failed to open display\n");
        return EXIT_FAILURE;
    }

    screen = DefaultScreen(display);
    root = RootWindow(display, screen);

    create_taskbar();  // Adds the taskbar and Flow Settings launcher

    event_loop();
    return EXIT_SUCCESS;
}
