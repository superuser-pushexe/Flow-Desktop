// flow-builder.c - GTK4-based IDE for Flow Desktop

#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include <vte/vte.h>
#include <unistd.h>
#include <git2.h>

#define REPO_URL "https://github.com/superuser-pushexe/Flow-Desktop.git"
#define PROGRAM_DIR "./flow_programs"

static void clone_repo(const char *url, const char *path) {
    git_repository *repo = NULL;
    git_clone_options opts = GIT_CLONE_OPTIONS_INIT;
    if (git_clone(&repo, url, path, &opts) == 0 && repo) {
        git_repository_free(repo);
    } else {
        fprintf(stderr, "Failed to clone repository\n");
    }
}

static void setup_flow_desktop() {
    if (access(PROGRAM_DIR, F_OK) != 0) {
        printf("Downloading Flow Desktop programs...\n");
        git_libgit2_init();
        clone_repo(REPO_URL, PROGRAM_DIR);
        git_libgit2_shutdown();

        char setup_cmd[512];
        snprintf(setup_cmd, sizeof(setup_cmd), "cd %s/programs && chmod +x setup.sh && ./setup.sh", PROGRAM_DIR);
        if (system(setup_cmd) != 0) {
            printf("No setup script found or failed to execute\n");
        }
        printf("Flow Desktop programs setup complete!\n");
    } else {
        printf("Flow Desktop programs already installed.\n");
    }
}

static void setup_terminal(GtkWidget *notebook) {
    GtkWidget *terminal = vte_terminal_new();
    vte_terminal_spawn_async(VTE_TERMINAL(terminal), VTE_PTY_DEFAULT, NULL,
                             (char *[]){"/bin/bash", NULL}, NULL, G_SPAWN_DEFAULT,
                             NULL, NULL, NULL, -1, NULL, NULL, NULL);
    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), terminal);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolled, gtk_label_new("Terminal"));
}

static void create_editor_tab(GtkWidget *notebook, const char *title) {
    GtkWidget *scrolled = gtk_scrolled_window_new();
    GtkWidget *view = gtk_source_view_new();
    GtkSourceBuffer *buffer = gtk_source_buffer_new(NULL);
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(view), GTK_TEXT_BUFFER(buffer));
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), view);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolled, gtk_label_new(title));
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Flow Builder");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *notebook = gtk_notebook_new();
    gtk_box_append(GTK_BOX(box), notebook);

    create_editor_tab(notebook, "New File");
    setup_terminal(notebook);

    gtk_window_set_child(GTK_WINDOW(window), box);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new("org.flow.builder", G_APPLICATION_DEFAULT_FLAGS);
    if (!gtk_init_check()) {
        fprintf(stderr, "Failed to initialize GTK\n");
        return 1;
    }
    setup_flow_desktop();
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
