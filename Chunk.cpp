#include <iostream>
#include <sstream>
#include <vector>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include "Packets.h"
#include "Chunk.h"

void Chunk::SendChunk(int socket) {
    // Calculate packet 0x33 X, Y, Z
    int packet_x = x << 4;
    short packet_y = 0;
    int packet_z = y << 4;
    // Packet X, Y, Z size
    unsigned char packet_sx = 15;
    unsigned char packet_sy = 127;
    unsigned char packet_sz = 15;

    // Check if we need to redo the chunk data
    if (chunk_dirty) {
        // Regenerate the block server data
        GenerateBlockServerData();
    }

    // boost things

    boost::iostreams::filtering_streambuf<boost::iostreams::output> output_stream;
    output_stream.push(boost::iostreams::zlib_compressor());
    std::stringstream string_stream;
    output_stream.push(string_stream);
    boost::iostreams::copy(boost::iostreams::basic_array_source<char>(chunk_data.data.data(), chunk_data.data.size()),
                           output_stream);

    // now, send the packet!
    sendVar(socket, (unsigned char) 0x33);
    sendVar(socket, (unsigned int) packet_x);
    sendVar(socket, (unsigned short) packet_y);
    sendVar(socket, (unsigned int) packet_z);
    sendVar(socket, (unsigned char) packet_sx);
    sendVar(socket, (unsigned char) packet_sy);
    sendVar(socket, (unsigned char) packet_sz);
    sendVar(socket, (unsigned int) string_stream.str().length());
    send(socket, string_stream.str().data(), string_stream.str().length(), 0);
}

void Chunk::Generate() {
    // Set all blocks below level 64 as stone
    for (int y = 0; y < 30; y++) {
        for (int x = 0; x < 16; x++) {
            for (int z = 0; z < 16; z++) {
                SetBlock(Vec3i(x, y, z), 1);
            }
        }
    }

    // Make every block have a lot of light
    for (int y = 0; y < 30; y++) {
        for (int x = 0; x < 16; x++) {
            for (int z = 0; z < 16; z++) {
                SetBlockLight(Vec3i(x, y, z), 255);
            }
        }
    }

    // Make every block have a lot of light
    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            SetBlockSkyLight(Vec3i(x, 30, z), 255);
        }
    }
}

void Chunk::GenerateBlockServerData() {
    // Clear the chunk data
    chunk_data.Clear();
    chunk_data.AddByteArray((unsigned char *) blocks, sizeof(blocks));
    chunk_data.AddByteArray((unsigned char *) blocks_metadata_data, 17472);
    chunk_data.AddByteArray((unsigned char *) blocks_light_data, 17472);
    chunk_data.AddByteArray((unsigned char *) blocks_sky_light_data, 17472);
    chunk_dirty = false;
}
