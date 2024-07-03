#pragma once

#include <gtk/gtk.h>
#include <stdint.h>
#include "irc.h"

GtkWidget* create_main(void);
void create_new_server_dialog(void);
void create_new_channel_dialog(void);
void create_about_dialog(void);
void add_line_to_chat_no_newline(const char* line);
void add_line_to_chat(const char* line);
void set_name_list(const char** names, unsigned short amount);
void show_context_user_menu(GtkWidget* name);
void add_channel_to_tree(const char* channelname);
gchar* getCurrentChannelName(void);
void clearChat(void);
void notifyMention(const char* author, const char* channel);

struct ircClient* getCurrentClient(void);
FILE* getCurrentFile(void);
static uint8_t currentClientIndex(void);