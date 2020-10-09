<!--
Author: Jesudas Joseph
Project Name: Simple File Transfer
Description: Transfer a file between a server and client with client commands. Also list contents of server directory on client request.
Collaboration: I discussed this project with Hunter Land another student in CS-372.
-->

# Basic File Transfer Server

This project is a basic file transfer server and client. The server is coded in c and the client is coded in python2.

## Compiling
> ### ftserver:
>
>	gcc -std=gnu99 -o ftserver ftserver.c iio.c


## Running
> ### Client:
>
>	ftclient.py <SERVER_ADDRESS> <SERVER_PORT> <DATA_PORT> <FLAGS>
>>	#### Flags:
>>	copy file: 	-g <FILENAME></br>
>>	list:		    -l
>
> ### Server:
>
>	./ftserver <PORTNUM>
