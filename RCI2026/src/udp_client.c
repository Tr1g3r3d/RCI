#define _POSIX_C_SOURCE 200112L

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
#include "../headers/client.h"

#define MAX_EDGES 100

typedef struct
{
    int id;
    int sockfd;
} t_edge;

static t_edge edges[MAX_EDGES];
static int n_edges = 0;

static int find_edge_index(int id)
{
    int i;
    for (i = 0; i < n_edges; i++)
    {
        if (edges[i].id == id)
            return i;
    }
    return -1;
}

static void remove_edge_at(int idx)
{
    int i;

    if (idx < 0 || idx >= n_edges)
        return;

    close(edges[idx].sockfd);

    for (i = idx; i < n_edges - 1; i++)
        edges[i] = edges[i + 1];

    n_edges--;
}

static void clear_all_edges(void)
{
    while (n_edges > 0)
        remove_edge_at(0);
}

static int add_edge(int id, int sockfd)
{
    if (n_edges >= MAX_EDGES)
        return -1;

    edges[n_edges].id = id;
    edges[n_edges].sockfd = sockfd;
    n_edges++;
    return 0;
}

static int request_contact(
    int udp_fd,
    struct addrinfo *registry_res,
    int req_net,
    int req_id,
    char *out_ip,
    size_t out_ip_sz,
    char *out_tcp,
    size_t out_tcp_sz)
{
    char req[256];
    char reply[1024];
    int tid;
    ssize_t sent;
    ssize_t recvd;
    fd_set rfds;
    struct timeval tv;

    tid = rand() % 1000;
    snprintf(req, sizeof(req), "CONTACT %03d 0 %03d %02d", tid, req_net, req_id);

    sent = sendto(udp_fd, req, strlen(req), 0, registry_res->ai_addr, registry_res->ai_addrlen);
    if (sent == -1)
        return -1;

    for (;;)
    {
        int r_tid = -1;
        int op = -1;
        int net_reply = -1;
        int id_reply = -1;
        char ip[INET_ADDRSTRLEN] = "";
        char tcp[6] = "";

        FD_ZERO(&rfds);
        FD_SET(udp_fd, &rfds);
        tv.tv_sec = 3;
        tv.tv_usec = 0;

        if (select(udp_fd + 1, &rfds, NULL, NULL, &tv) <= 0)
            return -1;

        recvd = recvfrom(udp_fd, reply, sizeof(reply) - 1, 0, NULL, NULL);
        if (recvd <= 0)
            return -1;

        reply[recvd] = '\0';

        sscanf(reply, "CONTACT %d %d %d %d %15s %5s", &r_tid, &op, &net_reply, &id_reply, ip, tcp);

        if (r_tid != tid)
            continue;

        if (op != 1)
            return -1;

        snprintf(out_ip, out_ip_sz, "%s", ip);
        snprintf(out_tcp, out_tcp_sz, "%s", tcp);
        return 0;
    }
}

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
    tcp_server = -1;
    net = -1;
    nodes = -1;
    
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
        int i;
        

        FD_ZERO(&readfds);
        FD_SET(0, &readfds);  // Monitor stdin
        FD_SET(fd, &readfds); // Monitor socket

        if (tcp_server != -1) {
            FD_SET(tcp_server, &readfds);
            if (tcp_server > max_fd)
                max_fd = tcp_server;
        }

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

                char command[4];
                int arg1 = -1;
                int arg2 = -1;
                int parsed_args = sscanf(line, "%3s %d %d", command, &arg1, &arg2);

                if (parsed_args == 2 && strcmp(command, "n") == 0) {
                    // Handle 'n' with net
                    net = arg1;
                    int tid = rand() % 1000;  // Random tid between 0-999
                    sprintf(buffer, "NODES %03d 0 %d", tid, net);
                    n = sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen);
                    if (n == -1) {
                        perror("sendto error");
                        return -1;
                    }
                } else if(parsed_args == 3 && strcmp(command, "j") == 0) {
                    // Start TCP server
                    net = arg1;
                    nodes = arg2;
                    
                    int tid = rand() % 1000;  // Random tid between 0-999
                    sprintf(buffer, "REG %03d 0 %03d %02d %s %s", tid, net, nodes, iptcp, porttcp);

                    n = sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen);
                    if (n == -1) {
                        perror("sendto error");
                        return -1;
                    }   
                    tcp_server = server_init(iptcp, porttcp);
                    if (tcp_server == -1) {
                        fprintf(stderr, "Failed to start TCP server\n");
                        break;
                    }
                } else if (parsed_args == 2 && strcmp(command, "ae") == 0) {
                    int neigh_id = arg1;
                    int neigh_sock;
                    char neigh_ip[INET_ADDRSTRLEN];
                    char neigh_tcp[6];

                    if (net < 0 || nodes < 0) {
                        fprintf(stderr, "Join a network first with 'j net id'.\n");
                        continue;
                    }

                    if (neigh_id == nodes) {
                        fprintf(stderr, "Cannot add edge to self.\n");
                        continue;
                    }

                    if (n_edges >= MAX_EDGES) {
                        fprintf(stderr, "Maximum number of edges reached.\n");
                        continue;
                    }

                    if (find_edge_index(neigh_id) != -1) {
                        fprintf(stderr, "Edge to node %02d already exists.\n", neigh_id);
                        continue;
                    }

                    if (request_contact(fd, res, net, neigh_id, neigh_ip, sizeof(neigh_ip), neigh_tcp, sizeof(neigh_tcp)) != 0) {
                        fprintf(stderr, "Failed to get contact for node %02d.\n", neigh_id);
                        continue;
                    }

                    if (atoi(neigh_tcp) <= 0) {
                        fprintf(stderr, "Invalid TCP port for node %02d.\n", neigh_id);
                        continue;
                    }

                    neigh_sock = client_init(neigh_ip, neigh_tcp);
                    if (neigh_sock == -1) {
                        fprintf(stderr, "Failed to connect to node %02d (%s:%s).\n", neigh_id, neigh_ip, neigh_tcp);
                        continue;
                    }

                    snprintf(buffer, sizeof(buffer), "NEIGHBOR %02d\n", nodes);
                    if (write(neigh_sock, buffer, strlen(buffer)) == -1) {
                        perror("write error");
                        close(neigh_sock);
                        continue;
                    }

                    if (add_edge(neigh_id, neigh_sock) != 0) {
                        fprintf(stderr, "Maximum number of edges reached.\n");
                        close(neigh_sock);
                        continue;
                    }

                    printf("Edge added to node %02d (%s:%s).\n", neigh_id, neigh_ip, neigh_tcp);
                } else if (parsed_args == 1 && strcmp(command, "x") == 0) {
                    // Handle 'x' with no arguments
                    clear_all_edges();
                    if (tcp_server != -1)
                        server_close(tcp_server);
                    freeaddrinfo(res);
                    close(fd);
                    return 0;
                } else if(parsed_args == 1 && strcmp(command, "l") == 0 ) {
                    if (net < 0 || nodes < 0) {
                        fprintf(stderr, "Not in a network.\n");
                        continue;
                    }

                    int tid = rand() % 1000;
                    sprintf(buffer, "REG %03d 3 %03d %02d", tid, net, nodes);

                    n = sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen);
                    
                    if (n == -1) {
                        perror("sendto error");
                        return -1;
                    }
                    clear_all_edges();
                    if (tcp_server != -1) {
                        server_close(tcp_server);
                        tcp_server = -1;
                    }
                    net = -1;
                    nodes = -1;
                } else if(parsed_args == 2 && strcmp(command, "re") == 0 ) {
                    int neigh_id = arg1;
                    int idx = find_edge_index(neigh_id);

                    if (idx == -1) {
                        fprintf(stderr, "No edge to node %02d.\n", neigh_id);
                        continue;
                    }

                    remove_edge_at(idx);
                    printf("Edge removed from node %02d.\n", neigh_id);
                } else if(parsed_args == 1 && strcmp(command, "sg") == 0 ) {
                    if (n_edges == 0) {
                        printf("No neighbors.\n");
                    } else {
                        for (i = 0; i < n_edges; i++)
                            printf("%02d\n", edges[i].id);
                    }
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

        if (tcp_server != -1 && FD_ISSET(tcp_server, &readfds)) {
            int new_sock = accept(tcp_server, NULL, NULL);

            if (new_sock == -1) {
                perror("accept error");
            } else {
                char neigh_msg[128];
                ssize_t r = read(new_sock, neigh_msg, sizeof(neigh_msg) - 1);
                int neigh_id = -1;

                if (r <= 0) {
                    close(new_sock);
                } else {
                    neigh_msg[r] = '\0';

                    if (sscanf(neigh_msg, "NEIGHBOR %d", &neigh_id) != 1) {
                        fprintf(stderr, "Invalid NEIGHBOR message: %s\n", neigh_msg);
                        close(new_sock);
                    } else if (neigh_id == nodes) {
                        close(new_sock);
                    } else if (find_edge_index(neigh_id) != -1) {
                        /* Keep a single edge between each node pair. */
                        close(new_sock);
                    } else if (add_edge(neigh_id, new_sock) != 0) {
                        fprintf(stderr, "Maximum number of edges reached.\n");
                        close(new_sock);
                    } else {
                        printf("Edge added from node %02d.\n", neigh_id);
                    }
                }
            }
        }

    }

    return 0;

}