// flow-settings.c - GTK4-based settings UI for Flow Desktop

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <sys/resource.h>

static void change_wallpaper(GtkWidget *button, gpointer window) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Wallpaper", GTK_WINDOW(window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Open", GTK_RESPONSE_ACCEPT, NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        GSettings *settings = g_settings_new("org.gnome.desktop.background");
        char *uri = g_filename_to_uri(filename, NULL, NULL);
        g_settings_set_string(settings, "picture-uri", uri);
        g_free(filename);
        g_free(uri);
        g_object_unref(settings);
    }
    gtk_widget_destroy(dialog);
}

static void show_system_info(GtkWidget *button, gpointer window) {
    struct rusage usage;
    char system_info[512];
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        snprintf(system_info, sizeof(system_info), "Memory: %ld KB\n", usage.ru_maxrss);
    } else {
        snprintf(system_info, sizeof(system_info), "Error retrieving system info\n");
    }

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", system_info);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Flow Settings");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 400);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_window_set_child(GTK_WINDOW(window), box);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_box_append(GTK_BOX(box), notebook);

    GtkWidget *settings_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *wallpaper_button = gtk_button_new_with_label("Change Wallpaper");
    g_signal_connect(wallpaper_button, "clicked", G_CALLBACK(change_wallpaper), window);
    gtk_box_append(GTK_BOX(settings_tab), wallpaper_button);

    GtkWidget *sysinfo_button = gtk_button_new_with_label("System Info");
    g_signal_connect(sysinfo_button, "clicked", G_CALLBACK(show_system_info), window);
    gtk_box_append(GTK_BOX(settings_tab), sysinfo_button);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), settings_tab, gtk_label_new("General Settings"));

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new("org.flow.settings", G_APPLICATION_DEFAULT_FLAGS);
    if (!gtk_init_check()) {
        fprintf(stderr, "Failed to initialize GTK\n");
        return 1;
    }
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
