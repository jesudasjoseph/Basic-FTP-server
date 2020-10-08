# basic-file-transfer-server
# Author: Jesudas Joseph
# Project Name: Simple File Transfer
# Description: Transfer a file between a server and client with client commands. Also list contents of server directory on client request.
// Collaboration: I discussed this project with Hunter Land another student in CS-372.

A basic file-transfer-Server.

References:

python reference pages
linux man pages


--compile ftserver with this command:
gcc -std=gnu99 -o ftserver ftserver.c iio.c

--run ftserver:
./ftserver <PORTNUM>


--compile ftclient:
No need to compile.

--give ftclient.py execute permissions.
chmod +x ftclient.py

--run ftclient.py
./ftclient.py <SERVER_ADDRESS> <SERVER_PORT> <DATA_PORT> <COMMAND> [FILENAME]

    --possible commands:

Copy File:      -g <FILENAME>
List:           -l
