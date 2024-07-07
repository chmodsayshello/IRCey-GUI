// Implementation of
#include "irc.h"

// Additional own Headers
#include "util.h"

// Network Headers
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

// Others
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

/* +++++Documentation References+++++
 * 
 * R1 (shutdown): https://www.gnu.org/software/libc/manual/html_node/Closing-a-Socket.html
 * R2 (recv)    : https://www.gnu.org/software/libc/manual/html_node/Receiving-Data.html
 * R3 (read)    : https://www.man7.org/linux/man-pages/man2/read.2.html
 */

//+---------+
//| Helpers |
//+---------+

struct nameList currList;

typedef struct Packet {
    enum PacketType {
        PRIVMSG,
        USERLIST,
        ENDOFNAMELIST,
        PING,
        MODT,
        ENDOFMODT,
        IGNORE,
        USER_JOIN_LEAVE,
        USERINFO,
        FATAL_ERROR
    } PacketType;
    struct ircClient* client;
    union PacketData {
        struct MODTPacket {
            const char* modt;
        } MODTPacket;
        //In case of EndOfModt, just stop collecting, nothing to do here ;)

        struct MessagePacket {
            const char* message;
            const char* username;
            const char* channel;
        } MessagePacket;

        struct PingPacket {
            char* ping;
        } PingPacket;

        struct NameListPacket {
            char** names;
            char* channel;
            uint16_t amount;

        } NameListPacket;
        struct EndOfNameListPacket {
            const char* channel;
        } EndOfNameListPacket;

        struct UserJoinLeavePacket {
            const char* user;
            const char* channel;
            const char* reason;
            bool join;
        } UserJoinLeavePacket;
        
        struct UserInfoPacket {
            const char* user;
            const char* username;
            const char* hostname;
            const char* realname;
        } UserInfoPacket;

        struct FatalErrorPacket {
            const char* error;
        } FatalErrorPacket;
    } data;
} Packet;

void sendPacket(int connection, char* packetFormat, ...) {
    va_list args;
    va_start(args, packetFormat);
    char* out = malloc(IRC_MAX_BUFFER);
    assert(out != NULL);

    vsnprintf(out, IRC_MAX_BUFFER, packetFormat, args);
    va_end(args);
#ifdef DEBUG
    printf("!!OUT!!    %s\n\n", out);
#endif
    send(connection, out, strlen(out), 0);
    free(out);
}

//pktStr WILL be written to!
static Packet parsePacket(struct ircClient* client, char* packetString) {
    #ifdef DEBUG
        printf("PARSING: %s\n",packetString);
        fflush(stdout);
    #endif

    //WARNING: SKIP THE NEXT ~150 lines if you hate bad code
    Packet packet;
    packet.client = client;

    char* c = strstr(packetString, "PING :");
    if (c != NULL) {
        packet.PacketType = PING;
        packet.data.PingPacket.ping = c + 6;
        return packet;
    }

    c = strstr(packetString, "PRIVMSG "); // This is everything but safe!
    if (c != NULL) {
        packet.PacketType = PRIVMSG;
        //Parsing Username
        char* unameEndPtr;
        for(char* c2=packetString + 1; *c2 != '!'; c2++) {
            if (c2 > packetString + 21 || *c2 == '\0') { goto IGNORE_PKT; }
            unameEndPtr=c2;
        }
        unameEndPtr++;
        *unameEndPtr = '\0';
        packet.data.MessagePacket.username = packetString+1;
        
        //Parsing Channel
        c += 8; //avoid unneccesary loop iterations
        char* channelEndPtr = strchr(c, ' ');
        packet.data.MessagePacket.channel = c;
        *channelEndPtr = '\0';
        c = channelEndPtr;
        //Parsing actual Message
        c += 2;
        packet.data.MessagePacket.message = c;
        return packet;
    }

    c = strstr(packetString, "353 ");
    if (c != NULL ){ //Namelist
        packet.PacketType = USERLIST;
        c = strchr(c, ':');
        if (c == NULL) { goto IGNORE_PKT; }
        char* channelName = c - 1;
        {
            *c = '\0'; //terminator for channelname
            while(*(channelName - 1) != ' ' && channelName > packetString) {
                channelName--;
            }
        }
        packet.data.NameListPacket.channel = channelName;
        c++;
        {
            char* nameend = strchr(c, '\0');
            if(nameend == NULL) { goto IGNORE_PKT; }
            *nameend = '\0';
        }
        char* name = strtok(c, " ");
        char** names = NULL;
        packet.data.NameListPacket.amount = 0;
        while (name != NULL) {
            packet.data.NameListPacket.amount++;

            names = realloc(names, packet.data.NameListPacket.amount * sizeof(char*)); // realloc() works like malloc() if the pointer is NULL
            size_t size = strlen(name) + 1;
            names[packet.data.NameListPacket.amount-1] = malloc(size);
            strncpy(names[packet.data.NameListPacket.amount-1], name, size);
            name = strtok(NULL, " ");
        }
        packet.data.NameListPacket.names = names;
        return packet;
    }
    c = strstr(packetString, "366 ");
    if (c != NULL) { // end of names;
        packet.PacketType = ENDOFNAMELIST;
        return packet;
    }
    { // join + leave
        bool join = true;
        c = strstr(packetString, "JOIN ");
        bool quit = false;
        
        if(c == NULL) {
            join = false;
            c = strstr(packetString, "PART ");
            if (c == NULL) {
                c = strstr(packetString, "QUIT ");
                quit = true;
            }
        }

        if(c != NULL) {
            packet.PacketType = USER_JOIN_LEAVE;
            packet.data.UserJoinLeavePacket.join = join;
            if(join) {
                char* name = c;
                while(*(name - 1) != ':' && name > packetString) {
                    name--;
                    if(*name == '!') { *name = '\0'; }
                }
                c += 5;

                packet.data.UserJoinLeavePacket.user = name;
                packet.data.UserJoinLeavePacket.channel = c;
            } else {
                //in channel leave messages, the name is at the beginning
                char* name = packetString + 1;
                {
                    char* hostname = strchr(name, '!');
                    if(hostname == NULL) { goto IGNORE_PKT; }
                    *hostname = '\0';
                }

                c += 4; // channel / reason parsing;

                char* reason = strchr(c, ' ');
                if (reason == NULL) { goto IGNORE_PKT; }
                *reason = '\0';
                reason++;

                packet.data.UserJoinLeavePacket.user = name;
                if (quit) {
                    packet.data.UserJoinLeavePacket.channel = "";
                } else {
                    packet.data.UserJoinLeavePacket.channel = c;
                }
                
                packet.data.UserJoinLeavePacket.reason = reason;

            }
           return packet;
        }
    }
    {
        c = strstr(packetString, "372");
        if (c != NULL) {
            packet.PacketType = MODT;
            const size_t minLen = 5 + strlen(client->username);
            if (strlen(c) <= minLen) { goto IGNORE_PKT; }
            packet.data.MODTPacket.modt = c + minLen;
            return packet;
        }
    }
    {
        c = strstr(packetString, "311 ");
        if (c != NULL) {
            packet.PacketType = USERINFO;
            c += 5 + strlen(client->username);
            packet.data.UserInfoPacket.user = c;
            c = strchr(c, ' ');
            if (c == NULL) { goto IGNORE_PKT; }
            *c = '\0';
            packet.data.UserInfoPacket.username = ++c;
            c = strchr(c, ' ');
            if (c == NULL) { goto IGNORE_PKT; }
            packet.data.UserInfoPacket.hostname = ++c;
            c = strstr(c, " * :");
            if (c == NULL) { goto IGNORE_PKT; }
            *c = '\0';
            c += 4;
            packet.data.UserInfoPacket.realname = c;
            
            return packet;
        }
    }

    c = strstr(packetString, "ERROR :");
    if (c != NULL) {
        packet.PacketType = FATAL_ERROR;
        c+=7;
        packet.data.FatalErrorPacket.error = c;
        return packet;
    }
    
    IGNORE_PKT:
    packet.PacketType = IGNORE;
    return packet;
}

static void handlePacket(Packet* pkt) {
    switch (pkt->PacketType) {
        case IGNORE:
        break;

        case PING:
            sendPacket(pkt->client->connection, "PONG :%s\r", pkt->data.PingPacket.ping);
            if (! pkt->client->flags & FLAG_PINGED) {
                pkt->client->flags += FLAG_PINGED;
            }
        break;
        
        case PRIVMSG:
            onMessage(pkt->client, pkt->data.MessagePacket.channel, pkt->data.MessagePacket.username, pkt->data.MessagePacket.message);
        break;   

        case MODT:
            onMODT(pkt->client, pkt->data.MODTPacket.modt);
        break;
        
        case USERLIST:
            if (currList.channel == NULL) {
                size_t size = strlen(pkt->data.NameListPacket.channel) + 1;
                currList.channel = malloc(size);
                memcpy(currList.channel, pkt->data.NameListPacket.channel, size - 1);
                currList.channel[size - 1] = '\0';
            } else if (strcmp(currList.channel, pkt->data.NameListPacket.channel) != 0 || !(pkt->client->flags & REQUEST_NAMES)) {
                break;
            }
            
            uint16_t oldAmount = currList.amount;
            currList.amount += pkt->data.NameListPacket.amount;
            currList.names = realloc(currList.names, currList.amount * sizeof(char*));
            
            for (uint16_t i= oldAmount; i < currList.amount; i++) {
                currList.names[i] = pkt->data.NameListPacket.names[i - oldAmount];
            }
        break;
        case ENDOFNAMELIST:
            if (currList.channel == NULL) { break; }
            onNamesList(pkt->client, currList);
            pkt->client->flags -= REQUEST_NAMES;
            freeNameList(currList);
            currList.channel = NULL;
            currList.amount = 0;
            currList.names = NULL;
        break;
        
        case USER_JOIN_LEAVE:
            if (pkt->data.UserJoinLeavePacket.join) {
                onUserJoin(pkt->client, pkt->data.UserJoinLeavePacket.channel, pkt->data.UserJoinLeavePacket.user);
            } else {
                onUserLeave(pkt->client, pkt->data.UserJoinLeavePacket.channel, pkt->data.UserJoinLeavePacket.user, pkt->data.UserJoinLeavePacket.reason);
            }
        break;

        case FATAL_ERROR:
            disconnectIRC(pkt->client, "An unexpected error occured within the connection between the server and the client");
            puts(pkt->data.FatalErrorPacket.error);
            onFatalFailure(pkt->client, FAILURE_UNEXPECTED_QUIT);
        break;

        case USERINFO:
            if (pkt->client->flags & REQUEST_WHOIS) {
                onUserWhoIsInfo(pkt->client, pkt->data.UserInfoPacket.user, pkt->data.UserInfoPacket.username, pkt->data.UserInfoPacket.realname, pkt->data.UserInfoPacket.hostname);
                pkt->client->flags -= REQUEST_WHOIS;
            }
        break;

        #ifdef DEBUG
        default:
            printf("Ignored packet: \n");
        break;
        #endif
    }
}

//+-----------------+
//| Implementations |
//+-----------------+

struct ircClient* newClient(const char* uname) {
    struct ircClient* client = malloc(sizeof(struct ircClient));
    char i;
    for(i=0; i < UMIN(strlen(uname), 20); i++) {
        client->username[i] = uname[i];
    }
    client->username[i] = '\0';
    client->connection = socket(AF_INET, SOCK_STREAM, 0);
    client->connected = false;
    client->flags = REQUEST_NAMES;
    return client;
}

void freeClient(struct ircClient *client) {
    if(client->connected) {
        disconnectIRC(client, "Client Shutdown!");
    }
    free(client);
}

void connectIRC(struct ircClient *client, const char* address, const uint16_t port) {
    {   
        struct sockaddr_in addr;
        struct hostent *host = NULL;
        host = gethostbyname(address);
        if (host == NULL) {
            onFatalFailure(client, FAILURE_IP_LOOKUP_FAILED);
            return;
        }
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = *(long *) host->h_addr_list[0];
        connect(client->connection, (struct sockaddr*) &addr, sizeof(addr));
    }
    updateUsername(client);
    sendPacket(client->connection, "USER %s 0 * :%s\r", client->username, client->username);
    client->connected = true;
}

void disconnectIRC(struct ircClient *client, const char* reason) {
    sendPacket(client->connection, "QUIT: %s\r", reason);
    shutdown(client->connection, 0); // See R1
    client->connected = false;
}

void joinChannel(struct ircClient *client, char* channel) {
    client->flags = client->flags | REQUEST_NAMES;
    sendPacket(client->connection, "JOIN %s\r", channel);
}

void leaveChannel(struct ircClient *client, const char* channel, const char* reason) {
    sendPacket(client->connection, "PART %s %s\r", channel, reason);
}

void updateUsername(const struct ircClient *client) {
    sendPacket(client->connection, "NICK %s\r", client->username);
}

void sendMessage(const struct ircClient* client, const char* channel, const char* msg) {
    sendPacket(client->connection, "PRIVMSG %s :%s\r", channel, msg);
}

char* newBuffer(void) {
    char* out = malloc(IRC_MAX_BUFFER + 1);
    assert(out != NULL);
    return out;
}

void tick(struct ircClient* client, char* buffer) {
    if(recv(client->connection, buffer, IRC_MAX_BUFFER - 1, MSG_DONTWAIT) == -1) { return; }
    //NOTE: THIS IS UGLY! INVALID PACKETS CAN ACTUALLY CRASH THIS OR CAUSE UB
    buffer[IRC_MAX_BUFFER - 1] = '\0';// ensure the main buffer is null terminated!
#ifdef DEBUG
    printf("\n\n!!IN!! %s\n\n\n", buffer);
    fflush(stdout); //The following code writes to buffer A LOT, so flush now!
#endif
    uint8_t num = 0;


    buffer[MIN(strlen(buffer), IRC_MAX_BUFFER - 2)] = '\r'; 
    char** strings = NULL;
    {
        char* prev = buffer;
        for (char* str = strchr(buffer,'\r'); str != NULL; str = strchr(str + 1,'\r')) {
            num++;

            strings = realloc(strings, sizeof(char*) * num);
            strings[num - 1] = prev;
            
            *str = '\0';
            prev = str + 1;
        }
    }

    if (num == 0) { return; } // need to quit now; otherwise free(NULL)

    for (uint8_t i = 0; i < num; i++) {
        char* packetString = strings[i];
        Packet pkt = parsePacket(client, packetString);
        handlePacket(&pkt);
        // ACTUAL PARSING
        
    } // end of packet loop

    free(strings);
}

void requestOnlineUsers(struct ircClient* client, const char* channel) {
    client->flags = client->flags | REQUEST_NAMES;
    sendPacket(client->connection, "NAMES %s\r", channel);
}

void freeNameList(struct nameList list) {
    for(unsigned short i = 0; i < list.amount; i++) {
        free(list.names[i]);
    }
    free(list.names);
    free(list.channel);
}

void requestUserInformation(struct ircClient* client, const char* user) {
    client->flags = client->flags | REQUEST_WHOIS;
    sendPacket(client->connection, "WHOIS %s\r", user);
}