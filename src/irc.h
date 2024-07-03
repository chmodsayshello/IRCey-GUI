#pragma once

#include <stdbool.h>
#include <stdint.h>

#define IRC_DEFAULT_PORT 6667

//Maximum buffer size for IRC in bytes
#define IRC_MAX_BUFFER 512

#define REQUEST_NAMES   (1 << 0)
#define REQUEST_WHOIS   (1 << 1)
#define FLAG_PINGED     (1 << 2)

struct ircClient {
    int connection;
    bool connected;
    uint8_t flags;
    char username[21];
};

struct nameList {
    uint16_t amount;
    char** names;
    char* channel;
};

typedef enum {
    FAILURE_UNEXPECTED_QUIT,
    FAILURE_IP_LOOKUP_FAILED
} Failure;

extern void (*onMessage)(const struct ircClient* client, const char* channel, const char* author, const char* message);
extern void (*onMODT)(const struct ircClient* client, const char* MODTLine);
extern void (*onNamesList)(const struct ircClient* client, struct nameList list);
extern void (*onUserJoin)(const struct ircClient* client, const char* channel, const char* user);
extern void (*onUserLeave)(const struct ircClient* client, const char* channel, const char* user, const char* reason);
extern void (*onUserWhoIsInfo)(const struct ircClient* client, const char* nick, const char* user, const char* realname, const char* hostname);
extern void (*onFatalFailure)(const struct ircClient* client, const uint8_t failure);


struct ircClient* newClient(const char* uname);
void freeClient(struct ircClient *client);

void connectIRC(struct ircClient *client, const char* address, const uint16_t port);
void joinChannel(struct ircClient *client, char* channel);
void leaveChannel(struct ircClient *client, const char* channel, const char* reason);
void updateUsername(const struct ircClient *client); //just set it in the struct and call this
void sendMessage(const struct ircClient* client, const char* channel, const char* msg);
void disconnectIRC(struct ircClient *client, const char* reason);
char* newBuffer(void); //free()

//Reads from socket; parses packets and handles them
void tick(struct ircClient* client, char* buffer); //run this as often as possible! it'll check for new messages etc
void requestOnlineUsers(struct ircClient* client, const char* channel);
void freeNameList(struct nameList list);
void requestUserInformation(struct ircClient* client, const char* user);