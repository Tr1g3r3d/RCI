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
#include <errno.h>
    int fd,errcode;
    ssize_t n;
    socklen_t addr_len;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[1024];
/**
 * @brief UDP client inicialization
 *
 * @param ipaddr IPADDR of the node to connect to
 * @param port PORT of the node to connect to
 * @return [ @b int ] 0 if sucessfull, -1 otherwise
 */
int udp_client_init(char *ipaddr, char port[])
{
        char command;
        int net;
   
    
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

    for (;;) //infinite loop, equivalent to while(true)
    {
        scanf( "%c %d", &command, &net);
        switch (command)
        {
        case 'n':
            sprintf(buffer, "NODES %d 0 100", net);
            n=sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen);
            if(n==-1)
                return -1;
            
            addr_len = sizeof(addr);
            n = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addr_len);
            if(n==-1)
                return -1;
            
            write(1,buffer,n);
            break;
        case 'x':
            freeaddrinfo(res);
            close(fd);
            return fd
        default:
            fprintf(stderr, "Invalid command.\n");
            break;
        }
        

        
    }

}