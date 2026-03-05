#ifndef SERVER_H
#define SERVER_H

int server_init(char *iptcp, char *porttcp);
int server_close(int sockfd);

#endif