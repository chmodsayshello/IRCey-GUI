//implementation of
#include "gui.h"

//other headers
#include <gtk/gtk.h>
#include <gtk/gtkimage.h>

#include "callbacks.h"
#include "shared.h"

#define VERTICAL GTK_ORIENTATION_VERTICAL
#define HORIZONTAL GTK_ORIENTATION_HORIZONTAL

GtkWidget* servertree;
GtkWidget* users;
GtkWidget* chat;
GtkWidget* state_selector;

GtkWidget* create_main(void) {
    GtkWidget *main, *box, *toolbar, *message, *contentbox;

    main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main), "IRCeyGUI");
    gtk_window_set_default_size (GTK_WINDOW (main), 505, 400);

    box = gtk_box_new(VERTICAL, 3);
    toolbar = gtk_toolbar_new();
    { // Toolbar
        const GtkIconSize isize = gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar));

        //Server add button
        GtkWidget* tmp_image = gtk_image_new_from_icon_name ("gtk-add", isize);
        gtk_widget_show (tmp_image);
        GtkWidget* element = (GtkWidget*) gtk_tool_button_new(tmp_image, "text");
        g_signal_connect((gpointer) element, "clicked", G_CALLBACK(GUI_on_add_server), NULL);
        gtk_container_add(GTK_CONTAINER(toolbar), element);

        //Channel add button
        tmp_image = gtk_image_new_from_icon_name ("help-faq", isize);
        gtk_widget_show (tmp_image);
        element = (GtkWidget*) gtk_tool_button_new(tmp_image, "text");
        g_signal_connect((gpointer) element, "clicked", G_CALLBACK(GUI_on_channel_add), NULL);
        gtk_container_add(GTK_CONTAINER(toolbar), element);

        //Server connect button
        tmp_image = gtk_image_new_from_icon_name("gtk-connect", isize);
        gtk_widget_show(tmp_image);
        element = (GtkWidget*) gtk_tool_button_new(tmp_image, "connect");
        g_signal_connect((gpointer) element, "clicked", G_CALLBACK(GUI_on_connect_server), NULL);
        gtk_container_add(GTK_CONTAINER(toolbar), element);

        //Server disconnect button
        tmp_image = gtk_image_new_from_icon_name("gtk-disconnect", isize);
        gtk_widget_show(tmp_image);
        element = (GtkWidget*) gtk_tool_button_new(tmp_image, "disconnect");
        g_signal_connect((gpointer) element, "clicked", G_CALLBACK(GUI_on_disconnect_server), NULL);
        gtk_container_add(GTK_CONTAINER(toolbar), element);

        //Save button
        tmp_image = gtk_image_new_from_icon_name("media-floppy", isize);
        gtk_widget_show(tmp_image);
        element = (GtkWidget*) gtk_tool_button_new(tmp_image, "save");
        g_signal_connect((gpointer) element, "clicked", G_CALLBACK(GUI_on_chat_save), NULL);
        gtk_container_add(GTK_CONTAINER(toolbar), element);

        //Seperator
        element = gtk_separator_new(HORIZONTAL);
        gtk_widget_set_hexpand(element, TRUE);
        gtk_container_add(GTK_CONTAINER(toolbar), element);

        //About
        tmp_image = gtk_image_new_from_icon_name("gtk-dialog-info", isize);
        gtk_widget_show(tmp_image);
        element = (GtkWidget*) gtk_tool_button_new(tmp_image, "about");
        g_signal_connect((gpointer) element, "clicked", G_CALLBACK(GUI_on_about), NULL);
        gtk_container_add(GTK_CONTAINER(toolbar), element);
    } // Toolbar end

    gtk_container_add(GTK_CONTAINER(box), toolbar);

    contentbox = gtk_box_new(HORIZONTAL, 3);
    { // Contentbox
  // Create a GtkTreeModel
        GtkTreeStore *store = gtk_tree_store_new(1, G_TYPE_STRING);
        
        servertree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
                    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Connections", renderer, "text", 0, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(servertree), column);
        gtk_widget_set_size_request(servertree, 120, -1);
                //gtk_widget_set_hexpand(servertree, TRUE);
        //gtk_widget_set_vexpand(servertree, TRUE);
        gtk_container_add(GTK_CONTAINER(contentbox), servertree);
        g_signal_connect((gpointer) servertree, "cursor-changed", G_CALLBACK(GUI_on_select_channel), NULL);


        //Chat
        chat = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(chat), FALSE);
        gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(chat), FALSE);
        gtk_widget_set_hexpand(chat, TRUE);
        gtk_widget_set_vexpand(chat, TRUE);

        GtkWidget* scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(scrolledWindow), chat);
        gtk_container_add(GTK_CONTAINER(contentbox), scrolledWindow);

        //Userlist;
        users = gtk_list_box_new();
        scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(scrolledWindow), users);
        gtk_widget_set_size_request(scrolledWindow, 120, -1);
        gtk_container_add(GTK_CONTAINER(contentbox), scrolledWindow);
        g_signal_connect((gpointer) users, "row-activated", G_CALLBACK(GUI_on_select_user), NULL);

    } // Contentbox end
    gtk_container_add(GTK_CONTAINER(box), contentbox);

    {
        message = gtk_entry_new();
        g_signal_connect((gpointer) message, "activate", G_CALLBACK(GUI_on_message_enter), NULL);
    }
    gtk_container_add(GTK_CONTAINER(box), message);

    gtk_container_add(GTK_CONTAINER(main), box);
    g_signal_connect((gpointer) main, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    return main;
} // create_main

void create_new_server_dialog(void) {
    GtkWidget *new_server, *dialog_content_area, *adress_box, *port_box, *username_box, *channel_box, *sizers;
    
    new_server = gtk_dialog_new_with_buttons("Add a new IRC Connection",
        GTK_WINDOW(mainWindow),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Cancel", GTK_RESPONSE_REJECT,
        "Ok", GTK_RESPONSE_ACCEPT,
        NULL
    );

    dialog_content_area = gtk_dialog_get_content_area(GTK_DIALOG(new_server));

    { //First row
        sizers = gtk_box_new(HORIZONTAL, 3);

        gtk_container_add(GTK_CONTAINER(sizers), gtk_label_new("Server Adress Information"));

        adress_box = gtk_entry_new();
        //gtk_entry_set_input_hints(GTK_ENTRY(adress_box),"server ip/domain");
        gtk_container_add(GTK_CONTAINER(sizers), adress_box);

        port_box = gtk_spin_button_new_with_range(0, 65535, 1);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(port_box), IRC_DEFAULT_PORT);
        gtk_container_add(GTK_CONTAINER(sizers), port_box);

        gtk_container_add(GTK_CONTAINER(dialog_content_area), sizers);
    }

    { //Second row
        sizers = gtk_box_new(HORIZONTAL, 2);

        gtk_container_add(GTK_CONTAINER(sizers), gtk_label_new("Username"));

        username_box = gtk_entry_new();
        gtk_widget_set_vexpand_set(username_box, TRUE);
        gtk_container_add(GTK_CONTAINER(sizers), username_box);

        gtk_container_add(GTK_CONTAINER(dialog_content_area), sizers);
    }

    gtk_widget_show_all(new_server);
    gint r_code = gtk_dialog_run(GTK_DIALOG(new_server));

    if (r_code == GTK_RESPONSE_ACCEPT) {
        GtkTreeStore *store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(servertree)));
        GtkTreeIter iter;
        gtk_tree_store_append(store, &iter, NULL);
        gtk_tree_store_set(store, &iter, 0, gtk_entry_buffer_get_text(gtk_entry_get_buffer(GTK_ENTRY(adress_box))), -1);
        gtk_tree_view_set_model(GTK_TREE_VIEW(servertree), GTK_TREE_MODEL(store));
        gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(servertree)), &iter);

        GUI_on_new_server_dialog_confirm(
            gtk_entry_buffer_get_text(gtk_entry_get_buffer(GTK_ENTRY(adress_box))),
            (unsigned short) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(port_box)),
            gtk_entry_buffer_get_text(gtk_entry_get_buffer(GTK_ENTRY(username_box)))
        );
    }

    gtk_widget_destroy(new_server);
} // create_new_server_dialog

void create_new_channel_dialog(void) {
    GtkWidget* new_channel, *dialog_content_area, *sizers, *channel_box;
    new_channel = gtk_dialog_new_with_buttons("Add a new IRC Channel",
        GTK_WINDOW(mainWindow),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Cancel", GTK_RESPONSE_REJECT,
        "Ok", GTK_RESPONSE_ACCEPT,
        NULL
    );

    dialog_content_area = gtk_dialog_get_content_area(GTK_DIALOG(new_channel));

    { //First row
        sizers = gtk_box_new(HORIZONTAL, 2);

        gtk_container_add(GTK_CONTAINER(sizers), gtk_label_new("Channel"));

        channel_box = gtk_entry_new();
        //gtk_entry_set_input_hints(GTK_ENTRY(adress_box),"server ip/domain");
        gtk_container_add(GTK_CONTAINER(sizers), channel_box);


        gtk_container_add(GTK_CONTAINER(dialog_content_area), sizers);
    }

    gtk_widget_show_all(new_channel);
    gint r_code = gtk_dialog_run(GTK_DIALOG(new_channel));

    if (r_code == GTK_RESPONSE_ACCEPT) {
        GUI_on_new_channel_dialog_confirm(
            gtk_entry_buffer_get_text(gtk_entry_get_buffer(GTK_ENTRY(channel_box)))
        );
    }

    gtk_widget_destroy(new_channel);
}


void create_about_dialog(void) {
    gtk_show_about_dialog(GTK_WINDOW(mainWindow),
        "comments", "A simple IRC-Client written in C noteworthy for it's horrible codestyle.",
        "license", "IRCeyGUI is distributed under the GPLv3 License, please see the file LICENSE for details.",
        NULL
    );
} //create_about_dialog

void add_line_to_chat_no_newline(const char* line) {
    GtkTextIter iter;
    GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(chat));
    gtk_text_buffer_get_end_iter(buf, &iter);
    gtk_text_buffer_insert(buf, &iter, line, strlen(line));
} //add_line_to_chat_no_newline

void add_line_to_chat(const char* line) {
    GtkTextIter iter;
    GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(chat));
    gtk_text_buffer_get_end_iter(buf, &iter);
    gtk_text_buffer_insert(buf, &iter, line, strlen(line));
    gtk_text_buffer_insert(buf, &iter, "\n", 1);
} //add_line_to_chat

void set_name_list(const char** names, unsigned short amount) {
    //remove current names;
    GList* list = gtk_container_get_children(GTK_CONTAINER(users));
    GList* iter;
    for(iter = list; iter != NULL; iter = g_list_next(iter)) {
        gtk_container_remove(GTK_CONTAINER(users), GTK_WIDGET(iter->data));
    }
    g_list_free(list);

    //set names
    for (unsigned short i = 0; i < amount; i++) {
        gtk_container_add(GTK_CONTAINER(users), gtk_label_new(names[i])); //TODO: HEADER;
    }
    gtk_widget_show_all(users);
} //set_name_list

void show_context_user_menu(GtkWidget* name) {
    GtkWidget* popup = gtk_popover_new(name);
    GtkWidget* box = gtk_box_new(VERTICAL, 2);
    GtkWidget* messageButton, *whoisButton;

    messageButton = gtk_button_new_with_label("Message");
    g_signal_connect((gpointer) messageButton, "clicked", G_CALLBACK(GUI_on_message_user_context), (gpointer) gtk_label_get_label(GTK_LABEL(name)));
    gtk_container_add(GTK_CONTAINER(box), messageButton);

    whoisButton = gtk_button_new_with_label("Who is?");
    g_signal_connect((gpointer) whoisButton, "clicked", G_CALLBACK(GUI_on_whois_user_context), (gpointer) gtk_label_get_label(GTK_LABEL(name)));
    gtk_container_add(GTK_CONTAINER(box), whoisButton);

    gtk_container_add(GTK_CONTAINER(popup), box);

    gtk_widget_show_all(popup);
    gtk_popover_popup(GTK_POPOVER(popup));
} // show_context_user_menu

void add_channel_to_tree(const char* channelname) {
    GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(servertree)));

    GtkTreeIter iter, newiter;
    if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(servertree)), NULL, &iter)) { return; }
    for (unsigned char i = gtk_tree_store_iter_depth(store, &iter); i != 0; i--) {
        GtkTreeIter newIter;
        gtk_tree_model_iter_parent(GTK_TREE_MODEL(store), &newIter, &iter);
        iter = newIter;
    }

    gtk_tree_store_insert(store, &newiter, &iter, 0);
    gtk_tree_store_set(store, &newiter, 0, channelname, -1);
    gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(servertree)), &newiter);
} // add_channel_to_tree

gchar* getCurrentChannelName(void) {
    GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(servertree)));
    GtkTreeIter iter;

    gchar *selected_string;

    if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(servertree)), NULL, &iter)) { return NULL; }
    if (gtk_tree_store_iter_depth(store, &iter) < 1) { return NULL; }

    gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &selected_string, -1);

    return selected_string;
} // getCurrentChannelName

void notifyMention(const char* author, const char* channel) {
    /*char title[16 + strlen(channel)];
    sprintf(title, "New mention in %s", channel);
    GNotification* noti = g_notification_new(title);

    char body[4 + strlen(author)];
    sprintf(body, "By %s", author);
    g_notification_set_body(noti, body);

    GApplication* tmp = g_application_new("tmpapp", G_APPLICATION_FLAGS_NONE);
    g_application_send_notification(tmp, NULL, noti);
    
    g_application_quit(tmp);
    g_free(tmp);
    g_free(noti);*/
} // notifyMention

void clearChat(void) {
    GtkTextBuffer* buff;
    buff = gtk_text_view_get_buffer(GTK_TEXT_VIEW(chat));
    gtk_text_buffer_set_text(buff, "",0);
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(chat), buff);
}

struct ircClient* getCurrentClient(void) {
    return connections->clients[currentClientIndex()];
} // getCurrentClient

FILE* getCurrentFile(void) {
    return connections->history[currentClientIndex()];
} //getCurrentCLone

// Other helpers
static uint8_t currentClientIndex(void) {
    GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(servertree)));
    GtkTreeIter iter;
    if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(servertree)), NULL, &iter)) { return NULL; }
    for (unsigned char i = gtk_tree_store_iter_depth(store, &iter); i != 0; i--) {
        GtkTreeIter newIter;
        gtk_tree_model_iter_parent(GTK_TREE_MODEL(store), &newIter, &iter);
        iter = newIter;
    }
    GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
    return *gtk_tree_path_get_indices(path);
} //currentClientIndex