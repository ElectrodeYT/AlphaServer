#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#elif linux

#include <sys/socket.h>
#include <arpa/inet.h>

#define htonll(x) ((1==htonl(1)) ? (x) : (((uint64_t)htonl((x) & 0xFFFFFFFFUL)) << 32) | htonl((uint32_t)((x) >> 32)))
#define ntohll(x) ((1==ntohl(1)) ? (x) : (((uint64_t)ntohl((x) & 0xFFFFFFFFUL)) << 32) | ntohl((uint32_t)((x) >> 32)))
#endif

/// Returns 0 if data was available and received, not 0 if no data is present.
/// If any data is present, this function will wait for all the data of len to be received
/// If block is true, this function emulates a blocking call without having to change the socket to blocking
int static safeRecv(int socket, char *data, int len, bool block = false) {
    char *dat = data;
    int len_to_go = len;
    bool data_has_been_received = false;

    while (len_to_go > 0) {
        int data_received_len = recv(socket, dat, len_to_go, 0);
#ifdef _WIN32
        if (data_received_len == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK && !data_has_been_received && !block) { return -1; }
            if (error != WSAEWOULDBLOCK) { std::cout << "safeRecv got non-WOULDBLOCK error from recv: " << error << "\n";  throw std::runtime_error("socket error"); }
#else
        if (data_received_len == -1) {
            if ((errno == EAGAIN || errno == EWOULDBLOCK) && !data_has_been_received && !block) { return -1; }
            if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
                std::cout << "safeRecv got non-EAGAIN/EWOULDBLOCK error from recv: " << errno << "\n";
            }
#endif
        } else {
            dat += data_received_len;
            len_to_go -= data_received_len;
        }
    }
#ifdef HOLY_HELL_FUCKING_REALLY_VERBOSE
    std::cout << "safeRecv: " << std::hex;
    for(int i = 0; i < len; i++) {
        printf("%02x", (int)0 | (unsigned char)data[i]);
    }
    std::cout << "\n" << std::dec;
#endif
    return 0;
}

// 
// Functions for sending variables
//

#if defined(_WIN32) || defined(linux)

inline static void sendVar(int socket, unsigned char byte) {
    send(socket, (char *) &byte, 1, 0);
}

inline static void sendVar(int socket, unsigned short s) {
    unsigned short network_short = htons(s);
    send(socket, (char *) &network_short, sizeof(s), 0);
}

inline static void sendVar(int socket, unsigned int i) {
    unsigned int network_short = htonl(i);
    send(socket, (char *) &network_short, sizeof(i), 0);
}

inline static void sendVar(int socket, unsigned long long l) {
    unsigned long long network_short = htonll(l);
    send(socket, (char *) &network_short, sizeof(l), 0);
}

// TODO: endianness

inline static void sendVar(int socket, float f) {
    unsigned char bytes[sizeof(float)];
    memcpy(bytes, (char*)&f, sizeof(float));
    std::reverse(bytes, bytes + sizeof(float));
    send(socket, (char *) bytes, sizeof(float), 0);
}

inline static void sendVar(int socket, double d) {
    unsigned char bytes[sizeof(double)];
    memcpy(bytes, (char*)&d, sizeof(double));
    std::reverse(bytes, bytes + sizeof(double));
    send(socket, (char *) bytes, sizeof(double), 0);
}

inline static void sendVar(int socket, std::string data, bool dont_send_length = false) {
    // Send string length as short
    short string_len = htons(data.size());
    if (!dont_send_length) { send(socket, (char *) &string_len, sizeof(string_len), 0); }
    // Now send string data
    send(socket, data.data(), data.size(), 0);
}

#else
#error unsupported OS
#endif

//
// Functions for receiving variables
//

inline unsigned char recvChar(int socket) {
    unsigned char ret;
    safeRecv(socket, (char *) &ret, 1, true);
    return ret;
}

inline unsigned short recvShort(int socket) {
    unsigned short ret;
    safeRecv(socket, (char *) &ret, 2, true);
    return ntohs(ret);
}

inline unsigned int recvInt(int socket) {
    unsigned int ret;
    safeRecv(socket, (char *) &ret, 4, true);
    return ntohl(ret);
}

inline unsigned long long recvLong(int socket) {
    unsigned long long ret;
    safeRecv(socket, (char *) &ret, 8, true);
    return ntohll(ret);
}

inline std::string recvString(int socket) {
    // Get string length
    short length = recvShort(socket);
    // Get string
    char *cp = (char *) malloc(length + 1);
    if (cp == NULL) {
        std::cout << "Failed to allocate string!\n";
        exit(1);
    }
    safeRecv(socket, cp, length, true);
    cp[length] = 0;
    std::string str;
    str += cp;
    free(cp);
    return str;
}

// TODO: endianness

inline static float recvFloat(int socket) {
    float ret;
    unsigned char bytes[sizeof(float) + 1];
    bytes[sizeof(float)] = 0;
    safeRecv(socket, (char *) bytes, sizeof(float), true);
    std::reverse(bytes, bytes + sizeof(float));
    memcpy((char*)&ret, bytes, sizeof(float));
    return ret;
}

inline static double recvDouble(int socket, bool wtf = false) {
    double ret;
    unsigned char bytes[sizeof(double) + 1];
    bytes[sizeof(double)] = 0;
    safeRecv(socket, (char*)bytes, sizeof(double), true);
    std::reverse(bytes, bytes + sizeof(double));
    memcpy((char*)&ret, bytes, sizeof(double));
    return ret;
}
