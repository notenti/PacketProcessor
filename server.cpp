// server.cpp
// Purpose: Provides a framework for processing
// packets
//
// @author Nate Otenti
// @version 1.0 02/19/2018

#include <iostream>
#include <string.h>
#include <algorithm>
#include <cctype>
#include <stdio.h>
#include <stdint.h>
#include "server.h"

// Enumeration to hold the defined
// response codes.
enum ResponseCode {
        OK,
        UNKNOWN_ERROR,
        PAYLOAD_TOO_LARGE,
        UNSUPPORTED_REQ_TYPE,
        MAGIC_NUMBER_ERROR = 33,
        COMPRESSION_ERROR
};

// Enumeration to hold the defined
// request codes.
enum RequestCode {
        PING = 1,
        GET_STATS,
        RESET_STATS,
        COMPRESS
};

// Constructor for PacketProcessor class.
// Takes in a pointer to a buffer and it's size and
// copies it to the appropriate member variable.
PacketProcessor::PacketProcessor(char* buffer, int bufferSize){
        memcpy(_incomingBuffer, buffer, bufferSize);
}

// Executes private functions in the necessary
// order to properly process the packet.
void PacketProcessor::processPacket(){
        incomingPacketParseAndSeparate();
        incomingPacketErrorCheck();
        incomingPacketDetermineResponse();
        outgoingPacketCompile();
}

// Overloaded cout operator that enables for verbose
// printing of all basic packet processing information.
std::ostream& operator<<(std::ostream& os, PacketProcessor const &pp){
        std::string requestType, responseType;

        switch (pp._incomingRequestCode) {
        case RequestCode::PING:
                requestType = "Ping";
                break;
        case RequestCode::GET_STATS:
                requestType = "Get Stats";
                break;
        case RequestCode::RESET_STATS:
                requestType = "Reset Stats";
                break;
        case RequestCode::COMPRESS:
                requestType = "Compression";
                break;
        default:
                requestType = "Invalid Request Code";
                break;
        }

        switch (pp._outgoingResponseCode) {
        case ResponseCode::OK:
                responseType = "OK";
                break;
        case ResponseCode::UNKNOWN_ERROR:
                responseType = "Unknown Error";
                break;
        case ResponseCode::UNSUPPORTED_REQ_TYPE:
                responseType = "Reset Stats";
                break;
        case ResponseCode::MAGIC_NUMBER_ERROR:
                responseType = "Magic Number Error";
                break;
        case ResponseCode::COMPRESSION_ERROR:
                responseType = "Compression Erorr";
                break;
        }

        os << std::endl <<
        std::endl <<
        "--RECEIVED FROM CLIENT--------" << std::endl <<
        "\tMagic Number:\t"              << std::hex << pp._incomingMagicNumber << std::endl <<
        "\tPayload Length:\t"            << std::dec << pp._incomingPayloadLength << std::endl <<
        "\tRequest Type:\t"              << requestType << std::endl <<
        "\tPayload:\t"                   << ((pp._incomingRequestCode == RequestCode::COMPRESS) ? pp._incomingPayload : "None") << std::endl <<
        std::endl <<
        "--SENT TO CLIENT--------------" << std::endl <<
        "\tMagic Number:\t"              << std::hex << pp._outgoingMagicNumber << std::endl <<
        "\tPayload Length:\t"            << std::dec << pp._outgoingPayloadLength << std::endl <<
        "\tRequest Type:\t"              <<responseType << std::endl;

        if(pp._incomingRequestCode == RequestCode::GET_STATS && pp._outgoingResponseCode == ResponseCode::OK) {
                os << "\tBytes Received:\t"     << pp._totalBytesReceived << std::endl <<
                "\tBytes Sent:\t"               << pp._totalBytesSent - 17 << std::endl <<
                "\tRatio:\t\t"                  << std::dec << static_cast <unsigned>(pp._compressionRatio) << std::endl <<
                std::endl;

        } else if(pp._incomingRequestCode == RequestCode::COMPRESS && pp._outgoingResponseCode == ResponseCode::OK) {
                os << "\tPayload:\t"            << pp._outgoingPayloadString << std::endl <<
                std::endl;
        }
        return os;
}

// Takes the full incomming buffer and breaks it
// down into the header and payload. Also keeps
// track of the total number of bytes received.
void PacketProcessor::incomingPacketParseAndSeparate() {
        memcpy(&_incomingPacketHeader, &_incomingBuffer, sizeof(_incomingPacketHeader));
        _incomingMagicNumber = _incomingPacketHeader >> 32;
        _incomingPayloadLength = _incomingPacketHeader >> 16;
        _incomingRequestCode = _incomingPacketHeader;
        if(_incomingPayloadLength > 0) {
                _incomingPayload.assign(&_incomingBuffer[sizeof(_incomingPacketHeader)], &_incomingBuffer[sizeof(_incomingPacketHeader)] + _incomingPayloadLength);
        }
        _totalBytesReceived += sizeof(_incomingPacketHeader) + _incomingPayloadLength;
}

// Checks the packet header and payload for
// errors. Sets a member variable to define
// the current error state.
void PacketProcessor::incomingPacketErrorCheck(){
        const int maxPayloadLength = 12288;
        _outgoingResponseCode = ResponseCode::OK;
        _healthyPacket = true;
        if(_incomingMagicNumber != 0X53545259) {
                _outgoingResponseCode = ResponseCode::MAGIC_NUMBER_ERROR;
                _healthyPacket = false;
        } else if(!(_incomingRequestCode == RequestCode::PING || _incomingRequestCode == RequestCode::GET_STATS || _incomingRequestCode == RequestCode::RESET_STATS||_incomingRequestCode == RequestCode::COMPRESS)) {
                _outgoingResponseCode =ResponseCode::UNSUPPORTED_REQ_TYPE;
                _healthyPacket = false;
        } else if(_incomingPayloadLength > maxPayloadLength) {
                _outgoingResponseCode = ResponseCode::PAYLOAD_TOO_LARGE;
                _healthyPacket = false;
        }else if(!_incomingPayload.empty()) {
                _healthyPacket = !(any_of(_incomingPayload.begin(), _incomingPayload.end(), ::isdigit) || any_of(_incomingPayload.begin(), _incomingPayload.end(), ::isupper));
                if(!_healthyPacket) {
                        _outgoingResponseCode = ResponseCode::COMPRESSION_ERROR;
                }
        }
}

// Determines, based on the request type, what
// the response should be. Generates the payload
// buffers accordingly.
void PacketProcessor::incomingPacketDetermineResponse(){
        if(_healthyPacket) {
                switch (_incomingRequestCode) {
                case RequestCode::PING:
                        _outgoingPayloadLength = 0;
                        break;
                case RequestCode::GET_STATS:
                        outgoingPacketFillStats();
                        _outgoingPayloadLength = 9;
                        break;
                case RequestCode::RESET_STATS:
                        _outgoingPayloadLength = 0;
                        _totalBytesSent = 0, _totalBytesReceived = 0, _preCompressedBytes = 0, _postCompressedBytes = 0;
                        break;
                case RequestCode::COMPRESS:
                        compressionAlgorithm();
                        outgoingPacketFillString();
                        _outgoingPayloadLength = _outgoingPayloadString.length();
                        break;
                }
        } else{
                _outgoingPayloadLength = 0;
        }
        outgoingPacketSetHeader();
}

// Sets the header for the outgoing packet.
void PacketProcessor::outgoingPacketSetHeader() {
        _outgoingPacketHeader = static_cast<uint64_t>(_outgoingMagicNumber) << 32 | _outgoingPayloadLength << 16 | _outgoingResponseCode;
        _totalBytesSent += sizeof(_outgoingPacketHeader) + _outgoingPayloadLength;
}

// Fills the outgoing packet's payload with the
// compressed string.
void PacketProcessor::outgoingPacketFillString(){
        memcpy(&_outputBufferPayload, _outgoingPayloadString.c_str(), _outgoingPayloadString.length());
}

// Fills the outgoing packet's payload with the
// requested statistics.
void PacketProcessor::outgoingPacketFillStats(){
        uint64_t totalBytes = static_cast<uint64_t>(_totalBytesReceived) << 32 | _totalBytesSent;
        memcpy(&_outputBufferPayload, &totalBytes, sizeof(totalBytes));
        _compressionRatio = ((double)_postCompressedBytes/(double)_preCompressedBytes) * 100;
        memcpy(&_outputBufferPayload[sizeof(totalBytes)], &_compressionRatio, sizeof(_compressionRatio));
}

// Takes the constructed header and outgoing
// payload and compiles them into one outgoing
// buffer.
void PacketProcessor::outgoingPacketCompile(){
        memcpy(&_outputBufferFull, &_outgoingPacketHeader, sizeof(_outgoingPacketHeader));
        memcpy(&_outputBufferFull[sizeof(_outgoingPacketHeader)], &_outputBufferPayload, sizeof(_outputBufferPayload));
}

// Takes a string and compressed it using the
// simplified prefix encoding scheme.
void PacketProcessor::compressionAlgorithm(){
        int counter = 0;
        bool sameChar = false;
        _preCompressedBytes += _incomingPayloadLength;
        for(int i = 0; i <_incomingPayloadLength - 1; i++) {
                if(_incomingPayload[i] == _incomingPayload[i+1]) {
                        sameChar = true;
                        counter++;
                } else{
                        sameChar = false;
                }
                if(!sameChar && counter >= 2) {
                        _outgoingPayloadString = _outgoingPayloadString + std::to_string(counter + 1) + _incomingPayload[i];
                        counter = 0;
                } else if(!sameChar && counter < 2) {
                        for(int j = 0; j <= counter; j++) {
                                _outgoingPayloadString = _outgoingPayloadString + _incomingPayload[i];
                        }
                        counter = 0;
                }
        }
        if(counter >= 2 && sameChar) {
                _outgoingPayloadString = _outgoingPayloadString + std::to_string(counter + 1) + _incomingPayload[_incomingPayloadLength - 1];
                counter=0;
        } else if(counter < 2) {
                for(int i = 0; i <= counter; i++) {
                        _outgoingPayloadString = _outgoingPayloadString + _incomingPayload[_incomingPayloadLength - 1];
                }
        }
        _postCompressedBytes += _outgoingPayloadString.length();
}

// Setter to set the total number of bytes sent.
void PacketProcessor::setTotalBytesSent(uint32_t totalBytesSent){
        _totalBytesSent = totalBytesSent;
}

// Getter to get the total number of bytes sent.
uint32_t PacketProcessor::getTotalBytesSent(){
        return _totalBytesSent;
}

//Setter to set the total number of bytes received.
void PacketProcessor::setTotalBytesReceived(uint32_t totalBytesReceived){
        _totalBytesReceived = totalBytesReceived;
}

// Getter to get the total number of bytes received.
uint32_t PacketProcessor::getTotalBytesReceived() {
        return _totalBytesReceived;
}

// Setter to set the number of pre-compressed bytes.
void PacketProcessor::setPreCompressedBytes(uint32_t preCompressedBytes){
        _preCompressedBytes = preCompressedBytes;
}

// Getter to get the total number of pre-compressed bytes.
uint32_t PacketProcessor::getPreCompressedBytes(){
        return _preCompressedBytes;
}

// Setter to set the number of post-compressed bytes.
void PacketProcessor::setPostCompressedBytes(uint32_t postCompressedBytes){
        _postCompressedBytes = postCompressedBytes;
}

// Getter to get the total number of post-compressed bytes.
uint32_t PacketProcessor::getPostCompressedBytes(){
        return _postCompressedBytes;
}

// Getter to get the incoming payload string.
std::string PacketProcessor::getPayloadString(){
        return _incomingPayload;
}

// Getter to get the outgoing buffer.
char* PacketProcessor::getOutgoingPacket(){
        return _outputBufferFull;
}
