#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include "Packets.h"
#include "Vectors.h"

// Used for writing a nibble at a time
struct NibbleWriter {
    std::vector<char> data;
    bool at_nibble = false;

    void Clear() {
        at_nibble = false;
        data.clear();
        data.push_back(0);
    }

    void AddByte(unsigned char b) {
        if (at_nibble) {
            data[data.size() - 1] |= ((b & 0xF0) >> 4);
            data.push_back(((b & 0x0F) << 4));
        } else {
            data.push_back(b);
        }
    }

    void AddByteArray(unsigned char b[], int size) {
        if (at_nibble) {
            std::cout << "-------SERVER BUG-------\nNibble writer tried to add byte array at nibble position!\n";
            abort();
        } else {
            data.insert(data.end(), b, b + size);
        }
    }

    void AddNibble(unsigned char b) {
        b &= 0x0F;
        if (at_nibble) {
            data[data.size() - 1] |= b;
            data.push_back(0);
        } else {
            data[data.size() - 1] |= (b << 4);
        }

        at_nibble = !at_nibble;
    }

    NibbleWriter() {
        // Clear everything
        Clear();
    }
};

// One single chunk
struct Chunk {
    // Easy to do since the chunk data expects it like this
    unsigned char blocks[16][16][128];
    //char* blocks;

    // These arent
    //char blocks_metadata_data[16384];
    //char blocks_light_data[16384];
    //char blocks_sky_light_data[16384];

    unsigned char* blocks_metadata_data;
    unsigned char* blocks_light_data;
    unsigned char* blocks_sky_light_data;

    inline void SetBlock(Vec3i block, unsigned char id, bool log = false) {
        //blocks[block.y + (block.z * 128) + (block.x * 128 * 16)] = id;
        blocks[block.x][block.z][block.y] = id;
        chunk_dirty = true;
        if(log) {
            std::cout << "Changed block " << block.x << "/" << block.y << "/" << block.z << " in chunk " << x << "/" << y << " to " << (int)id << std::endl;
        }
    }

    inline int GetBlock(Vec3i block) {
        //return blocks[block.y + (block.z * 128) + (block.x * 128 * 16)];
        return blocks[block.x][block.z][block.y];
    }

    void SetBlockMetadata(Vec3i block, char data) {
        int index = block.y + (block.z * 128) + (block.x * 128 * 16);
        index /= 2;
        int entry_mask = index % 2 ? 0xF0 : 0x0F;
        int data_mask = index % 2 ? data : data << 4;
        blocks_metadata_data[index] &= entry_mask;
        blocks_metadata_data[index] |= data_mask;
        chunk_dirty = true;
    }

    void SetBlockLight(Vec3i block, char data) {
        int index = block.y + (block.z * 128) + (block.x * 128 * 16);
        index /= 2;
        int entry_mask = index % 2 ? 0xF0 : 0x0F;
        int data_mask = index % 2 ? data : data << 4;
        blocks_light_data[index] &= entry_mask;
        blocks_light_data[index] |= data_mask;
        chunk_dirty = true;
    }

    void SetBlockSkyLight(Vec3i block, char data) {
        int index = block.y + (block.z * 128) + (block.x * 128 * 16);
        index /= 2;
        int entry_mask = index % 2 ? 0xF0 : 0x0F;
        int data_mask = index % 2 ? data : data << 4;
        blocks_sky_light_data[index] &= entry_mask;
        blocks_sky_light_data[index] |= data_mask;
        chunk_dirty = true;
    }

    int x;
    int y;

    // Used for auto chunk unloading logic
    bool chunk_is_loaded_by_players = false;

    // Will be compressed later
    NibbleWriter chunk_data;
    bool chunk_dirty = true;

    Chunk(int chunk_x, int chunk_y) : x(chunk_x), y(chunk_y) {
        // Allocate memory
        //blocks = (char*)malloc(34944);
        blocks_metadata_data = (unsigned char*) malloc(17472);
        blocks_light_data = (unsigned char*) malloc(17472);
        blocks_sky_light_data = (unsigned char*) malloc(17472);
        if (blocks == 0 || blocks_metadata_data == 0 || blocks_light_data == 0 || blocks_sky_light_data == 0) {
            std::cout << "CHUNK ERROR: Failed to allocate data!\n";
            abort();
        }
        // Zero everything
        memset((void*) blocks, 0, sizeof(blocks));
        memset((void*) blocks_metadata_data, 0, 17472);
        memset((void*) blocks_light_data, 0, 17472);
        memset((void*) blocks_sky_light_data, 0, 17472);
    }


    // Sends prechunk paket.
    void SendPrechunk(int socket, bool load_chunk) {
        sendVar(socket, (unsigned char) 0x32);
        sendVar(socket, (unsigned int) x);
        sendVar(socket, (unsigned int) y);
        sendVar(socket, (unsigned char) load_chunk);
    }

    // Ensure a prechunk was sent first!
    void SendChunk(int socket);

    void Unload() {
        // Simply clear the NibbleWriter Data and set the dirty flag
        chunk_data.Clear();
        chunk_dirty = true;
    }

    void Generate();

    void GenerateBlockServerData();

    void Unallocate() {
        free(blocks_metadata_data);
        free(blocks_light_data);
        free(blocks_sky_light_data);
    }
};