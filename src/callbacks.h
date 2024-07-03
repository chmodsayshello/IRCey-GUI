
#pragma once
#include <gtk/gtk.h>
#include <stdint.h>

#include "irc.h"

// ######## GUI CALLBACKS ################
void GUI_on_add_server(GtkWidget* widget, gpointer data);
void GUI_on_connect_server(GtkWidget* widget, gpointer data);
void GUI_on_disconnect_server(GtkWidget* widget, gpointer data);
void GUI_on_chat_save(GtkWidget* widget, gpointer data);
void GUI_on_about(GtkWidget* widget, gpointer data);
void GUI_on_new_server_dialog_confirm(const char* adress, const unsigned short port, const char* username);
void GUI_on_message_enter(GtkWidget* widget, gpointer data);
void GUI_on_select_user(GtkListBox *box, GtkListBoxRow *row, gpointer user_data);
void GUI_on_message_user_context(GtkButton* button, gpointer data);
void GUI_on_whois_user_context(GtkButton* button, gpointer data);
void GUI_on_select_channel(GtkTreeView* stree, gpointer data);
void GUI_on_channel_add(GtkWidget* widget, gpointer data);
void GUI_on_new_channel_dialog_confirm(char* channel);

// ####### IRC CALLBACKS ###############
void IRConMessage(const struct ircClient* client, const char* channel, const char* author, const char* message);
void IRConMODT (const struct ircClient* client, const char* MODTLine);
void IRConNamesList (const struct ircClient* client, struct nameList list);
void IRConUserJoin (const struct ircClient* client, const char* channel, const char* user);
void IRConUserLeave (const struct ircClient* client, const char* channel, const char* user, const char* reason);
void IRConUserWhoIsInfo (const struct ircClient* client, const char* nick, const char* user, const char* realname, const char* hostname);
void IRConFatalFailure(const struct ircClient* client, const uint8_t failure);