/* flow-desktop.c - Enhanced X11 desktop environment with extensive documentation.
 *
 * Features:
 * - Sets a wallpaper using Nitrogen (with fallback to Feh).
 * - Uses a custom cursor.
 * - Creates a floating taskbar with rounded corners at the bottom.
 *   The taskbar includes a popup app menu button, a notification button,
 *   a logout button (integrates with LightDM via dm-tool), and a clock that
 *   updates every second.
 * - Comprehensive error checking is included.
 * - Hardware acceleration note: If using a compositing manager, GPU acceleration
 *   may be available. Advanced users may integrate XShm or XRender for further acceleration.
 *
 * Dependencies:
 * - X11 (libX11-dev and libXext-dev)
 * - wget, nitrogen, feh, dm-tool (for LightDM integration)
 * - A compositing manager is recommended for smooth rounded corner rendering.
 *
 * Author: Your Name
 * Date: YYYY-MM-DD
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/extensions/shape.h>   // For creating non-rectangular (rounded) windows
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>

/* Constants for UI dimensions and colors */
#define TASKBAR_HEIGHT     40                /* Height of the floating taskbar */
#define TASKBAR_WIDTH_RATIO 0.8              /* Taskbar takes 80% of screen width */
#define TASKBAR_MARGIN_BOTTOM 10            /* Margin from the bottom edge of screen */
#define TASKBAR_BG_COLOR   0x333333          /* Dark gray background color */
#define BUTTON_BG_COLOR    0x555555          /* Slightly lighter dark for buttons */
#define BUTTON_FG_COLOR    0xFFFFFF          /* White for button text */
#define CLOCK_WIDTH        120               /* Width of the clock area */
#define CORNER_RADIUS      10                /* Radius for rounded corners */

/* Global variables for X11 objects */
Display *display = NULL;
Window root;
int screen;

/* Floating taskbar and its subwindow variables */
Window taskbar_win;          /* The floating taskbar window */
Window app_menu_button;      /* Button to launch the apps menu */
Window notification_button;  /* Button for notifications */
Window logout_button;        /* Button to trigger LightDM logout */
Window clock_win;            /* Window that displays the clock */
Window app_menu_win = 0;     /* Popup window for app menu (dummy implementation) */

GC gc;  /* Graphics context used for drawing */

pid_t nitrogen_pid = -1;  /* Process id of the nitrogen wallpaper-setting process */

/* Forward declarations of functions */
void cleanup(void);
void setup_cursor(void);
void set_wallpaper(void);
void create_taskbar(void);
void apply_rounded_corners(Window win, int width, int height, int radius);
void draw_clock(void);
void show_app_menu(void);
void show_notifications(void);
void lightdm_logout(void);
void event_loop(void);

/**
 * cleanup - Releases allocated resources upon exit.
 */
void cleanup(void) {
    if (nitrogen_pid > 0) {
        kill(nitrogen_pid, SIGTERM);
        waitpid(nitrogen_pid, NULL, 0);
    }
    if (display)
        XCloseDisplay(display);
}

/**
 * setup_cursor - Sets the X11 cursor for the root window to a left-pointer.
 */
void setup_cursor(void) {
    if (!display)
        return;
    Cursor cursor = XCreateFontCursor(display, XC_left_ptr);
    if (!cursor) {
        fprintf(stderr, "Error creating cursor.\n");
        return;
    }
    XDefineCursor(display, root, cursor);
    XFlush(display);
}

/**
 * set_wallpaper - Sets the desktop wallpaper using Nitrogen (fallback to Feh).
 *
 * Forks a child process to run the wallpaper setter.
 */
void set_wallpaper(void) {
    if (nitrogen_pid > 0) {
        kill(nitrogen_pid, SIGTERM);
        waitpid(nitrogen_pid, NULL, 0);
    }
    nitrogen_pid = fork();
    if (nitrogen_pid < 0) {
        perror("Fork failed for wallpaper");
        return;
    }
    if (nitrogen_pid == 0) {
        /* Try using nitrogen to restore wallpaper */
        execlp("nitrogen", "nitrogen", "--restore", NULL);
        /* If nitrogen fails, try feh as a fallback */
        execlp("feh", "feh", "--bg-fill", "/usr/share/backgrounds/default.jpg", NULL);
        perror("Failed to set wallpaper");
        exit(EXIT_FAILURE);
    }
}

/**
 * apply_rounded_corners - Applies a rounded corner shape mask to a given window.
 *
 * This function creates a bitmap mask for the window based on a rounded rectangle.
 * It then applies the mask using the X Shape Extension.
 *
 * Parameters:
 *   win    - The X11 window to modify.
 *   width  - The width of the window.
 *   height - The height of the window.
 *   radius - The corner radius.
 */
void apply_rounded_corners(Window win, int width, int height, int radius) {
    /* Create a 1-bit depth pixmap for the mask */
    Pixmap mask = XCreatePixmap(display, win, width, height, 1);
    if (!mask) {
        fprintf(stderr, "Error creating shape mask pixmap.\n");
        return;
    }
    /* Create a graphics context for the mask pixmap */
    GC mask_gc = XCreateGC(display, mask, 0, NULL);
    if (!mask_gc) {
        fprintf(stderr, "Error creating graphics context for mask.\n");
        XFreePixmap(display, mask);
        return;
    }
    /* Set the entire mask to 0 (transparent) */
    XSetForeground(display, mask_gc, 0);
    XFillRectangle(display, mask, mask_gc, 0, 0, width, height);

    /* Set drawing color to 1 (opaque) */
    XSetForeground(display, mask_gc, 1);

    /* Draw a rounded rectangle.
     * First, fill the central rectangle and side rectangles.
     */
    XFillRectangle(display, mask, mask_gc, radius, 0, width - 2 * radius, height);
    XFillRectangle(display, mask, mask_gc, 0, radius, width, height - 2 * radius);

    /* Draw circles at each corner */
    XFillArc(display, mask, mask_gc, 0, 0, 2 * radius, 2 * radius, 0, 360 * 64);
    XFillArc(display, mask, mask_gc, width - 2 * radius, 0, 2 * radius, 2 * radius, 0, 360 * 64);
    XFillArc(display, mask, mask_gc, 0, height - 2 * radius, 2 * radius, 2 * radius, 0, 360 * 64);
    XFillArc(display, mask, mask_gc, width - 2 * radius, height - 2 * radius, 2 * radius, 2 * radius, 0, 360 * 64);

    /* Apply the mask to the window using the Shape extension */
    XShapeCombineMask(display, win, ShapeBounding, 0, 0, mask, ShapeSet);

    /* Cleanup: free the mask graphic context and pixmap */
    XFreeGC(display, mask_gc);
    XFreePixmap(display, mask);
}

/**
 * create_taskbar - Creates the floating taskbar with rounded corners and all buttons.
 *
 * The taskbar is positioned at the bottom of the screen with a horizontal margin.
 * It includes:
 *  - An apps menu button ("Apps")
 *  - A notification button ("Notify")
 *  - A logout button ("Logout") for LightDM integration
 *  - A clock area that displays the current time
 */
void create_taskbar(void) {
    int display_width = DisplayWidth(display, screen);
    int display_height = DisplayHeight(display, screen);

    /* Calculate taskbar width (80% of screen width) and centered position */
    int taskbar_width = (int)(display_width * TASKBAR_WIDTH_RATIO);
    int taskbar_x = (display_width - taskbar_width) / 2;
    int taskbar_y = display_height - TASKBAR_HEIGHT - TASKBAR_MARGIN_BOTTOM;

    XSetWindowAttributes wa;
    wa.override_redirect = True;  // So that the window manager doesn't reposition/tile it
    wa.background_pixel = TASKBAR_BG_COLOR;
    wa.event_mask = ExposureMask | ButtonPressMask;

    /* Create the floating taskbar window with the calculated size and position */
    taskbar_win = XCreateWindow(display, root, taskbar_x, taskbar_y,
                                taskbar_width, TASKBAR_HEIGHT,
                                0, CopyFromParent, InputOutput,
                                CopyFromParent, CWOverrideRedirect | CWBackPixel | CWEventMask, &wa);
    if (!taskbar_win) {
        fprintf(stderr, "Error creating taskbar window.\n");
        cleanup();
        exit(EXIT_FAILURE);
    }
    XMapWindow(display, taskbar_win);

    /* Apply rounded corners to the taskbar window */
    apply_rounded_corners(taskbar_win, taskbar_width, TASKBAR_HEIGHT, CORNER_RADIUS);

    /* Create the Apps Menu button */
    app_menu_button = XCreateSimpleWindow(display, taskbar_win, 10, 5, 80, 30,
                                          0, BlackPixel(display, screen), BUTTON_BG_COLOR);
    if (!app_menu_button) {
        fprintf(stderr, "Error creating app menu button.\n");
        cleanup();
        exit(EXIT_FAILURE);
    }
    XSelectInput(display, app_menu_button, ButtonPressMask | ExposureMask);
    XMapWindow(display, app_menu_button);

    /* Create the Notification button */
    notification_button = XCreateSimpleWindow(display, taskbar_win, 100, 5, 80, 30,
                                              0, BlackPixel(display, screen), BUTTON_BG_COLOR);
    if (!notification_button) {
        fprintf(stderr, "Error creating notification button.\n");
        cleanup();
        exit(EXIT_FAILURE);
    }
    XSelectInput(display, notification_button, ButtonPressMask | ExposureMask);
    XMapWindow(display, notification_button);

    /* Create the Logout button for LightDM integration */
    logout_button = XCreateSimpleWindow(display, taskbar_win, 190, 5, 80, 30,
                                        0, BlackPixel(display, screen), BUTTON_BG_COLOR);
    if (!logout_button) {
        fprintf(stderr, "Error creating logout button.\n");
        cleanup();
        exit(EXIT_FAILURE);
    }
    XSelectInput(display, logout_button, ButtonPressMask | ExposureMask);
    XMapWindow(display, logout_button);

    /* Create the Clock window on the far right of the taskbar.
       The clock window shows the current time, updating each second.
    */
    clock_win = XCreateSimpleWindow(display, taskbar_win,
                                    taskbar_width - CLOCK_WIDTH - 10, 5,
                                    CLOCK_WIDTH, 30,
                                    0, BlackPixel(display, screen), TASKBAR_BG_COLOR);
    if (!clock_win) {
        fprintf(stderr, "Error creating clock window.\n");
        cleanup();
        exit(EXIT_FAILURE);
    }
    XSelectInput(display, clock_win, ExposureMask);
    XMapWindow(display, clock_win);

    /* Create a shared graphics context for drawing on these windows */
    gc = XCreateGC(display, taskbar_win, 0, NULL);
    if (!gc) {
        fprintf(stderr, "Error creating graphics context.\n");
        cleanup();
        exit(EXIT_FAILURE);
    }
}

/**
 * draw_clock - Updates and draws the current system time in the clock window.
 *
 * This function uses local system time, formats it, then clears the clock area
 * and draws the time in white on the taskbar.
 */
void draw_clock(void) {
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    if (!local)
        return;

    char time_str[64];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", local);

    /* Clear the clock area by filling it with the taskbar background color */
    XSetForeground(display, gc, TASKBAR_BG_COLOR);
    XFillRectangle(display, clock_win, gc, 0, 0, CLOCK_WIDTH, 30);

    /* Draw the time string; set foreground to white */
    XSetForeground(display, gc, 0xFFFFFF);
    XDrawString(display, clock_win, gc, 10, 20, time_str, strlen(time_str));
    XFlush(display);
}

/**
 * show_app_menu - Displays a popup application menu.
 *
 * This function provides a dummy implementation of an app menu popup.
 * In a real-world scenario, you would populate this window with icons
 * and commands for launching installed applications.
 */
void show_app_menu(void) {
    if (app_menu_win) {
        XMapRaised(display, app_menu_win);
        return;
    }
    int width = 300, height = 400;
    app_menu_win = XCreateSimpleWindow(display, root, 100, 100, width, height, 2,
                                       BlackPixel(display, screen), 0xAAAAAA);
    if (!app_menu_win) {
        fprintf(stderr, "Error creating app menu window.\n");
        return;
    }
    XSelectInput(display, app_menu_win, ExposureMask | ButtonPressMask);
    XMapWindow(display, app_menu_win);
}

/**
 * show_notifications - Displays a simple notification popup.
 *
 * A dummy implementation. Extend this function to support real notifications.
 */
void show_notifications(void) {
    Window notif_win = XCreateSimpleWindow(display, root, 150, 150, 200, 100, 2,
                                           BlackPixel(display, screen), 0xDDDDDD);
    if (!notif_win) {
        fprintf(stderr, "Error creating notification window.\n");
        return;
    }
    XSelectInput(display, notif_win, ExposureMask | ButtonPressMask);
    XMapWindow(display, notif_win);
    /* For a real notification system, you can integrate D-Bus or custom logic. */
}

/**
 * lightdm_logout - Triggers LightDM logout by switching to the greeter.
 *
 * Uses 'dm-tool switch-to-greeter'. Make sure dm-tool is installed and LightDM is configured.
 */
void lightdm_logout(void) {
    if (fork() == 0) {
        execlp("dm-tool", "dm-tool", "switch-to-greeter", NULL);
        perror("Failed to execute dm-tool");
        exit(EXIT_FAILURE);
    }
}

/**
 * event_loop - Main loop that processes X events and updates the UI continuously.
 *
 * Handles window expose events, button clicks (for app menu, notifications, logout),
 * and updates the clock display every second. Incorporates robust error-checking.
 */
void event_loop(void) {
    XEvent ev;
    while (1) {
        /* Process any pending events */
        while (XPending(display)) {
            XNextEvent(display, &ev);
            if (ev.type == Expose) {
                if (ev.xexpose.window == taskbar_win) {
                    /* Redraw the background for the taskbar */
                    XFillRectangle(display, taskbar_win, gc, 0, 0,
                                   DisplayWidth(display, screen) * TASKBAR_WIDTH_RATIO, TASKBAR_HEIGHT);
                } else if (ev.xexpose.window == app_menu_button) {
                    XSetForeground(display, gc, BUTTON_FG_COLOR);
                    XDrawString(display, app_menu_button, gc, 10, 20, "Apps", 4);
                } else if (ev.xexpose.window == notification_button) {
                    XSetForeground(display, gc, BUTTON_FG_COLOR);
                    XDrawString(display, notification_button, gc, 5, 20, "Notify", 6);
                } else if (ev.xexpose.window == logout_button) {
                    XSetForeground(display, gc, BUTTON_FG_COLOR);
                    XDrawString(display, logout_button, gc, 5, 20, "Logout", 6);
                } else if (ev.xexpose.window == clock_win) {
                    draw_clock();
                }
            }
            if (ev.type == ButtonPress) {
                if (ev.xbutton.window == app_menu_button) {
                    show_app_menu();
                } else if (ev.xbutton.window == notification_button) {
                    show_notifications();
                } else if (ev.xbutton.window == logout_button) {
                    lightdm_logout();
                } else if (ev.xbutton.window == app_menu_win) {
                    /* Clicking inside the popup app menu closes it */
                    XUnmapWindow(display, app_menu_win);
                    app_menu_win = 0;
                }
            }
        }
        /* Update the clock every second */
        draw_clock();
        sleep(1);
    }
}

/**
 * main - Entry point for Flow Desktop.
 *
 * Connects to the X server, initializes all components, and starts the event loop.
 */
int main(void) {
    /* Open a connection to the X server */
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Error: Unable to open display.\n");
        return EXIT_FAILURE;
    }
    screen = DefaultScreen(display);
    root = RootWindow(display, screen);
    if (!root) {
        fprintf(stderr, "Error: Unable to determine root window.\n");
        cleanup();
        return EXIT_FAILURE;
    }
    
    /* Initialize desktop components */
    setup_cursor();
    set_wallpaper();
    create_taskbar();

    /* Enter the main event loop */
    event_loop();

    cleanup();
    return EXIT_SUCCESS;
}
