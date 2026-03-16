#ifndef CLIENT_H
#define CLIENT_H

#include "structs.h"

int client_init(char * ipaddr, unsigned int port);
int send_tcp_message(int sockfd, char *message);
int receive_tcp_message(int sockfd, char *buffer);
int establish_connection(t_nodeinfo *node, char *ipaddr, char *port);
int client_udp(t_nodeinfo *node, char *ipaddr, char *port, char *message, unsigned int tid, int joining);

#endif