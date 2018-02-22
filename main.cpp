// main.cpp
// Purpose: Takes in packets, send them off
// to be processed, and then send them back
// to their origin.
//
// @author Nate Otenti
// @version 1.0 02/19/2018

#include "server.h"
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
// Main function to establish the socket connection,
// take in packets, call the PacketProcessor, and send
// the packets back to their origin.
int main(int argc, char *argv[])
{
        int server, client, portNum = 4000, bufferSize = 16384;
        char buffer[bufferSize];
        bool isExit = false;
        struct sockaddr_in serverAddr;
        socklen_t size;
        uint32_t totalBytesSent = 0, preCompressedBytes = 0, postCompressedBytes = 0,
                 totalBytesReceived = 0;

        server = socket(AF_INET, SOCK_STREAM, 0);

        if (server < 0)
        {
                std::cout << "Cannot establish socket. Exiting." << std::endl;
                exit(-1);
        }

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htons(INADDR_ANY);
        serverAddr.sin_port = htons(portNum);

        int hold = 1;
        if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &hold, sizeof(hold)) == -1)
        {
                perror("setsockopt");
                exit(1);
        }

        // Binding the socket
        if ((bind(server, (struct sockaddr *)&serverAddr, sizeof(serverAddr))) < 0)
        {
                std::cout << "The socket has already been established. Exiting."
                          << std::endl;
                exit(-1);
        }

        size = sizeof(serverAddr);
        std::cout << "Waiting for clients..." << std::endl;
        listen(server, 1);
        client = accept(server, (struct sockaddr *)&serverAddr, &size);

        if (client < 0)
                std::cout << "Issue accepting client." << std::endl;
        while (client > 0)
        {
                std::cout << "Connected to client, waiting for data." << std::endl;
                while (!isExit)
                {
                        // Packet comes in
                        recv(client, buffer, bufferSize, 0);

                        // Packet is sent to be processed
                        PacketProcessor packet(buffer, bufferSize);
                        packet.setTotalBytesReceived(totalBytesReceived);
                        packet.setTotalBytesSent(totalBytesSent);
                        packet.setPreCompressedBytes(preCompressedBytes);
                        packet.setPostCompressedBytes(postCompressedBytes);
                        packet.processPacket();

                        // Packet is copied back to the original buffer
                        memcpy(&buffer, packet.getOutgoingPacket(), bufferSize);

                        // Packet is sent back to its origin
                        send(client, buffer, bufferSize, 0);

// Compilation flag to have output or not
#ifdef DEBUG
                        std::cout << packet;
#endif

                        // Variables that need to survive longer than the
                        // life of one object are stored
                        totalBytesReceived = packet.getTotalBytesReceived();
                        totalBytesSent = packet.getTotalBytesSent();
                        preCompressedBytes = packet.getPreCompressedBytes();
                        postCompressedBytes = packet.getPostCompressedBytes();

                        // If the payload ever equals # the connection
                        // will terminate
                        if (packet.getPayloadString() == "#")
                        {
                                isExit = true;
                        }
                }

                std::cout << "Connection terminated" << std::endl;
                close(client);
                exit(1);
        }
        close(server);
        return 0;
}
