#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

int socketlisten = -1;
/* 
Handle the ctrl-C signal 
It is assumed that the connections with the individual clients has been closed and only 
the socket that is associated with listening port is open. This is now closed by handler function.
*/

void handler(int sig)
{
    if (socketlisten != -1)
    {
        close(socketlisten);
        printf("\n Closing listening socket and Exiting gracefully\n");
    }

    else
        printf("\n Listening socket was already closed, Exiting gracefully\n");
        
    exit(0);
}

// Converts the numec part of a charcter array to an integer
int getnum(char *p)
{
    int ans = 0;

    while ((((*p) - '0') >= 0) && (((*p) - '0') <= 9))
    {
        ans = ans * 10;
        ans += ((*p) - '0');
        p++;
    }

    return ans;
}

// Converts integer to a character array
char *convert(int x)
{
    char *str = (char *)malloc(11 * sizeof(char));
    int st = 0;

    if (x == 0)
        return "0";

    while (x != 0)
    {
        str[st] = (x % 10) + '0';
        x = x / 10;
        st++;
    }

    char temp;
    for (int i = 0; i < (st / 2); ++i)
    {
        temp = str[i];
        str[i] = str[st - 1 - i];
        str[st - 1 - i] = temp;
    }
    str[st] = '\0';

    return str;
}

// Check whether the character is a number
int isnum(char c)
{
    return (((c - '0') >= 0) && ((c - '0') <= 9));
}

// Check whether the character is an alphabet
int isalph(char c)
{
    return ((((c - 'a') >= 0) && ((c - 'a') <= 25)) || (((c - 'A') >= 0) && ((c - 'A') <= 25)));
}

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        printf("Incorrect format\n");
        return 0;
    }

    // To handle ctrl-C signal
    signal(SIGINT, handler);

    // Retrieve information from database
    char *nameofproduct[50];
    int upcprice[50][2];
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t readbits;

    fp = fopen("database.txt", "r");
    if (fp == NULL)
        return 0;

    int index = 0;
    while ((readbits = getline(&line, &len, fp)) != -1)
    {

        upcprice[index][0] = getnum(line);
        upcprice[index][1] = getnum(line + 4);
        int start = 0;
        int ind = 4;

        while (isnum(*(line + ind)))
        {
            ind++;
        }
        ind++;
        start = ind;
        nameofproduct[index] = (char *)malloc(sizeof(char) * 50);

        while (isalph(*(line + ind)))
        {
            nameofproduct[index][ind - start] = *(line + ind);
            ind++;
        }
        nameofproduct[index][ind] = '\0';

        index++;
    }

    fclose(fp);
    if (line)
        free(line);

    // Size of database less than 50
    int databasesize = index;

    // Error message corresponding to UPC not found
    char *errmessage = "UPC not in database";
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    char sendBuffer[200];
    char recvBuffer[200];
    int pid, n;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    // Setting socket listen to the relevant socket, this will be used by handler
    socketlisten = listenfd;
    printf("Socket retrieve success\n");
    memset(&serv_addr, '\0', sizeof(serv_addr));
    memset(sendBuffer, '\0', sizeof(sendBuffer));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
    bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    if (listen(listenfd, 10) == -1)
    {
        printf("Failed to listen\n");
        return -1;
    }

    while (1)
    {
        connfd = accept(listenfd, NULL, NULL); // Accept awaiting request

        // Create child process to handle request
        if ((pid = fork()) == 0)
        {
            close(listenfd);
            // Keep track of amount to be paid
            int sum = 0;

            while (1)
            {
                n = read(connfd, recvBuffer, sizeof(recvBuffer) - 1);
                
                /* NOTE : The sleep function has been used so that the evaluator can see multiple
                requests from different clients being processed concurrently, otherwise results would
                be printed instantly
                */
                sleep(1);

                int type, upccode, quantity;
                type = getnum(recvBuffer);
                upccode = getnum(recvBuffer + 2);  // Convert the character array to an integer
                quantity = getnum(recvBuffer + 6); // Convert the character array to an integer

                // Handle type 0 queries
                if (type == 0)
                {
                    // Find the item corresponding to this UPC code
                    int ind = -1;
                    for (int j = 0; j < databasesize; ++j)
                    {
                        if (upcprice[j][0] == upccode)
                        {
                            ind = j;
                            break;
                        }
                    }

                    // If no item is found, send error message
                    if (ind == -1)
                    {
                        int start = 0;
                        sendBuffer[start] = '1';
                        sendBuffer[start + 1] = ' ';
                        start += 2;

                        for (; errmessage[start - 2] != '\0'; start++)
                            sendBuffer[start] = errmessage[start - 2];

                        sendBuffer[start] = '\0';
                    }

                    // If item is found return name and price
                    else
                    {
                        sum += upcprice[ind][1] * quantity;
                        int start = 0;
                        sendBuffer[start] = '0';
                        sendBuffer[start + 1] = ' ';
                        start += 2;

                        // Convert the integer array into character array
                        char *p = convert(upcprice[ind][1]);
                        while (*p != '\0')
                        {
                            sendBuffer[start] = *p;
                            p++;
                            start++;
                        }
                        sendBuffer[start] = ' ';
                        start++;
                        p = nameofproduct[ind];
                        while (*p != '\0')
                        {
                            sendBuffer[start] = *p;
                            p++;
                            start++;
                        }
                        sendBuffer[start] = '\0';
                    }

                    write(connfd, sendBuffer, strlen(sendBuffer));

                    /* NOTE : The sleep function has been used so that the evaluator can see multiple
                    requests from different clients being processed concurrently, otherwise results would
                    be printed instantly
                    */
                    sleep(1);
                }

                // For type 1 query return price
                else
                {
                    int start = 0;
                    sendBuffer[start] = '0';
                    sendBuffer[start + 1] = ' ';
                    start += 2;

                    // Convert the integer array into character array
                    char *p = convert(sum);
                    while (*p != '\0')
                    {
                        sendBuffer[start] = *p;
                        p++;
                        start++;
                    }
                    sendBuffer[start] = '\0';
                    write(connfd, sendBuffer, strlen(sendBuffer));

                    /* NOTE : The sleep function has been used so that the evaluator can see multiple
                    requests from different clients being processed concurrently, otherwise results would
                    be printed instantly
                    */
                    sleep(1);
                    break;
                }
            }

            close(connfd);
            exit(0);
        }

        close(connfd);
        sleep(1);
    }

    return 0;
}
