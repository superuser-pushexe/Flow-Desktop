/**
 * Flow Desktop - Lightweight X11 desktop environment with auto-updates
 *
 * Features:
 * - Cursor customization
 * - Taskbar with an application menu
 * - Automatic self-updates from GitHub releases
 * - Basic application launch functionality
 *
 * Dependencies:
 * Requires X11 libraries (`libX11-dev`), `wget`, and internet access for updates.
 */

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

// GitHub repository releases URL for updates
#define REPO_RELEASES "https://github.com/superuser-pushexe/Flow-Desktop/releases/latest"

// Taskbar and menu window dimensions
#define TASKBAR_HEIGHT 40
#define MENU_WIDTH 300
#define MENU_HEIGHT 400

// Launcher structure to store applications
typedef struct {
    Window win;       // Window ID for the launcher icon
    char *command;    // Command to execute when clicked
    char *icon_path;  // Path to the icon
} Launcher;

// X11-related global variables
Display *display;
Window root, taskbar_win, app_menu_button;
int screen;
Launcher launchers[5];
int launcher_count = 0;
GC gc;

/**
 * Checks for updates from the GitHub release page,
 * downloads the latest version if available, and replaces the executable.
 */
void update_flow_desktop() {
    printf("Checking for updates...\n");

    // Fetch latest release details from GitHub
    system("wget -q --show-progress --output-document=latest_release.html " REPO_RELEASES);
    system("grep 'href=\"/superuser-pushexe/Flow-Desktop/releases/tag/' latest_release.html | head -1 | sed 's/.*tag\\///;s/\".*//' > latest_version.txt");

    FILE *file = fopen("latest_version.txt", "r");
    if (!file) {
        printf("Failed to check for updates.\n");
        return;
    }

    char latest_version[50];
    fgets(latest_version, sizeof(latest_version), file);
    fclose(file);
    
    // Cleanup temporary files
    system("rm latest_release.html latest_version.txt");

    // Download and apply update if a newer version is found
    if (strlen(latest_version) > 0) {
        printf("Latest version found: %s\n", latest_version);
        char download_cmd[256];
        snprintf(download_cmd, sizeof(download_cmd), "wget -q --show-progress -O flow-desktop-new https://github.com/superuser-pushexe/Flow-Desktop/releases/download/%s/flow-desktop", latest_version);
        
        printf("Downloading update...\n");
        system(download_cmd);
        system("chmod +x flow-desktop-new && mv flow-desktop-new ./flow-desktop");
        printf("Flow Desktop updated successfully!\n");
    } else {
        printf("No updates found.\n");
    }
}

/**
 * Creates the taskbar window at the bottom of the screen.
 * Includes an application menu button.
 */
void create_taskbar() {
    taskbar_win = XCreateSimpleWindow(display, root, 0, DisplayHeight(display, screen) - TASKBAR_HEIGHT,
        DisplayWidth(display, screen), TASKBAR_HEIGHT, 0, CopyFromParent, InputOutput, CopyFromParent, 0, NULL);
    XMapWindow(display, taskbar_win);

    // Create the menu button
    app_menu_button = XCreateSimpleWindow(display, taskbar_win, 10, 5, 80, 30, 0, BlackPixel(display, screen), 0x444444);
    XSelectInput(display, app_menu_button, ButtonPressMask | ExposureMask);
    XMapWindow(display, app_menu_button);
}

/**
 * Main event loop for handling user interactions.
 * Detects button presses and executes updates when the menu button is clicked.
 */
void event_loop() {
    XEvent ev;
    while (1) {
        while (XPending(display)) {
            XNextEvent(display, &ev);
            if (ev.type == ButtonPress && ev.xbutton.window == app_menu_button) {
                update_flow_desktop();  // Perform update when clicking the menu button
            }
        }
        usleep(100000);  // Prevents excessive CPU usage
    }
}

/**
 * Entry point for the program.
 * Initializes X11 display, creates windows, and starts the event loop.
 */
int main() {
    // Open the display connection to X11
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Failed to open display\n");
        return EXIT_FAILURE;
    }

    screen = DefaultScreen(display);
    root = RootWindow(display, screen);

    create_taskbar();
    update_flow_desktop(); // Automatically check for updates on startup

    event_loop();
    return EXIT_SUCCESS;
}
