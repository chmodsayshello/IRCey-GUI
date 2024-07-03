#pragma once
#include <gtk/gtk.h>
#include <stdlib.h>

extern GtkWidget* mainWindow;
extern GtkWidget* servertree;
extern GtkWidget* users;
extern GtkWidget* chat;
//extern GtkWidget* state_selector;

struct connections {
    struct ircClient** clients; // an array!
    unsigned char amount;
    FILE** history;
};

extern struct connections* connections;