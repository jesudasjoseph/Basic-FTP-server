/*
Author: Jesudas Joseph

Last Modified: Dec 1st, 2019

Description: This is a simple file transfer server. You can transfer any text file to a client using the script 'ftclient.py'.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

//send/recieve functions
#include "iio.h"

//Common packet size between server and client.
const int32_t def_packet_size = 1024;

//Re-used from my coding in OS1
/*Takes one line and splits it into individual arguments
size is optional
user must free() argsOut
returns number of argmuments. */
int split_args(const char *argLine, unsigned int argsMax, char ***argsOut)
{
    char *line = strdup(argLine);
    char *temp;
    int currSize = 0;

    //This function fails if no max is defined.
    if (argsMax == 0)
        return -1;
    else
        currSize = argsMax;

    *argsOut = malloc(sizeof(char **)*currSize);//Set the list to the max possible arguments.
    temp = malloc(sizeof(char)*2048);//Only this variable is a static 2048 characters long.

    //increment until we reach argMax (This will exit before it reaches argMax on most occassions)
    for (unsigned int arg_count = 0; arg_count < argsMax; arg_count++)
    {
        //If first cycle run strtok with char*
        if (arg_count == 0)
            temp = strtok(line, " ");
        //Otherwise run strtok with NULL which will increment by the delimeter
        else
            temp = strtok(NULL, " ");

        //Increase *argsOut size if we run into the end. (Probably won't happen as I set the init size to argsMax)
        //Useless code atm.
        if (temp == NULL)
        {
            *argsOut = realloc(*argsOut, sizeof(char **)*(currSize +1));
            (*argsOut)[arg_count] = NULL;
            return arg_count;
        }
        //alloc memory for the new argument item and then set it to temp's value
        else
        {
            (*argsOut)[arg_count] = malloc(sizeof(char)*(strlen(temp)+1));
            strcpy((*argsOut)[arg_count], temp);
        }

    }
    printf("before\n");
    free(temp);
    printf("after\n");
    return -1;
}

//Check is the provided port is in the correct range.
int check_port_inrange(char* portnum)
{
    int server_port = -1;

    //convert portnum to int
    server_port = atoi(portnum);
    //check if the port falls within the valid range of portnums
    if (server_port < 1 || server_port > 65535)
    {
        return -1;
    }
    else
    {
        return server_port;
    }
}

int try_port_bind(int sockfd, int server_port)
{
    //Setup server address
    struct sockaddr_in serverAdr = {0};
    serverAdr.sin_family = AF_INET;
    serverAdr.sin_port = htons(server_port);
    serverAdr.sin_addr.s_addr = INADDR_ANY;

    //Try to bind socket
    if (bind(sockfd, (struct sockaddr*)&serverAdr, sizeof(struct sockaddr_in)) == -1)
        return -1;
    else
        return 0;
}

//Send items in directory to client. (-l command)
void send_dir_list(int sockfd)
{
    //open DIR and iterate through items and concat the file entries with a newline character.
    //send the list string wich has newlines as delimeters (makes it easy to print out on the client side)
    DIR* dir;
    dir = opendir(".");
    char* list = NULL;
    printf("Sending client directory list...\n");
    while (1)
    {
        struct dirent *mydirent = readdir(dir);
        if (mydirent != NULL)
        {
            if (mydirent->d_type == DT_REG)
            {

                if (list == NULL)
                {
                    //allocate enough memory and cpy entree
                    list = malloc(sizeof(char)*(strlen(mydirent->d_name) + 1));
                    strcpy(list, mydirent->d_name);
                    strcat(list, "\n");
                }
                else
                {
                    //reallocate enough memory and strcat the entree to list
                    list = realloc(list, strlen(mydirent->d_name)+strlen(list)+1);
                    strcat(list, mydirent->d_name);
                    strcat(list, "\n");
                }
            }
        }
        else
        {
            break;
        }
    }
    printf("%s\n", list);
    //send the whole list
    send_packet(sockfd, def_packet_size, list);
}

//Run the server on sockfd
void run_server(int sockfd)
{
    char** client_commands;
    int command_count;

    int command_socket = 0;
    listen(sockfd, 5);
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(struct sockaddr_in);

    //Loop the serve so multiple clients can connect one after another
    while (1)
    {
        //Accept a client connection.
        printf("Waiting for client connection...\n");
        command_socket = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);

        //If the accept fails then go around and try again.
        if (command_socket < 0)
        {
            printf("[%d] ERROR! %s\n", errno, strerror(errno));
        }
        else
        {
            //recv the commands from the client
            char* command_str;
            if (recv_all_packets(command_socket, def_packet_size, &command_str) == 0)
            {
                //split the args with a " " as a delimeter
                command_count = split_args(command_str, 10, &client_commands);

                //send an OK to the client
                send_packet(command_socket, def_packet_size, "OK");
                char* resp;
                //Wait for client's ok.
                recv_all_packets(command_socket, def_packet_size, &resp);
                if (!strcmp(resp, "OK"))
                {
                    //if the client responds OK
                    //setup a data port
                    int data_socket = socket(AF_INET, SOCK_STREAM, 0);

                    //setup the new sockaddr with the clients data port
                    struct sockaddr_in client_data_addr = client_addr;
                    client_data_addr.sin_port = htons(atoi(client_commands[0]));

                    //Try to connect to clients data port
                    if (connect(data_socket, (struct sockaddr*)&client_data_addr, sizeof(struct sockaddr_in)) == 0)
                    {
                        //check the commands
                        if (!strcmp(client_commands[1], "-l"))
                        {
                            //Send the dir list if the -l command was sent
                            send_dir_list(data_socket);
                            resp = NULL;
                            //check for an OK response
                            if (recv_all_packets(command_socket, def_packet_size, &resp) < 0)
                            {
                                printf("[%d] ERROR! %s\n", errno, strerror(errno));
                            }
                            else if (!strcmp(resp, "OK"))
                            {
                                //if OK sent then close down sockets
                                printf("Sent directory list to client!\n");
                                close(data_socket);
                                close(command_socket);
                            }
                            else
                            {
                                printf("What the heck!? (Weird error)\n");
                                exit(10);
                            }
                        }
                        else if (!strcmp(client_commands[1], "-g"))
                        {
                            //start reading the file line by line when a -g is sent.
                            FILE *request_file;
                            request_file = fopen(client_commands[2], "r");
                            if (request_file == NULL)
                            {
                                send_packet(data_socket, def_packet_size, "/ERROR1!");
                                printf("[%d] ERROR! %s\n", errno, strerror(errno));
                            }
                            else
                            {
                                //send the file name to the client
                                send_packet(data_socket, def_packet_size, client_commands[2]);
                                char* response;
                                //recv an OK from client
                                recv_all_packets(command_socket, def_packet_size, &response);

                                if (!strcmp(response, "OK"))
                                {
                                    //once the client sends an OK then start sending lines of the file
                                    printf("Sending \"%s\" file to client!\n", client_commands[2]);

                                    char* line = NULL;
                                    size_t line_len = 0;

                                    while (1)
                                    {
                                        //send each line of the file until we hit an error and then send "!EOF"
                                        //"!EOF" represents EOF for the client.
                                        if (getline(&line, &line_len, request_file) >= 0)
                                        {
                                            send_packet(data_socket, def_packet_size, line);
                                        }
                                        else
                                        {
                                            send_packet(data_socket, def_packet_size, "!EOF");
                                            break;
                                        }
                                    }
                                    recv_all_packets(command_socket, def_packet_size, &response);//recv another OK.
                                    //(just forcing the server to wait until the client recv's everything)
                                    printf("File sent!\n");
                                }
                            }
                        }
                    }
                    else
                    {
                        printf("Failed to connect to client data socket!\n");
                        printf("[%d] ERROR! %s\n", errno, strerror(errno));
                    }
                }
            }

            close(command_socket);
        }
    }
}

int main(int argv, char** argc)
{
    int server_port = 0;

    //check args
    if (argv < 2)
    {
        fprintf(stderr, "ERROR! Too few arguments!\n");
        exit(-1);
    }
    else if (argv > 2)
    {
        fprintf(stderr, "ERROR! Too many arguments!\n");
        exit(-2);
    }
    else
    {
        server_port = check_port_inrange(argc[1]);
        if (server_port == -1)
        {
            fprintf(stderr, "ERROR! Portnum out of range!\n");
            exit(-3);
        }
        else
        {
            int server_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (try_port_bind(server_socket, server_port) != -1)
            {
                //if all checks are past then run the server
                printf("Successfully bound to port: %d\n", server_port);
                run_server(server_socket);
            }
            else
            {
                fprintf(stderr, "ERROR! %s\n", strerror(errno));
                exit(-4);
            }


        }
    }
}
