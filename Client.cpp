#include <iostream>
#include <string>
#include <vector>

#include "Packets.h"
#include "Client.h"
#include "ChunkManager.h"

// Maximum number of packets per client to handle at once
#define MAX_PACKTES_PER_CLIENT 25

void AlphaClient::DoClient(ChunkManager &chunks) {
    if (kicked) { return; } // Do not process a kicked client
    try {
        switch (state) {
            case AlphaClient::HANDSHAKE_WAIT: {
                // check if the socket has data availble to be received
                char packet_id;
                if (safeRecv(socket, &packet_id, 1) != 0) { return; }
                name = recvString(socket);

                // Send own handshake
                sendVar(socket, (unsigned char) 0x02);
                sendVar(socket, unique_hash_str);

                std::cout << "Sent handshake for client " << name << "\n";

                // Set state
                state = AlphaClient::LOGIN_WAIT;
            }
            case AlphaClient::LOGIN_WAIT: {
                // check if the socket has data availble to be received
                char packet_id;
                if (safeRecv(socket, &packet_id, 1) != 0) { return; }

                // Get login request
                int protoversion = recvInt(socket);
                std::string username = recvString(socket);
                std::string password = recvString(socket);
                long long unused_seed = recvLong(socket);
                unsigned char unused_dimension = recvChar(socket);

                // Check if the login request is valid
                if (protoversion != 6) {
                    // Send kick
                    sendVar(socket, (unsigned char) 0xff);
                    sendVar(socket, "Wrong minecraft version! This server only supports MC Protocol 6");
#ifdef _WIN32
                    closesocket(socket);
#elif linux
                    close(socket);
#endif
                    // Remove client
                    std::cout << "Kicked client, wrong protocol version. Got version " << protoversion << "\n";
                    kicked = true;
                    return;
                }
                if (username != name) {
                    // Send kick
                    sendVar(socket, (unsigned char) 0xff);
                    sendVar(socket, "MC Bug: Handshake and login respone names differ");
#ifdef _WIN32
                    closesocket(socket);
#elif linux
                    close(socket);
#endif
                    // Remove client
                    std::cout << "Kicked client handshake and login names differed. Got name " << username << "\n";
                    kicked = true;
                    return;
                }

                /// TODO: Add player to entity manager or something


                // Send login response
                sendVar(socket, (unsigned char) 0x01);
                sendVar(socket, (unsigned int) entity);
                sendVar(socket, "");
                sendVar(socket, "");
                sendVar(socket, (unsigned long long) 1234567);
                sendVar(socket, (unsigned char) dimension);

                std::cout << "Sent login response for client " << name << "\n";

                state = AlphaClient::GAME;
                break;
            }
            case AlphaClient::GAME: {
                // check if the client has been initialized
                if (!has_been_initialized) {
                    std::cout << "Initializing client " << name << "\n";

                    // Send chunks
                    DoChunks(chunks);

                    // Send spawn position
                    sendVar(socket, (unsigned char) 0x06);
                    sendVar(socket, (unsigned int) 0);
                    sendVar(socket, (unsigned int) 64);
                    sendVar(socket, (unsigned int) 0);

                    // Ensure the inventory has been sent
                    //SendInventory();
                    inventory_dirty = false;

                    // Send player position
                    SendPositionAndRotation();

                    // Send player health
                    //SendHealth();

                    // Send world time
                    //SendTime(1000);

                    // Flag this client as initialized
                    has_been_initialized = true;
                    std::cout << "Initialized client " << name << "\n";
                }
                if (inventory_dirty) {
                    SendInventory();
                    inventory_dirty = false;
                }
                // check if the socket has data available to be received
                unsigned char packet_id = 0;
                int packet_handled_count = 0;
                while (packet_handled_count++ < MAX_PACKTES_PER_CLIENT && safeRecv(socket, (char *) &packet_id, 1) == 0) {
                    //std::cout << "Got packet: " << std::hex << (unsigned int) packet_id << std::dec << "\n";
                    switch (packet_id) {
                        // Keep-alive
                        case 0: {
                            keepalive = 0; // Reset kick alive counter
                            break;
                        }
                        // Chat message
                        case 0x03: {
                            chat_buffer.push_back(recvString(socket));
                            break;
                        }
                        // Flying
                        case 0xa: {
                            on_ground = recvChar(socket) == 1;
                            DoCollision(chunks);
                            break;
                        }
                        // Player position
                        case 0x0b: {
                            pos.x = recvDouble(socket);
                            pos.y = recvDouble(socket, true);
                            stance = recvDouble(socket) - pos.y;
                            pos.z = recvDouble(socket);
                            on_ground = recvChar(socket) == 1;
                            std::cout << "Updated player position: " << pos.x << " " << pos.y << " " << pos.z << "(stance: " << stance << ", on_ground: " << on_ground << ")\n";
                            DoCollision(chunks);
                            break;
                        }
                        // Player look
                        case 0x0c: {
                            rot.x = recvFloat(socket);
                            rot.y = recvFloat(socket);
                            on_ground = recvChar(socket) == 1;
                            break;
                        }
                        // Player position and look
                        case 0x0d: {
                            pos.x = recvDouble(socket);
                            stance = recvDouble(socket);
                            pos.y = recvDouble(socket);
                            pos.z = recvDouble(socket);
                            rot.x = recvFloat(socket);
                            rot.y = recvFloat(socket);
                            on_ground = recvChar(socket) == 1;
                            //std::cout << "Updated player position and rotation: " << pos.x << " " << pos.y << " "
                            //          << pos.z << " / " << rot.x << " " << rot.y << "\n";
                            DoCollision(chunks);
                            break;
                        }
                        // Arm animation
                        case 0x12: {
                            // We just fart these into the void for now
                            // TODO: handle arms properly
                            recvInt(socket);
                            recvChar(socket);
                            break;
                        }

                        // Disconnect
                        case 0xff: {
                            std::cout << "Player " << name << " disconnected\n";
                            DoKick("Disconnected");
                            kicked = true;
                            return;
                        }
                        default: {
                            // Kick player for invalid or not supported paket
                            DoKick("Internal server error (unsupported packet id)");
                            // Remove client
                            std::cout << "Kicked client, Invalid or unsupported packet id: " << std::hex
                                      << (int) packet_id << std::dec << "\n";
                            kicked = true;
                            return;
                        }
                    }
                }
                break;
            }
        }
    } catch (std::runtime_error) {
        // Remove client
        std::cout << "Kicked client " << name << ", disconnected (socket error)\n";
        kicked = true;
        return;
    }
}

void AlphaClient::DoGameTick(ChunkManager &chunks) {
    if (kicked) { return; } // Do not process a kicked client

    // Increment keep-alive counter
    keepalive++;
    // Check if a kick should be performed
    if (keepalive > keepalive_kick) {
        // Kick player
        try {
            DoKick("Game client error (Did not receive a keep-alive packet for too long)!");
        } catch (...) {}
        // Remove client
        std::cout << "Kicked client, no keep-alive packet for " << keepalive_kick << " gameticks\n";
        kicked = true;
        return;
    }
    try {
        // Send chunks

    } catch (...) {
        // Kick player
        try {
            sendVar(socket, (unsigned char) 0xff);
            sendVar(socket, "Error processing client gametick!");
#ifdef _WIN32
            closesocket(socket);
#elif linux
            close(socket);
#endif
        } catch (...) {}
        // Remove client
        std::cout << "Kicked client, exception while proccessing gametick!\n";
        kicked = true;
        return;
    }
}

void AlphaClient::DoChunks(ChunkManager &chunks) {
    int chunk_x_center = (int) pos.x / 16;
    int chunk_y_center = (int) pos.z / 16;

    // Clear should be loaded vector
    chunk_should_be_loaded.clear();
    chunk_should_be_loaded.insert(chunk_should_be_loaded.begin(), loaded_chunks.size(), false);

    for (int x = chunk_x_center - 4; x <= chunk_x_center + 4; x++) {
        for (int y = chunk_y_center - 4; y <= chunk_y_center + 4; y++) {
            if (!EnsureChunkIsLoaded(Vec2i(x, y))) {
                // Load chunk
                std::cout << "Player " << name << " loaded chunk " << x << " " << y << "\n";
                Chunk &chunk = chunks.GetChunk(x, y);
                chunk.SendPrechunk(socket, true);
                chunk.SendChunk(socket);
            }
        }
    }

    // Unload all chunks that should not be loaded
    for (int x = 0; x < chunk_should_be_loaded.size(); x++) {
        if (!chunk_should_be_loaded[x]) {
            Chunk &unloaded_chunk = chunks.GetChunk(loaded_chunks[x], false);
            unloaded_chunk.SendPrechunk(socket, false);

            // Remove this chunk
            chunk_should_be_loaded.erase(chunk_should_be_loaded.begin() + x);
            loaded_chunks.erase(loaded_chunks.begin() + x);
            x--;
        }
    }
}

void AlphaClient::DoCollision(ChunkManager &chunks) {
    // Being below bedrock should not be possible, set position to 128
    //if (pos.y < 0) {
    //    pos.y = 66;
    //    SendPosition();
    //    std::cout << "Ejecting player " + name << " from below bedrock" << std::endl;
    //}
    // Check if the client is in a block
    //if (chunks.GetBlock(Vec3i((int)pos.x, (int)pos.y - 1, (int)pos.z)) != 0) {
    //	pos.y++;
    //	SendPositionAndRotation();
    //}
}

