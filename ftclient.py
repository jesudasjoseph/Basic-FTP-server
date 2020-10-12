#!/usr/bin/python2

"""
Last Modified: Dec 1st, 2019

Author: Jesudas Joseph

Description: A file transfer client that can retrieve a text file and also can retrieve the file names in a directory.
"""

import sys
import socket
import struct
import os
import errno

def_packet_size = 1024

#send all data provided in packets of size 'packet_size'
#it is extremely important that the recv function woirks with the same packet_size
#server and client have to work the same in this functionality
def send_packets(sockfd, packet_size, data, inc):
    #if the data from (inc-end of data) is larger than a packet
    if (len(data) - inc) > packet_size:
        #send a full packet of data with a signed int32 header (with data length)
        pack_size = struct.pack('!i', packet_size)
        sockfd.sendall(pack_size, socket.MSG_WAITALL)
        sockfd.sendall(data[inc:packet_size + inc], socket.MSG_WAITALL)
        #use a bit of recurssion and do this function again with a new increment
        send_packets(sockfd, packet_size, data, (inc + packet_size));
    else:
        #if the data doesn't fit in one packet (end of data)
        #the int32 header will be set to size but with the negative sign
        #a negative int32 header represents an end packet
        temp = (0 - (len(data) - inc))
        pack_sizes = struct.pack('!i', temp);
        sockfd.sendall(pack_sizes, socket.MSG_WAITALL)
        sockfd.sendall(data[inc:len(data)], socket.MSG_WAITALL)

#recv a byte of data at a time until we reach data size in Bytes
def recv_packet(sockfd, size):
    raw_data = ""
    while len(raw_data) < size:
        #recv one Byte at a time
        raw_data += sockfd.recv(1)
    return raw_data

#recv all packets from one data set
def recv_packets(sockfd, packet_size):
    #first recv and set the int32 header (indicates how much more to read)
    pack_head = recv_packet(sockfd, 4)
    actual_packet_size = struct.unpack('!i', pack_head)[0]
    data = ""

    #run until we recv all packets
    while 1:
        #if end packet cleanup and return data
        if actual_packet_size < 0:
            data += recv_packet(sockfd, (0 - actual_packet_size))
            return data
        #if middle packet recv and add to data
        else:
            data += recv_packet(sockfd, actual_packet_size)
        #get the next header size
        pack_head = recv_packet(sockfd, 4)
        actual_packet_size = struct.unpack('!i', pack_head)[0]

#checks if its possible to bind to port
#takes: port_number
#returns:
#-1 if out of range
#-2 if failed to bind
#1 if good and bound
def good_port(sockfd, test_port):
    if test_port < 1 or test_port > 65535:
        return -1
    else:
        if try_bind(sockfd, test_port):
            return 1
        else:
            return -2

#try to bind port (try_port)
#returns:
#True if success
#False if fail
def try_bind(sockfd, try_port):
    #try to bind socket
    try:
        sockfd.bind(('',try_port))
        return True
    #if it fails
    except socket.error, msg:
        #close the socket
        sockfd.close()
        #error message
        print "Couldn't bind to data port!", try_port, msg
        return False

def create_file(file_name):
    open(file_name, "x")

def rename_header(header):
    if os.path.exists(header):
        header += "1"
        header = rename_header(header)
        return header
    else:
        return header

def get_file(sockfd, sockd):
    header = recv_packets(sockd, def_packet_size)
    if header == "/ERROR1!":
        print "File does not exist!"
        return -1
    elif header == "/ERROR2!":
        print "Couldn't read file!"
        return -1
    else:
        header = rename_header(header)
        send_packets(sockfd, def_packet_size, "OK", 0)
        print "Receiving file \"", header, "\" from server!"
        line = ""
        f = open(header, 'w')
        while 1:
            line = recv_packets(sockd, def_packet_size)
            if line != "!EOF":
                f.write(line)
            else:
                break
        f.close()
        return 0

def get_list(sockfd, sockd):
    list = recv_packets(sockd, def_packet_size)
    print "Server Directory Contents:"
    print list
    send_packets(sockfd, def_packet_size, "OK", 0)
    return 0

if len(sys.argv) < 5 or len(sys.argv) > 6:
    print("Invalid Arg count!")
    sys.exit(1)

server_port = 0
try:
    server_port = int(sys.argv[2])
except ValueError:
    print "Invalid server port number!"
    sys.exit(2)

data_port = 0
try:
    data_port = int(sys.argv[3])
except ValueError:
    print "Invalid data port number!"
    sys.exit(3)

data_socket = socket.socket()
if good_port(data_socket, data_port) != 1:
    sys.exit(4)
else:
    print "Data port bind successful!"

data_socket.listen(1)

server_socket = socket.socket()

try:
    server_socket.connect((sys.argv[1], server_port))
except socket.error, msg:
    print "Failed to connect!:", msg
    data_socket.close()
    server_socket.close()
    sys.exit(5)

if len(sys.argv) >= 6 and sys.argv[4] == "-g":
    send_packets(server_socket, def_packet_size, (sys.argv[3] + " " + sys.argv[4] + " " + sys.argv[5]), 0)
    response = recv_packets(server_socket, def_packet_size)
    if response == "OK":
        send_packets(server_socket, def_packet_size, "OK", 0)
        serv_data_socket, serv_adr = data_socket.accept();
        print "Data connection successful!"
        if get_file(server_socket, serv_data_socket) == 0:
            print "File transfer complete!"
            send_packets(server_socket, def_packet_size, "OK", 0)
            server_socket.close()
            data_socket.close()
            exit(0)
        else:
            print "Could not copy file!"
            server_socket.close()
            data_socket.close()
            exit(7)
    else:
        print response
        server_socket.close()
        data_socket.close()
        exit(6)
elif len(sys.argv) >= 5 and sys.argv[4] == "-l":
    send_packets(server_socket, def_packet_size, (sys.argv[3] + " " + sys.argv[4]), 0)
    response = recv_packets(server_socket, def_packet_size)
    if response == "OK":
        send_packets(server_socket, def_packet_size, "OK", 0)
        serv_data_socket, serv_adr = data_socket.accept();
        print "Data connection successful!"
        if get_list(server_socket, serv_data_socket) == 0:
            server_socket.close()
            data_socket.close()
            exit(0)
        else:
            print "Could not get directory list"
            server_socket.close()
            data_socket.close()
            exit(7)
    else:
        print response
        server_socket.close()
        data_socket.close()
        exit(6)
else:
    print "Invalid command!!"
    server_socket.close()
    data_socket.close()
    exit(8)
