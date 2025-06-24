// safe guard to prevent compilation on non-Linux platforms
#ifdef __linux__

#include "../FileDialogs.h"
#include <gtk/gtk.h>

namespace Utils {
    std::string FileDialogs::OpenFile(const char* title, const char* filter, GLFWwindow* window) {
        if (!gtk_init_check(nullptr, nullptr)) {
            return "";
        }

        GtkWidget* dialog = gtk_file_chooser_dialog_new(title,
            nullptr,
            GTK_FILE_CHOOSER_ACTION_OPEN,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Open", GTK_RESPONSE_ACCEPT,
            nullptr);

        if (window) {
            gtk_window_set_transient_for(GTK_WINDOW(dialog), nullptr);
        }

        // Add file filters
        GtkFileFilter* fileFilter = gtk_file_filter_new();
        gtk_file_filter_set_name(fileFilter, filter);
        gtk_file_filter_add_pattern(fileFilter, "*.*");
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), fileFilter);

        std::string result;
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            if (filename) {
                result = filename;
                g_free(filename);
            }
        }

        gtk_widget_destroy(dialog);
        while (gtk_events_pending())
            gtk_main_iteration();
        return result;
    }

    std::string FileDialogs::SaveFile(const char* title, const char* filter, GLFWwindow* window) {
        if (!gtk_init_check(nullptr, nullptr)) {
            return "";
        }

        GtkWidget* dialog = gtk_file_chooser_dialog_new(title,
            nullptr,
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Save", GTK_RESPONSE_ACCEPT,
            nullptr);

        if (window) {
            gtk_window_set_transient_for(GTK_WINDOW(dialog), nullptr);
        }

        // Add file filters
        GtkFileFilter* fileFilter = gtk_file_filter_new();
        gtk_file_filter_set_name(fileFilter, filter);
        gtk_file_filter_add_pattern(fileFilter, "*.*");
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), fileFilter);

        std::string result;
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            if (filename) {
                result = filename;
                g_free(filename);
            }
        }

        gtk_widget_destroy(dialog);
        while (gtk_events_pending())
            gtk_main_iteration();
        return result;
    }
}

#endif // __linux__