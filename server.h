//
// server.h
// Purpose: Header file for server.cpp
//
// @author Nate Otenti
// @version 1.0 02/19/2018

#ifndef SERVER_H
#define SERVER_H

// Processes a packet based unon the guidelines set forth
// in the Starry homework problem
class PacketProcessor {
public:
PacketProcessor(char* buffer, int bufferSize);
void processPacket();
friend std::ostream& operator<<(std::ostream& os, PacketProcessor const &pp);

void setTotalBytesSent(uint32_t totalBytesSent);
void setTotalBytesReceived(uint32_t totalBytesReceived);
void setPreCompressedBytes(uint32_t preCompressedBytes);
void setPostCompressedBytes(uint32_t postCompressedBytes);

uint32_t getTotalBytesSent();
uint32_t getTotalBytesReceived();
uint32_t getPreCompressedBytes();
uint32_t getPostCompressedBytes();

std::string getPayloadString();

char* getOutgoingPacket();

private:
void incomingPacketParseAndSeparate();
void incomingPacketErrorCheck();
void incomingPacketDetermineResponse();

void outgoingPacketSetHeader();
void outgoingPacketFillString();
void outgoingPacketFillStats();
void outgoingPacketCompile();

void compressionAlgorithm();

uint8_t _compressionRatio;
uint16_t _incomingPayloadLength, _outgoingPayloadLength, _outgoingResponseCode, _incomingRequestCode;
uint32_t _totalBytesSent, _totalBytesReceived, _preCompressedBytes, _postCompressedBytes, _incomingMagicNumber, _outgoingMagicNumber = 0X53545259;
uint64_t _incomingPacketHeader, _outgoingPacketHeader;

char _incomingBuffer[16384];
char _outputBufferPayload[16376];
char _outputBufferFull[16384];

std::string _incomingPayload, _outgoingPayloadString;

bool _healthyPacket;
};
#endif
