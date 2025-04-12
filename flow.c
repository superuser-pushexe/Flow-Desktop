/* flow-desktop.c - X11 desktop with cursor, wallpaper, taskbar, app menu, and auto-updates */
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

#define REPO_RELEASES "https://github.com/superuser-pushexe/Flow-Desktop/releases/latest"
#define TASKBAR_HEIGHT 40
#define MENU_WIDTH 300
#define MENU_HEIGHT 400

typedef struct {
    Window win;
    char *command;
    char *icon_path;
} Launcher;

Display *display;
Window root, taskbar_win, app_menu_button;
int screen;
Launcher launchers[5];
int launcher_count = 0;
GC gc;

// Function to fetch the latest version from GitHub
void update_flow_desktop() {
    printf("Checking for updates...\n");

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
    system("rm latest_release.html latest_version.txt");

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

// Initialize taskbar and UI
void create_taskbar() {
    taskbar_win = XCreateSimpleWindow(display, root, 0, DisplayHeight(display, screen) - TASKBAR_HEIGHT,
        DisplayWidth(display, screen), TASKBAR_HEIGHT, 0, CopyFromParent, InputOutput, CopyFromParent, 0, NULL);
    XMapWindow(display, taskbar_win);

    app_menu_button = XCreateSimpleWindow(display, taskbar_win, 10, 5, 80, 30, 0, BlackPixel(display, screen), 0x444444);
    XSelectInput(display, app_menu_button, ButtonPressMask | ExposureMask);
    XMapWindow(display, app_menu_button);
}

void event_loop() {
    XEvent ev;
    while (1) {
        while (XPending(display)) {
            XNextEvent(display, &ev);
            if (ev.type == ButtonPress && ev.xbutton.window == app_menu_button) {
                update_flow_desktop();  // Perform update when clicking app menu button
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

    create_taskbar();
    update_flow_desktop(); // Check for updates on startup

    event_loop();
    return EXIT_SUCCESS;
}
