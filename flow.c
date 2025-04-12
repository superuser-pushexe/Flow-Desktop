/* flow-desktop.c - X11 desktop with cursor, wallpaper (via Nitrogen), taskbar with clock,
   notification button, popup app menu, comprehensive error checking, and LightDM integration.
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>

/* Constants for UI elements */
#define TASKBAR_HEIGHT     40
#define TASKBAR_BG_COLOR   0x333333   /* Shade of grey for taskbar */
#define BUTTON_BG_COLOR    0x555555   /* Button background color */
#define BUTTON_FG_COLOR    0xFFFFFF   /* Button text color */
#define CLOCK_WIDTH        120

/* Global X11 variables */
Display *display = NULL;
Window root;
int screen;

/* Taskbar and button window variables */
Window taskbar_win;
Window app_menu_button;
Window notification_button;
Window logout_button;
Window clock_win;         /* Window for clock display */
Window app_menu_win = 0;  /* Popup apps menu window */

GC gc;  /* Graphics context for drawing */

pid_t nitrogen_pid = -1;  /* To track the wallpaper process */

/* Function declarations */
void cleanup(void);
void setup_cursor(void);
void set_wallpaper(void);
void create_taskbar(void);
void draw_clock(void);
void show_app_menu(void);
void show_notifications(void);
void lightdm_logout(void);
void event_loop(void);

/* Cleanup function releases resources on exit */
void cleanup(void) {
    if (nitrogen_pid > 0) {
        kill(nitrogen_pid, SIGTERM);
        waitpid(nitrogen_pid, NULL, 0);
    }
    if (display)
        XCloseDisplay(display);
}

/* Setup the default cursor (left pointer) */
void setup_cursor(void) {
    if (!display) return;
    Cursor cursor = XCreateFontCursor(display, XC_left_ptr);
    if (!cursor) {
        fprintf(stderr, "Error creating cursor.\n");
        return;
    }
    XDefineCursor(display, root, cursor);
    XFlush(display);
}

/* Set the wallpaper using Nitrogen, fallback to Feh if needed */
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
    if (nitrogen_pid == 0) { // Child process
        execlp("nitrogen", "nitrogen", "--restore", NULL);
        /* If nitrogen fails, try feh */
        execlp("feh", "feh", "--bg-fill", "/usr/share/backgrounds/default.jpg", NULL);
        perror("Failed to set wallpaper");
        exit(EXIT_FAILURE);
    }
}

/* Create the taskbar and its subwindows (apps, notifications, logout, clock) */
void create_taskbar(void) {
    int display_width = DisplayWidth(display, screen);
    int display_height = DisplayHeight(display, screen);

    XSetWindowAttributes wa;
    wa.override_redirect = True;
    wa.background_pixel = TASKBAR_BG_COLOR;
    wa.event_mask = ExposureMask | ButtonPressMask;

    taskbar_win = XCreateWindow(display, root,
                                0, display_height - TASKBAR_HEIGHT,
                                display_width, TASKBAR_HEIGHT,
                                0, CopyFromParent, InputOutput, CopyFromParent,
                                CWOverrideRedirect | CWBackPixel | CWEventMask, &wa);
    if (!taskbar_win) {
        fprintf(stderr, "Error creating taskbar window.\n");
        cleanup();
        exit(EXIT_FAILURE);
    }
    XMapWindow(display, taskbar_win);

    /* Create App Menu Button */
    app_menu_button = XCreateSimpleWindow(display, taskbar_win, 10, 5, 80, 30,
                                          0, BlackPixel(display, screen), BUTTON_BG_COLOR);
    if (!app_menu_button) {
        fprintf(stderr, "Error creating app menu button.\n");
        cleanup();
        exit(EXIT_FAILURE);
    }
    XSelectInput(display, app_menu_button, ButtonPressMask | ExposureMask);
    XMapWindow(display, app_menu_button);

    /* Create Notification Button */
    notification_button = XCreateSimpleWindow(display, taskbar_win, 100, 5, 80, 30,
                                              0, BlackPixel(display, screen), BUTTON_BG_COLOR);
    if (!notification_button) {
        fprintf(stderr, "Error creating notification button.\n");
        cleanup();
        exit(EXIT_FAILURE);
    }
    XSelectInput(display, notification_button, ButtonPressMask | ExposureMask);
    XMapWindow(display, notification_button);

    /* Create Logout Button for LightDM integration */
    logout_button = XCreateSimpleWindow(display, taskbar_win, 190, 5, 80, 30,
                                        0, BlackPixel(display, screen), BUTTON_BG_COLOR);
    if (!logout_button) {
        fprintf(stderr, "Error creating logout button.\n");
        cleanup();
        exit(EXIT_FAILURE);
    }
    XSelectInput(display, logout_button, ButtonPressMask | ExposureMask);
    XMapWindow(display, logout_button);

    /* Create Clock Window at the far right */
    clock_win = XCreateSimpleWindow(display, taskbar_win,
                                    display_width - CLOCK_WIDTH - 10, 5,
                                    CLOCK_WIDTH, 30,
                                    0, BlackPixel(display, screen), TASKBAR_BG_COLOR);
    if (!clock_win) {
        fprintf(stderr, "Error creating clock window.\n");
        cleanup();
        exit(EXIT_FAILURE);
    }
    XSelectInput(display, clock_win, ExposureMask);
    XMapWindow(display, clock_win);

    /* Create a graphics context for drawing */
    gc = XCreateGC(display, taskbar_win, 0, NULL);
    if (!gc) {
        fprintf(stderr, "Error creating graphics context.\n");
        cleanup();
        exit(EXIT_FAILURE);
    }
}

/* Draw the current system time in the clock window */
void draw_clock(void) {
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    if (!local) return;

    char time_str[64];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", local);

    /* Clear clock area */
    XSetForeground(display, gc, TASKBAR_BG_COLOR);
    XFillRectangle(display, clock_win, gc, 0, 0, CLOCK_WIDTH, 30);

    /* Draw time in white */
    XSetForeground(display, gc, 0xFFFFFF);
    XDrawString(display, clock_win, gc, 10, 20, time_str, strlen(time_str));
    XFlush(display);
}

/* Display a popup app menu (dummy implementation) */
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

/* Display a simple notification popup */
void show_notifications(void) {
    Window notif_win = XCreateSimpleWindow(display, root, 150, 150, 200, 100, 2,
                                           BlackPixel(display, screen), 0xDDDDDD);
    if (!notif_win) {
        fprintf(stderr, "Error creating notification window.\n");
        return;
    }
    XSelectInput(display, notif_win, ExposureMask | ButtonPressMask);
    XMapWindow(display, notif_win);
    /* You might later add more comprehensive notification handling here */
}

/* Trigger LightDM logout/switch-to-greeter using dm-tool */
void lightdm_logout(void) {
    if (fork() == 0) {
        execlp("dm-tool", "dm-tool", "switch-to-greeter", NULL);
        perror("Failed to execute dm-tool");
        exit(EXIT_FAILURE);
    }
}

/* Main event loop: handle expose events, button clicks, and update the clock every second */
void event_loop(void) {
    XEvent ev;
    while (1) {
        /* Process all pending events */
        while (XPending(display)) {
            XNextEvent(display, &ev);
            if (ev.type == Expose) {
                if (ev.xexpose.window == taskbar_win) {
                    /* Redraw the taskbar */
                    XFillRectangle(display, taskbar_win, gc, 0, 0,
                                   DisplayWidth(display, screen), TASKBAR_HEIGHT);
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
                    /* Clicking inside the popup app menu will close it */
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

/* The main function initializes everything and enters the event loop */
int main(void) {
    /* Open connection to the X server */
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Error: Unable to open display.\n");
        return EXIT_FAILURE;
    }
    screen = DefaultScreen(display);
    root = RootWindow(display, screen);
    if (!root) {
        fprintf(stderr, "Error: Unable to get root window.\n");
        cleanup();
        return EXIT_FAILURE;
    }
    
    /* Setup desktop environment components */
    setup_cursor();
    set_wallpaper();
    create_taskbar();

    /* Start handling events */
    event_loop();

    cleanup();
    return EXIT_SUCCESS;
}
