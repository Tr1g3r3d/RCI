#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../headers/utils.h"
#include "../headers/structs.h"
#include "../headers/server.h"
#include "../headers/select.h"
#include "../headers/processing.h"


int main(int argc, char *argv[])
{
    //verifica os argumentos da consola
    if (process_invocation(argc, argv) != 0)
        exit(1);
    char *inputs[5] = {argv[0], argv[1], argv[2], (argc < 4) ? "193.136.138.142" : argv[3], (argc < 5) ? "59000" : argv[4]};

    /* Iniciar Server TCP */
    int server_sockfd = tcp_server_init(inputs[2]);
    if (server_sockfd == -1)
    {
        fprintf(stderr, "[ERROR]: Error initiating the server.\n");
        exit(1);
    }

    //criação do node
    t_nodeinfo *node = node_init(inputs[1], inputs[2], inputs[3], inputs[4], server_sockfd);
    if (node == NULL)
    {
        fprintf(stderr, "[ERROR]: Error creating the node.\n");
        exit(1);
    }
    printf("[INFO]: Node created successfully!\n");
    printNode(node);
    printf("\n");

    printf("[COMMAND]: ");
    fflush(stdout);

    while (1)
    {
        //verifica a ocorrência de um evento
        int event = select_event(node);

        // Clear user prompt
        if (event != 1 && event != 0)
            printf("\x08\x08\x08\x08");

        // processa a ocorrência
        int result = 0;
        if (event < 4)
        {
            switch (event)
            {
            case 0:
                break;

            case 1:
                result = process_user_input(node);
                break;

            case 2:
                result = incoming_connection(node);
                break;

            default:
                break;
            }
        }
        else
        {
            unsigned int int_id = event - 4;
            result = process_intern_connection(node, int_id);
        }

        //em caso de erro dá leave do node, fecha o socket do servidor e sai do programa
        if (result < 0)
        {
            result = process_leave_command(node);
            if (node->server_tcp_sockfd != -1)
                close(node->server_tcp_sockfd);
            puts("\x1b[31m[!] An error has occurred!\033[m");
            break;
        }
        if (result > 0)
            break;
        if (event != 0)
        {
            printf("\r[COMMAND]: ");
            fflush(stdout);
        }
    }

    if(node->server_tcp_sockfd != -1) close(node->server_tcp_sockfd);
    free(node);
    exit(1);
}