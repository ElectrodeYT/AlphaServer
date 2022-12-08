#pragma once

#include <iostream>
#include "ChunkManager.h"
#include "Packets.h"
#include "Vectors.h"

struct AlphaClient {
    // Connection info
    int socket;
    std::string unique_hash_str;
    unsigned int unique_hash;
    enum State {
        HANDSHAKE_WAIT = 0,
        LOGIN_WAIT,
        GAME
    };
    State state;

    // Player info
    std::string name = "";
    int entity = 100; // Entity id
    int dimension = 0; // Dimension player is in
    unsigned char health = 20; // Health of player, from 0 - 20 (0 is interpreted by the client as dead)
    Vec3d pos;
    // X is yaw, Y is pitch
    Vec2f rot;
    double stance = 1.6200000047683716; // Weird thing this is
    bool on_ground = false;

    // Inventory
    short inv[36];
    unsigned char inv_c[36];
    unsigned short inv_h[36];
    short armour[4];
    unsigned char armour_c[4];
    unsigned short armour_h[4];
    short crafting_slots[4];
    unsigned char crafting_slots_c[4];
    unsigned short crafting_slots_h[4];

    // Internal info
    bool kicked = false;
    bool has_been_initialized = false;
    bool inventory_dirty = true; // Used to signal that the inventory slots have been updated

    std::vector<std::string> chat_buffer;

    // Chunk Info
    std::vector<Vec2i> loaded_chunks;
    std::vector<bool> chunk_should_be_loaded;

    int keepalive = 0;
    int keepalive_kick = 1000;

    AlphaClient() {
        GenerateHash();
        for (int i = 0; i < 36; i++) {
            if (i < 4) {
                armour[i] = -1;
                armour_c[i] = 0;
                armour_h[i] = 0;
                crafting_slots[i] = -1;
                crafting_slots_c[i] = 0;
                crafting_slots_h[i] = 0;
            }
            inv[i] = -1;
            inv_c[i] = 0;
            inv_h[i] = 0;
        }

        // Setup basic, valid position
        pos.x = 0;
        pos.y = 70;
        pos.z = 0;
        //stance = pos.y + 0.3;
        rot.x = 0;
        rot.y = 0;
    }

    void GenerateHash() {
        // Generate random number
        unique_hash = rand();
        // Convert it to string as hex
        char *data = (char *) malloc(8);
        if (data == NULL) {
            perror("Failed to generate random hash");
            exit(1);
        }
        unique_hash_str = "-";
        free(data);
    }

    // Sending packets
    void SendInventory() {
        // Send inventory
        sendVar(socket, (unsigned char) 0x05);
        sendVar(socket, (unsigned int) -1);
        sendVar(socket, (unsigned short) 36);
        for (int i = 0; i < 36; i++) {
            sendVar(socket, (unsigned short) inv[i]);
            if (inv[i] != -1) {
                // Send count (byte) and health (short)
                sendVar(socket, (unsigned char) inv_c[i]);
                sendVar(socket, (unsigned short) inv_h[i]);
            }
        }

        // Send armour
        sendVar(socket, (unsigned char) 0x05);
        sendVar(socket, (unsigned int) -2);
        sendVar(socket, (unsigned short) 4);
        for (int i = 0; i < 4; i++) {
            sendVar(socket, (unsigned short) armour[i]);
            if (inv[i] != -1) {
                // Send count (byte) and health (short)
                sendVar(socket, (unsigned char) armour_c[i]);
                sendVar(socket, (unsigned short) armour_h[i]);
            }
        }

        // Send crafting table
        sendVar(socket, (unsigned char) 0x05);
        sendVar(socket, (unsigned int) -3);
        sendVar(socket, (unsigned short) 4);
        for (int i = 0; i < 4; i++) {
            sendVar(socket, (unsigned short) crafting_slots[i]);
            if (inv[i] != -1) {
                // Send count (byte) and health (short)
                sendVar(socket, (unsigned char) crafting_slots_c[i]);
                sendVar(socket, (unsigned short) crafting_slots_h[i]);
            }
        }

    }

    void SendPositionAndRotation() {
        // Sends SERVER version of player position and look paket
        sendVar(socket, (unsigned char) 0x0d);
        sendVar(socket, (double)pos.x);
        sendVar(socket, (double)(pos.y + stance));
        sendVar(socket, (double)pos.y);
        sendVar(socket, (double)pos.z);
        sendVar(socket, (float)rot.x);
        sendVar(socket, (float)rot.y);
        sendVar(socket, (unsigned char) on_ground);
        std::cout << "Sent pos+rot: " << pos.x << " " << pos.y << " " << pos.z << " / " << rot.x << " " << rot.y
                  << "\n";
    }

    void SendPosition() {
        sendVar(socket, (unsigned char)0xb);
        sendVar(socket, (double)pos.x);
        sendVar(socket, (double)pos.y);
        sendVar(socket, (double)stance);
        sendVar(socket, (double)pos.z);
        sendVar(socket, (unsigned char) on_ground);
        std::cout << "Sent pos: " << pos.x << " " << pos.y << " " << pos.z << "\n";
    }

    void SendHealth() {
        sendVar(socket, (unsigned char) 0x08);
        sendVar(socket, health);
    }

    void SendChat(std::string message) {
        // Chat message is packet 0x03
        sendVar(socket, (unsigned char) 0x03);
        sendVar(socket, message);
    }

    void SendTime(long long time) {
        sendVar(socket, (unsigned char) 0x04);
        sendVar(socket, (unsigned long long) time);
    }

    /// Performs network activity for this client.
    /// Variable kicked should be checked after running this function.
    void DoClient(ChunkManager &chunks);

    /// Perform per-tick activity for this client.
    /// Will kick the player upon a serious problem, but checking ist not strictly required.
    void DoGameTick(ChunkManager &chunks);

    /// Do Kick logic
    void DoKick(std::string reason) {
        try {
            sendVar(socket, (unsigned char) 0xff);
            sendVar(socket, reason);
#ifdef _WIN32
            closesocket(socket);
#elif linux
            close(socket);
#endif
        } catch (...) {} // Ignore any and all exceptions
        kicked = true;
    }

    // Ensure that a chunk is loaded
    // Returns true if chunk is presently loaded.
    bool EnsureChunkIsLoaded(Vec2i chunk) {
        // Try and find chunk in chunk list
        for (int i = 0; i < loaded_chunks.size(); i++) {
            if (loaded_chunks[i] == chunk) {
                chunk_should_be_loaded[i] = true;
                return true;
            }
        }

        // Not found, add the chunk and return true
        loaded_chunks.push_back(chunk);
        chunk_should_be_loaded.push_back(true);
        return false;
    }

    // Load/Unload/Do nothing if it doesnt need to be done with chunks
    void DoChunks(ChunkManager &chunks);

    // Handle collisions
    void DoCollision(ChunkManager &chunks);
};