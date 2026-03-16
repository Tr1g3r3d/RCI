#ifndef SERVER_H
#define SERVER_H

#include "structs.h"

int tcp_server_init(char port[]);
int accept_client_connection(int sockfd);
int incoming_connection(t_nodeinfo *node);

#endif