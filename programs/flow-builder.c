#include <gtk/gtk.h>
#include <gtksourceview/gtksourceview.h>
#include <vte/vte.h>
#include <stdlib.h>

#define REPO_URL "https://github.com/superuser-pushexe/Flow-Desktop.git"
#define PROGRAM_DIR "./flow_programs"

GtkWidget *window, *notebook, *terminal;

void setup_flow_desktop() {
    printf("Checking for Flow Desktop programs...\n");

    // Clone the repository if it doesn't exist
    if (system("test -d " PROGRAM_DIR) != 0) {
        printf("Downloading Flow Desktop programs...\n");
        
        if (system("git --version > /dev/null 2>&1") == 0) {
            system("git clone " REPO_URL " " PROGRAM_DIR);
        } else {
            printf("Git is not installed. Installing...\n");
            system("sudo apt install git -y");
            system("git clone " REPO_URL " " PROGRAM_DIR);
        }
        
        // Set permissions and run setup script (if available)
        system("cd " PROGRAM_DIR "/programs && chmod +x setup.sh");
        if (system("test -f " PROGRAM_DIR "/programs/setup.sh") == 0) {
            system(PROGRAM_DIR "/programs/setup.sh");
        } else {
            printf("No setup script found. Dependencies may need manual installation.\n");
        }

        printf("Flow Desktop programs setup complete!\n");
    } else {
        printf("Flow Desktop programs already installed.\n");
    }
}

void create_editor_tab(const char *title) {
    GtkWidget *scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *source_view = gtk_source_view_new();
    
    GtkSourceBuffer *source_buffer = gtk_source_buffer_new(NULL);
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(source_view), GTK_TEXT_BUFFER(source_buffer));
    
    gtk_container_add(GTK_CONTAINER(scrolled_win), source_view);
    
    GtkWidget *tab_label = gtk_label_new(title);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolled_win, tab_label);
    gtk_widget_show_all(scrolled_win);
}

void setup_terminal() {
    terminal = vte_terminal_new();
    vte_terminal_spawn_async(VTE_TERMINAL(terminal), VTE_PTY_DEFAULT, NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL);
    GtkWidget *scrolled_term = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_term), terminal);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolled_term, gtk_label_new("Terminal"));
    gtk_widget_show_all(scrolled_term);
}

void setup_ui(GtkWidget *window) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

    setup_terminal();
    create_editor_tab("New File");

    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show_all(window);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Flow Builder");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    setup_flow_desktop();  // Automatically download & setup Flow Desktop programs
    setup_ui(window);

    gtk_main();
    return 0;
}
