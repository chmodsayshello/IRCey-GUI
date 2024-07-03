#include "util.h"

//#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
//#include <unistd.h>
//#include <time.h>
#include "shared.h"
#include "irc.h"

bool stringStartsWith(const char* str, const char* prefix) {
    return strncmp(prefix, str, strlen(prefix)) == 0;
}

struct connections* create_connections(void) {
    struct connections* conenections;

    connections = malloc(sizeof(struct connections));
    connections->clients = NULL;
    connections->history = NULL;
    connections->amount = 0;

    return connections;
}

void free_connections(struct connections* connections) {
    for (unsigned char i = 0; i < connections->amount; i++) {
        if (connections->clients[i]->connected) {
            fclose(connections->history[i]);
        }
        freeClient(connections->clients[i]);
    }
    free(connections);
}

char* getLocalTimeString(void) {
    time_t timet = time(NULL);
    struct tm* info = localtime(&timet);
    char* out = malloc(9);
    sprintf(out, "%2i:%02i:%02i", info->tm_hour, info->tm_min, info->tm_sec);
    return out;
}

char* formatChatMessage(const char* author, const char* message) {
    char* out = malloc(24 + strlen(author) + strlen(message));
    char* timestr = getLocalTimeString();
    sprintf(out, "[%s] <%s>: %s", timestr, author, message);
    free(timestr);
    return out;
}