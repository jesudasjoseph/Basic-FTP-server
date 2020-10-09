/*
Author: Jesudas Joseph
Last Modified: 2020-10-08

Description:
Functions to reliably transfer strings between UNIX sockets.
*WILL BLOCK IF DATA ISN'T RECEIVED!*
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#define PRE_PACKET_SIZE 4 //int32_t size

/*
Send all data specified in 'data'. use packets the size of packet_size.
Packet length is specified by an initial packet sent that contains an int32_t

returns:
0 if success
-1 if any socket.h function fails
*/
int send_packet(int sockfd, int32_t packet_size, char* data)
{
    int data_size = strlen(data);
    int32_t actual_packet_size = htonl((uint32_t)packet_size);
    int sent_data = 0;
    int stat = 0;

    if (data_size > packet_size)
    {
        if (send(sockfd, &actual_packet_size, PRE_PACKET_SIZE, 0) == -1)
        {
            printf("[iio->send_packets][%d] ERROR! %s\n", errno, strerror(errno));
            return -1;
        }
        if (send(sockfd, data, packet_size, 0) == -1)
        {
            printf("[iio->send_packets][%d] ERROR! %s\n", errno, strerror(errno));
            return -1;
        }
        stat = send_packet(sockfd, packet_size, (data+=packet_size));
    }
    else
    {
        actual_packet_size = htonl((uint32_t)(0 - data_size));
        if (send(sockfd, &actual_packet_size, PRE_PACKET_SIZE, 0) == -1)
        {
            printf("[iio->send_packets][%d] ERROR! %s\n", errno, strerror(errno));
            return -1;
        }
        if (send(sockfd, data, data_size, 0) == -1)
        {
            printf("[iio->send_packets][%d] ERROR! %s\n", errno, strerror(errno));
            return -1;
        }
    }
    return stat;
}

/*
Receive a packet of specified size 'packet_size'. set 'data' to packet data.

returns:
0 if this packet is a single packet or ends a series of packets
1 if there are more packets that are to be combined
-1 if any socket.h function fails
*/
int recv_packet(int sockfd, int32_t packet_size, char** data)
{
    int32_t actual_packet_size = 0;

    if (recv(sockfd, &actual_packet_size, PRE_PACKET_SIZE, MSG_WAITALL) == -1)
    {
        printf("[iio->recv_packet head][%d] ERROR! %s\n", errno, strerror(errno));
        return -1;
    }
    actual_packet_size = (int32_t)ntohl(actual_packet_size);
    if (actual_packet_size > 0)
    {
        *data = malloc(sizeof(char)*(packet_size));
        if (recv(sockfd, *data, packet_size, MSG_WAITALL) == -1)
        {
            printf("[iio->recv_packet][%d] ERROR! %s\n", errno, strerror(errno));
            return -1;
        }
        return 1;
    }
    else
    {
        *data = malloc(sizeof(char)*((0 - actual_packet_size)+1));
        if (recv(sockfd, *data, (0 - actual_packet_size), MSG_WAITALL) == -1)
        {
            printf("[iio->recv_packet][%d] ERROR! %s\n", errno, strerror(errno));
            return -1;
        }
        (*data)[(0 - actual_packet_size)] = '\0';
        return 0;
    }
}

/*
Receive all packets that were sent as a set and place into 'data'.

returns:
0 if success
-1 if any socket.h function fails
*/
int recv_all_packets(int sockfd, int32_t packet_size, char** data)
{
    char* temp;

    int packet_cont = 1;

    packet_cont = recv_packet(sockfd, packet_size, &temp);

    if (packet_cont == 0)
    {
        *data = malloc(sizeof(char)*(strlen(temp)+1));
        strcpy(*data, temp);
        free(temp);
        return 0;
    }
    else if (packet_cont == -1)
    {
        return -1;
    }
    else
    {
        *data = malloc(sizeof(char)*(strlen(temp)+1));
        strcpy(*data, temp);
        free(temp);
        while (packet_cont == 1)
        {
            packet_cont = recv_packet(sockfd, packet_size, &temp);
            *data = realloc(*data, sizeof(char)*(strlen(*data)+strlen(temp)+1));
            strcat(*data, temp);
            free(temp);
        }
        return 0;
    }
}






//DEBUG
/*
int main()
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

    socklen_t sizeOfClientInfo;
    struct sockaddr_in serverAddress, clientAddress;
    serverAddress.sin_family = AF_INET; // Create a network-capable socket
    serverAddress.sin_port = htons(25665); // Store the port number
    serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process
    // Set up the socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
    bind(server_socket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)); // Connect socket to port
    listen(server_socket, 5); // Flip the socket on - it can now receive up to 5 connections
    // Accept a connection, blocking if one is not available until one connects

    struct addrinfo *hostinfo;
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_INET;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

    getaddrinfo(NULL, "25665", &hints, &hostinfo);
    int i = connect(client_socket, hostinfo->ai_addr, hostinfo->ai_addrlen);
    printf("%d, %s\n", i, strerror(errno));
    sleep(1);

    i = accept(server_socket, NULL, NULL);
    printf("%d, %s\n", i, strerror(errno));

    printf("server accepted!\n");

    send_packet(client_socket, 5, "why so many dasfs asrfv wqarbvwarbwrwa5awnrawrnwarbawrarvqv awqe qewrqb rwa r  awr awrwa bar barbaa rwar rv awr awrawwaa df we awv ev aw d fasb aw wae a v v w br er vbaw we ad weaswawb awd awb wwwb bwawb !");

    char* buf;
    recv_all_packets(i, 5, &buf);
    printf("%s\n", buf);
}
*/
