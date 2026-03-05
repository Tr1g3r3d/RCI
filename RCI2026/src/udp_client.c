#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>  
#include <sys/select.h>
#include <errno.h>
#include <time.h>
#include "../headers/server.h"
    int fd,errcode,tcp_server;
    ssize_t n;
    socklen_t addr_len;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[1024];
    int net, nodes;
/**
 * @brief UDP client inicialization
 *
 * @param ipaddr IPADDR of the node to connect to
 * @param port PORT of the node to connect to
 * @return [ @b int ] 0 if sucessfull, -1 otherwise
 */
int udp_client_init(char *iptcp, char *porttcp, char *ipaddr, char *port)
{   
    srand(time(NULL));  // Initialize random seed
    
    //cria o socket de client
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
        return -1;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    errcode=getaddrinfo(ipaddr, port, &hints, &res);
    if(errcode != 0)
        return -1;

        

    for (;;) // infinite loop
    {
        fd_set readfds;
        struct timeval timeout;
        int max_fd = (fd > 0) ? fd : 0;
        

        FD_ZERO(&readfds);
        FD_SET(0, &readfds);  // Monitor stdin
        FD_SET(fd, &readfds); // Monitor socket

        timeout.tv_sec = 10;  // Optional timeout
        timeout.tv_usec = 0;

        int ready = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
        if (ready == -1) {
            perror("select error");
            return -1;
        } else if (ready == 0) {
            fprintf(stderr, "Select timed out.\n");
            continue;
        }

        if (FD_ISSET(0, &readfds)) {
            char line[1024];  // Buffer for the input line
            if (fgets(line, sizeof(line), stdin) != NULL) {
                // Remove trailing newline if present
                line[strcspn(line, "\n")] = '\0';

                char command;
                int parsed_args = sscanf(line, "%c %d %d", &command, &net, &nodes);

                if (parsed_args == 2 && command == 'n') {
                    // Handle 'n' with net
                    sprintf(buffer, "NODES 100 0 %d", net);
                    n = sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen);
                    if (n == -1) {
                        perror("sendto error");
                        return -1;
                    }
                } else if(parsed_args == 3 && command == 'j') {
                    // Start TCP server
                    tcp_server = server_init(iptcp, porttcp);
                    if (tcp_server == -1) {
                        fprintf(stderr, "Failed to start TCP server\n");
                        break;
                    }
                    int tid = rand() % 1000;  // Random tid between 0-999
                    sprintf(buffer, "REG %03d 0 %03d %02d %s %s", tid, net, nodes, iptcp, porttcp);

                    n = sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen);
                    if (n == -1) {
                        perror("sendto error");
                        return -1;
                    }   
                } else if (parsed_args == 1 && command == 'x') {
                    // Handle 'x' with no arguments
                    freeaddrinfo(res);
                    close(fd);
                    return 0;
                } else if(parsed_args == 1 && command == 'l') {
                    int tid = rand() % 1000;
                    sprintf(buffer, "REG %03d 3 %03d %02d", tid, net, nodes);

                    n = sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen);
                    
                    if (n == -1) {
                        perror("sendto error");
                        return -1;
                    }
                    server_close(tcp_server);
                }else {
                    // Invalid command or wrong number of args
                    fprintf(stderr, "Invalid command or arguments.\n");
                }
            } else {
                // EOF or error reading line
                fprintf(stderr, "Error reading input.\n");
                break;  // Or handle as needed
            }
        }

        if (FD_ISSET(fd, &readfds)) {
            addr_len = sizeof(addr);
            n = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addr_len);
            if (n == -1) {
                perror("recvfrom error");
                return -1;
            }
            write(1, buffer, n);
        }
    }

    return 0;

}