#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <fcntl.h>

#include "PlayerManager.h"
#include "ChunkManager.h"
#include "Packets.h"
#include "Client.h"

#ifdef _WIN32
#include <winsock2.h>
#elif linux

#include <sys/socket.h>

#else
#error unsupported OS
#endif


ChunkManager chunks;

int main() {
    std::cout << "AlphaServer, hosting on port 25565\n";

    PlayerManager players(chunks);

    std::cout << "Generating spawn chunks...\n";
    for (int x = -20; x <= 20; x++) {
        for (int y = -20; y <= 20; y++) {
            chunks.GetChunk(x, y);
        }
    }
    std::cout << "Done generating spawn chunks.\n";
    std::cout << "Generating spawn chunk server data...\n";
    for (int x = -20; x <= 20; x++) {
        for (int y = -20; y <= 20; y++) {
            chunks.GetChunk(x, y).GenerateBlockServerData();
        }
    }
    std::cout << "Done.\n";


    std::cout << "---- DONE ----\n";
    // Platform independant variables
    int socketfd;
    unsigned long opt = 1;
    struct sockaddr_in server;

#ifdef _WIN32
    // Windows variables for socket creation
    WSADATA wsa;
    // Initialize Winsock2
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cout << "Failed to start Winsock2. Error code: " << WSAGetLastError() << "\n";
        exit(1);
    }
    // Initialize socket
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == INVALID_SOCKET) {
        std::cout << "Failed to create socket : %d" << WSAGetLastError() << "\n";
    }

    // Prepare socket address structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(25565);

    // Bind the socket
    if (bind(socketfd, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        std::cout << "Bind failed with error code: " << WSAGetLastError() << "\n";
    }

    // Put socket into non-blocking mode
    ioctlsocket(socketfd, FIONBIO, &opt);
    // Start listening
    listen(socketfd, 3);
#elif linux
    // Initialize socket
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1) {
        perror("Failed to create socket");
        exit(-1);
    }

    // Prepare socket address structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(25565);

    // Bind the socket
    if (bind(socketfd, (struct sockaddr*) &server, sizeof(server)) == -1) {
        perror("Bind failed");
        exit(-1);
    }

    // Put socket into non-blocking mode
    if (fcntl(socketfd, F_SETFL, fcntl(socketfd, F_GETFL, 0) | O_NONBLOCK) == -1) {
        perror("fcntl failed");
    }
    // Start listening
    listen(socketfd, 3);
#endif


    auto tick_calc_time_from = std::chrono::high_resolution_clock::now();
    int llll = 0;
    while (true) {
        // Check if new clients have connected
        struct sockaddr_in address;
        unsigned int address_size = sizeof(address);
        int accept_return = accept(socketfd, (struct sockaddr*) &address, &address_size);
#ifdef _WIN32
        if (accept_return == INVALID_SOCKET) {
            // Check if the error is a WOULDBLOCK error or not
            int wsa_error = WSAGetLastError();
            if (wsa_error != WSAEWOULDBLOCK) { std::cout << "Accept error that was not WOULDBLOCK: " << wsa_error << "\n"; closesocket(socketfd); exit(1); }
#elif linux
        if (accept_return == -1) {
            // Check if the error is a non blocking error
            if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
                perror("Accept error");
                close(socketfd);
                exit(-1);
            }
#endif
        } else {
            // A connection was made
            AlphaClient* new_client = new AlphaClient(chunks);
            new_client->socket = accept_return;
            new_client->state = AlphaClient::HANDSHAKE_WAIT;
            new_client->has_been_initialized = false;
            std::cout << "New client connected: " << inet_ntoa(address.sin_addr) << "\n";
            // Add client to client list
            players.AddConnectedClient(new_client);
        }

        // Loop through every client
        players.HandlePlayers();

        // Check if a game tick needs to be performed
        auto tick_calc_time_now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(tick_calc_time_now - tick_calc_time_from).count() >=
            50) {
            // Reset from time (so that the next things dont get counted
            tick_calc_time_from = std::chrono::high_resolution_clock::now();
            players.HandleGameTick();
        }
    }
}