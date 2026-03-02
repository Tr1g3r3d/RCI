#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/**
 * @brief Turns a string into an unsigned int
 *
 * @param str String to transform
 * @return [ @b unsigned int ] Value of the string
 */
unsigned int strtoui(const char *str)
{
    size_t len = strlen(str);
    unsigned int result = 0;
    unsigned int power = 1;
    for (size_t i = 0; i < len; i++, power *= 10)
        result += (unsigned int)(str[len - i - 1] - '0') * power;
    return result;
}

/**
 * @brief Checks if a string is a number
 *
 * @param str String to check
 * @return [ @b int ] 1 if sucessfull, 0 otherwise
 */
int strisui(const char *str)
{
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++)
    {
        if (str[i] > '9' || str[i] < '0')
            return 0;
    }
    return 1;
}

/**
 * @brief Checks if a string is a valid ip address
 *
 * @param str String to check
 * @return [ @b int ] 1 if sucessfull, 0 otherwise
 */
int isipaddr(const char *str)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, str, &(sa.sin_addr));
    return result > 0;
}

/**
 * @brief Checks the arguments of the program invocation
 *
 * @param argc Number of arguments
 * @param inputsv Arguments
 * @return [ @b int ] 0 if sucessfull, -1 otherwise
 */
int process_invocation(int argc, char **inputsv)
{

    if (argc < 3 || argc > 5)
    {
        fprintf(stderr, "[ERROR]: Invalid invocation of the program!\n");
        return -1;
    }

    if (!isipaddr(inputsv[1]))
    {
        fprintf(stderr, "[ERROR]: IP ADDRESS must be a valid IP address (was %s).\n", inputsv[1]);
        return -1;
    }
    
    if (!strisui(inputsv[2]))
    {
        fprintf(stderr, "[ERROR]: TCP PORT must be a number (was %s).\n", inputsv[2]);
        return -1;
    }

    if (strtoui(inputsv[2]) < 49152 || strtoui(inputsv[2]) > 65535)
    {
        fprintf(stderr, "[ERROR]: TCP PORT must be between 49152 and 65535 (was %s).\n", inputsv[2]);
        return -1;
    }
    
    if (argc > 3)
    {
        if (!isipaddr(inputsv[3]))
        {
            fprintf(stderr, "[ERROR]: REG IP ADDRESS must be a valid IP address (was %s).\n", inputsv[3]);
            return -1;
        }
    }

    if (argc > 4)
    {
        if (!strisui(inputsv[4])  && argc > 4)
        {
            fprintf(stderr, "[ERROR]: REG UDP PORT must be a number (was %s).\n", inputsv[4]);
            return -1;
        }

        if ((strtoui(inputsv[4]) < 49152 || strtoui(inputsv[4]) > 65535)  && argc > 4)
        {
            fprintf(stderr, "[ERROR]: REG UDP PORT must be between 49152 and 65535 (was %s).\n", inputsv[2]);
            return -1;
        }
    }
    
    return 0;
}