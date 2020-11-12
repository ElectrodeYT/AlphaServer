#pragma once
#include <iostream>
#include <string>
#include <winsock2.h>

/// Returns 0 if data was available and received, not 0 if no data is present.
/// If any data is present, this function will wait for all the data of len to be received
/// If block is true, this function emulates a blocking call without having to change the socket to blocking
int static safeRecv(int socket, char* data, int len, bool block = false) {
	char* dat = data;
	int len_to_go = len;
	bool data_has_been_received = false;

	while (len_to_go > 0) {
		int data_received_len = recv(socket, dat, len_to_go, 0);
		if (data_received_len == SOCKET_ERROR) {
			int error = WSAGetLastError();
			if (error == WSAEWOULDBLOCK && !data_has_been_received && !block) { return -1; }
			if (error != WSAEWOULDBLOCK) { std::cout << "safeRecv got non-WOULDBLOCK error from recv: " << error << "\n";  throw std::runtime_error("socket error"); }
		} else {
			dat += data_received_len;
			len_to_go -= data_received_len;
		}
	}
	return 0;
}

// 
// Functions for sending variables
//

inline static void sendVar(int socket, unsigned char byte) {
	send(socket, (char*)& byte, 1, 0);
}
inline static void sendVar(int socket, unsigned short s) {
	unsigned short network_short = htons(s);
	send(socket, (char*)& network_short, sizeof(s), 0);
}
inline static void sendVar(int socket, unsigned int i) {
	unsigned int network_short = htonl(i);
	send(socket, (char*)& network_short, sizeof(i), 0);
}
inline static void sendVar(int socket, unsigned long long l) {
	unsigned long long network_short = htonll(l);
	send(socket, (char*)& network_short, sizeof(l), 0);
}
inline static void sendVar(int socket, float f) {
	send(socket, (char*)& f, sizeof(f), 0);
}
inline static void sendVar(int socket, double d) {
	send(socket, (char*)& d, sizeof(d), 0);
}
inline static void sendVar(int socket, std::string data, bool dont_send_length = false) {
	// Send string length as short
	short string_len = htons(data.size());
	if (!dont_send_length) { send(socket, (char*)& string_len, sizeof(string_len), 0); }
	// Now send string data
	send(socket, data.data(), data.size(), 0);
}


//
// Functions for receiving variables
//

inline unsigned char recvChar(int socket) {
	unsigned char ret;
	safeRecv(socket, (char*)& ret, 1, true);
	return ret;
}
inline unsigned short recvShort(int socket) {
	unsigned short ret;
	safeRecv(socket, (char*)& ret, 2, true);
	return ntohs(ret);
}
inline unsigned int recvInt(int socket) {
	unsigned int ret;
	safeRecv(socket, (char*)& ret, 4, true);
	return ntohl(ret);
}
inline unsigned long long recvLong(int socket) {
	unsigned long long ret;
	safeRecv(socket, (char*)& ret, 8, true);
	return ntohll(ret);
}
inline std::string recvString(int socket) {
	// Get string length
	short length = recvShort(socket);
	// Get string
	char* cp = (char*)malloc(length + 1);
	if (cp == NULL) { std::cout << "Failed to allocate string!\n"; exit(1); }
	safeRecv(socket, cp, length, true);
	cp[length] = 0;
	std::string str;
	str += cp;
	free(cp);
	return str;
}
inline static float recvFloat(int socket) {
	float ret;
	safeRecv(socket, (char*)& ret, sizeof(float), true);
	return ret;
}
inline static double recvDouble(int socket) {
	double ret;
	safeRecv(socket, (char*)& ret, sizeof(double), true);
	return ret;
}
