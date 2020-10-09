/*
Author: Jesudas Joseph
Last Modified: 2020-10-08

Description:
Functions to reliably transfer strings between UNIX sockets.
*WILL BLOCK IF DATA ISN'T RECEIVED!*

*/

#define PRE_PACKET_SIZE 4 //int32_t size

/*
Send all data specified in 'data'. use packets the size of packet_size.
Packet length is specified by an initial packet sent that contains an int32_t

returns:
0 if success
-1 if any socket.h function fails
*/
int send_packet(int sockfd, int32_t packet_size, char* data);

/*
Receive a packet of specified size 'packet_size'. set 'data' to packet data.

returns:
0 if this packet is a single packet or ends a series of packets
1 if there are more packets that are to be combined
-1 if any socket.h function fails
*/
int recv_packet(int sockfd, int32_t packet_size, char** data);

/*
Receive all packets that were sent as a set and place into 'data'.

returns:
0 if success
-1 if any socket.h function fails
*/
int recv_all_packets(int sockfd, int32_t packet_size, char** data);
