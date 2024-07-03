#include <gtk/gtk.h>

#include "gui.h"
#include "shared.h"
#include "util.h"
#include "irc.h"

GtkWidget* mainWindow;
struct connections* connections;

char* buffer;

int ircMain(void* agrs) {
    for (unsigned short i = 0; i < connections->amount; i++) {
        struct ircClient* client = connections->clients[i];
        if (client->connected) {
            tick(client, buffer);
        }
    }
    return G_SOURCE_CONTINUE;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    
    connections = create_connections();
    buffer = newBuffer();
    mainWindow = create_main();
    g_timeout_add(750, ircMain, NULL); // check for new messages all 0,75 seconds
    gtk_widget_show_all(mainWindow);

    gtk_main();
    free(buffer);
    free_connections(connections);
    return 0;
}