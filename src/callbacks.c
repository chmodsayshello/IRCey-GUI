// Implementation of
#include "callbacks.h"

//Other headers
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "gui.h"
#include "irc.h"
#include "shared.h"
#include "util.h"

// Set IRC callbacks
void (*onMessage)(const struct ircClient* client, const char* channel, const char* author, const char* message) = &IRConMessage;
void (*onMODT)(const struct ircClient* client, const char* MODTLine) = &IRConMODT;
void (*onNamesList)(const struct ircClient* client, struct nameList list) = &IRConNamesList;
void (*onUserJoin)(const struct ircClient* client, const char* channel, const char* user) = &IRConUserJoin;
void (*onUserLeave)(const struct ircClient* client, const char* channel, const char* user, const char* reason) = &IRConUserLeave;
void (*onUserWhoIsInfo)(const struct ircClient* client, const char* nick, const char* user, const char* realname, const char* hostname) = &IRConUserWhoIsInfo;
void (*onFatalFailure)(const struct ircClient* client, const uint8_t failure) = &IRConFatalFailure;

//Helpers
FILE* getFileFromClient(const struct ircClient* client) {
    for (unsigned char index = 0; index < connections->amount; index++) {
        if (connections->clients[index] == client) {
            return connections->history[index];
        }
    }
    return NULL;
}

void commonChatMessageLogic(const struct ircClient* client, const char* channel, const char* author, const char* message) {
    gchar* currentChannelName = getCurrentChannelName();
    char* formattedChatMessage = formatChatMessage(author, message);
    if (currentChannelName != NULL && strcmp(channel, (char*) currentChannelName) == 0) { //TODO: handle channels with the same name on different servers
        add_line_to_chat(formattedChatMessage);
        if (strstr(message, client->username) != NULL) {
            notifyMention(author, channel);
        }
    }
    FILE* tmpfile = getFileFromClient(client);
    assert(tmpfile != NULL);
    fprintf(tmpfile, "C%s!%s\n", channel, formattedChatMessage);
    fflush(tmpfile);

    g_free(currentChannelName);
    free(formattedChatMessage);
}

//Implementations of GUI callbacks
void GUI_on_add_server(GtkWidget* widget, gpointer data) {
    create_new_server_dialog();
}

void GUI_on_connect_server(GtkWidget* widget, gpointer data) {
    //puts("connect clicked!");
}

void GUI_on_disconnect_server(GtkWidget* widget, gpointer data) {
    //puts("disconnect clicked!");
}

void GUI_on_chat_save(GtkWidget* widget, gpointer data) {
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Save chat.",
        GTK_WINDOW(mainWindow),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "Save", GTK_RESPONSE_ACCEPT,
        "Cancel", GTK_RESPONSE_CANCEL,
        NULL
    );

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_CANCEL) {
        gtk_widget_destroy(dialog);
        return;
    }

    GFile* fileC = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
    gtk_widget_destroy(dialog);
    
    FILE* file = fopen(g_file_get_path(fileC), "ab");
    GtkTextIter start, end;
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(chat));
    gtk_text_buffer_get_bounds(buffer, &start, &end);

    fputs(gtk_text_buffer_get_text(buffer, &start, &end, FALSE), file);
    fclose(file);
}

void GUI_on_about(GtkWidget* widget, gpointer data) {
    create_about_dialog();
}

void GUI_on_new_server_dialog_confirm(const char* adress, const unsigned short port, const char* username) {
    connections->amount++;
    connections->clients = realloc(connections->clients, sizeof(struct ircClient*) * connections->amount);
    connections->clients[connections->amount - 1] = newClient(username);

    connectIRC(connections->clients[connections->amount - 1], adress, port);

    connections->history = realloc(connections->history, sizeof(FILE*) * connections->amount);
    connections->history[connections->amount - 1] = tmpfile();
}

void GUI_on_channel_add(GtkWidget* widget, gpointer data) {
    if (connections->amount == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mainWindow),
                                GTK_DIALOG_MODAL,
                                GTK_MESSAGE_ERROR,
                                GTK_BUTTONS_OK,
                                "Connect to a server before joining a channel!"
        );
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    create_new_channel_dialog();
}

void GUI_on_new_channel_dialog_confirm(char* channel) {
    joinChannel(getCurrentClient(), channel);
    add_channel_to_tree(channel);
}

void GUI_on_message_enter(GtkWidget* widget, gpointer data) {
    gchar* channel = getCurrentChannelName();
    if (channel == NULL) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mainWindow),
                                        GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_OK,
                                        "No Channel selected!"
        );
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    const struct ircClient* client = getCurrentClient();
    const char* message = (const char*) gtk_entry_buffer_get_text(gtk_entry_get_buffer(GTK_ENTRY(widget)));
    sendMessage(client, getCurrentChannelName() , message);
    commonChatMessageLogic(client, channel, client->username, message);
    gtk_entry_buffer_set_text(gtk_entry_get_buffer(GTK_ENTRY(widget)), "", -1);
    g_free(channel);
}

void GUI_on_select_user(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    GtkWidget* label = gtk_bin_get_child(GTK_BIN(row));
    #ifdef DEBUG
        puts(gtk_label_get_label(GTK_LABEL(label)));
    #endif
    show_context_user_menu(label);
}

void GUI_on_message_user_context(GtkButton* button, gpointer data) {
    if (data == NULL) { return; }
    GtkPopover* popup = GTK_POPOVER(gtk_widget_get_parent(GTK_WIDGET(button)));
    gtk_popover_popdown(popup);
    if (*((char*) data) == '@') {
        add_channel_to_tree(((char*) data) + 1);
    } else {
        add_channel_to_tree((char*) data);
    }
}

void GUI_on_whois_user_context(GtkButton* button, gpointer data) {
    if (data == NULL) { return; }
    GtkPopover* popup = GTK_POPOVER(gtk_widget_get_parent(GTK_WIDGET(button)));
    gtk_popover_popdown(popup);
    char* name = (char*) data;
    if (*name == '@') { name++; }

    requestUserInformation(getCurrentClient(), name);
}

void GUI_on_select_channel(GtkTreeView* stree, gpointer data) {
    GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(servertree)));
    GtkTreeIter iter;

    gchar *selected_string;

    if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(servertree)), NULL, &iter)) { return; }
    if (gtk_tree_store_iter_depth(store, &iter) < 1) {
        selected_string = g_malloc(1);
        *selected_string = '\0';
    } else {
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &selected_string, -1);
    }

    clearChat();
    FILE* file = getCurrentFile();
    assert(file != NULL);
    const size_t size = IRC_MAX_BUFFER + 10;
    char* buff = malloc(size);
    char* contents = buff + 1;
    rewind(file);
    while(fgets(buff, size, file)) {
        switch (*buff) {
            case 'M':
                add_line_to_chat_no_newline(contents);
            break;

            case 'C':
                char* message = strchr(buff, '!');
                assert(message != NULL);
                *message = '\0';
                message++;

                if (strcmp(selected_string, contents) != 0) { break; }

                add_line_to_chat_no_newline(message);
            break;
        }
    }

    free(buff);
    g_free(selected_string);
}

//Implementations of IRC callbacks
void IRConMessage(const struct ircClient* client, const char* channel, const char* author, const char* message) {
    commonChatMessageLogic(client, channel, author, message);
}

void IRConMODT (const struct ircClient* client, const char* MODTLine) {
    const struct ircClient* currClient = getCurrentClient();
    FILE* file = getFileFromClient(client);
    assert(file != NULL);
    
    fprintf(file, "M%s\n", MODTLine);
    fflush(file);

    if (currClient == NULL || currClient != client) {
        return;
    }
    add_line_to_chat(MODTLine);
}

void IRConNamesList (const struct ircClient* client, struct nameList list) {
    set_name_list(list.names, list.amount);
}

void IRConUserJoin (const struct ircClient* client, const char* channel, const char* user) {
    char* cn = getCurrentChannelName();
    if (cn != NULL && strcmp(cn, channel) != 0) { return; }
    size_t max = 6 + strlen(user) + strlen(channel);
    char msg[max];
    snprintf(msg, max, "[+] %s %s", user, channel);

    add_line_to_chat(msg);
}

void IRConUserLeave (const struct ircClient* client, const char* channel, const char* user, const char* reason) {
    assert(channel != NULL);
    const char* currentChannel = getCurrentChannelName();
    if (currentChannel == NULL || (strcmp(currentChannel, channel) != 0 && *channel != '\0')) { return; }
    size_t max = 7 + strlen(user) + strlen(channel) + strlen(reason);
    char msg[max];
    snprintf(msg, max, "[-] %s %s %s", user, channel, reason);

    add_line_to_chat(msg);
}

void IRConUserWhoIsInfo (const struct ircClient* client, const char* nick, const char* user, const char* realname, const char* hostname) {
    size_t size = 58 + (strlen(nick) * 2) + strlen(user) + strlen(realname) + strlen(hostname);
    char message[size];
    snprintf(message, size, "Information on :%s\nNick: %s\nUsername: %s\nReal Name: %s\nHostname: %s", nick, nick, user, realname, hostname);
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mainWindow),
                            GTK_DIALOG_MODAL,
                            GTK_MESSAGE_INFO,
                            GTK_BUTTONS_OK,
                            (gchar*) message
    );
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void IRConFatalFailure(const struct ircClient* client, const uint8_t failure) {
    char* message;
    switch (failure) {
        case FAILURE_IP_LOOKUP_FAILED:
            message = "Unable to look up the ip adress of the server!";
        break;
        
        case FAILURE_UNEXPECTED_QUIT:
            message = "Connection closed unexpectedly!";
        break;
        
        default:
            message = "An unknown error occured!\nThe program will attempt to resume like normal, although something likely is going to go wrong!";
        break;
    }

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mainWindow),
                            GTK_DIALOG_MODAL,
                            GTK_MESSAGE_ERROR,
                            GTK_BUTTONS_OK,
                            (gchar*) message
    );
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}