#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>

GtkWidget *window, *notebook;

void change_wallpaper() {
    system("nitrogen --restore");
}

void show_system_info(GtkWidget *button, gpointer data) {
    struct sysinfo info;
    sysinfo(&info);

    char system_info[512];
    snprintf(system_info, sizeof(system_info), "Uptime: %ld seconds\nRAM: %ld MB\n", info.uptime, info.totalram / (1024 * 1024));

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", system_info);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void setup_ui(GtkWidget *window) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

    GtkWidget *settings_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *wallpaper_button = gtk_button_new_with_label("Change Wallpaper");
    g_signal_connect(wallpaper_button, "clicked", G_CALLBACK(change_wallpaper), NULL);
    
    GtkWidget *sysinfo_button = gtk_button_new_with_label("System Info");
    g_signal_connect(sysinfo_button, "clicked", G_CALLBACK(show_system_info), NULL);

    gtk_box_pack_start(GTK_BOX(settings_tab), wallpaper_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(settings_tab), sysinfo_button, FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), settings_tab, gtk_label_new("General Settings"));

    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show_all(window);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Flow Settings");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 400);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    setup_ui(window);

    gtk_main();
    return 0;
}
