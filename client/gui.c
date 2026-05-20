#include "gui.h"
#include <gtk/gtk.h>
#include <stdio.h>

// CSS Aggiornato per ingrandire bottoni e testo
const char *cinema_css =
    "label.screen-label {"
    "  font-weight: bold;"
    "  font-size: 24px;" /* Ingrandito leggermente anche lo schermo */
    "  color: #333;"
    "  margin-bottom: 30px;" /* Più spazio sotto lo schermo */
    "}"
    ""
    "grid.cinema-grid {"
    "  margin: 15px;"
    "}"
    ""
    "button.seat-button {"
    "  background-image: none;"
    "  background-color: #2ec27e;"
    "  color: white;"
    "  font-weight: bold;"
    "  border-radius: 10px;" /* Bordi leggermente più tondi */
    "  border: 1px solid #1a9c61;"
    "  transition: all 0.2s ease-in-out;"
    ""
    /* --- MODIFICHE PER INGRANDIRE --- */
    "  font-size: 20px;"  /* Scritta molto più grande (prima era default ~14px) */
    "  padding: 0;"       /* Resettiamo il padding per usare dimensioni fisse */
    "  min-width: 70px;"  /* Larghezza minima forzata del bottone */
    "  min-height: 70px;" /* Altezza minima forzata del bottone */
    /* -------------------------------- */
    "}"
    ""
    "button.seat-button:hover {"
    "  background-color: #26a269;"
    "}"
    ""
    "button.seat-button:checked {"
    "  background-color: #e01b24;"
    "  border-color: #a51d2d;"
    "}"
    ""
    "button.seat-button:checked:hover {"
    "  background-color: #c01c28;"
    "}";

void apply_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, cinema_css, -1);

    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_object_unref(provider);
}

void on_seat_toggled(GtkToggleButton *btn, gpointer user_data) {
    const char *seat_label = gtk_button_get_label(GTK_BUTTON(btn));

    if (gtk_toggle_button_get_active(btn)) {
        g_print("Posto %s PRENOTATO.\n", seat_label);
    } else {
        g_print("Posto %s reso DISPONIBILE.\n", seat_label);
    }
}

void on_activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *screen_label;
    GtkWidget *grid;

    apply_css();

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Cinema Garofolo Favero - Prenotazione Posti");

    // --- MODIFICA: Finestra più grande ---
    // Avendo ingrandito i posti, la finestra deve essere più grande (es. 900x800)
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 800);
    // -------------------------------------

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(vbox, GTK_ALIGN_CENTER);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    screen_label = gtk_label_new("----------------  SCHERMO  ----------------");
    gtk_widget_add_css_class(screen_label, "screen-label");
    gtk_box_append(GTK_BOX(vbox), screen_label);

    grid = gtk_grid_new();
    // Aumentato leggermente lo spazio tra i posti
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_widget_add_css_class(grid, "cinema-grid");
    gtk_box_append(GTK_BOX(vbox), grid);

    int righe = 6;
    int colonne = 10;

    for (int r = 0; r < righe; r++) {
        for (int c = 0; c < colonne; c++) {
            char seat_name[10];
            snprintf(seat_name, sizeof(seat_name), "%c%d", 'A' + r, c + 1);

            GtkWidget *btn = gtk_toggle_button_new_with_label(seat_name);
            gtk_widget_add_css_class(btn, "seat-button");
            gtk_grid_attach(GTK_GRID(grid), btn, c, r, 1, 1);
            g_signal_connect(btn, "toggled", G_CALLBACK(on_seat_toggled), NULL);
        }
    }

    gtk_window_present(GTK_WINDOW(window));
}

int initGUI(void) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.garo.garofolo_favero", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

    status = g_application_run(G_APPLICATION(app), 0, NULL);

    g_object_unref(app);

    return status;
}