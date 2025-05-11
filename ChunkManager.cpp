#include "ChunkManager.h"

#define ChunkCoordsFromBlock() int chunk_x = (block.x >> 4), chunk_y = (block.z >> 4); // if(block.x < 0) { chunk_x--; } if(block.z < 0) { chunk_y--; }

Chunk& ChunkManager::GetChunk(int x, int y, bool mark) {
    // Try and find chunk
    for (int i = 0; i < chunks.size(); i++) {
        if (chunks[i].x == x && chunks[i].y == y) {
            if (mark) { chunks[i].chunk_is_loaded_by_players = true; }
            return chunks[i];
        }
    }

    // Chunk not found, make new one
    std::cout << "Generating Chunk " << x << "/" << y << std::endl;
    Chunk new_chunk(x, y);
    new_chunk.Generate();
    if (mark) { new_chunk.chunk_is_loaded_by_players = true; }
    chunks.push_back(new_chunk);
    return chunks[chunks.size() - 1];
}

void ChunkManager::MarkAllAsNotLoadedByClients() {
    for (int i = 0; i < chunks.size(); i++) {
        chunks[i].chunk_is_loaded_by_players = false;
    }
}

void ChunkManager::RemoveAllNotMarkedChunks() {
    for (int i = 0; i < chunks.size(); i++) {
        if (!chunks[i].chunk_is_loaded_by_players) { chunks[i].Unload(); }
    }
}

int ChunkManager::GetBlock(const Vec3i& block) {
    //int chunk_x = block.x >> 4;
    //int chunk_y = block.z >> 4;
    ChunkCoordsFromBlock();
    Chunk& chunk = GetChunk(chunk_x, chunk_y);
    int block_x = block.x & 0xF;
    int block_z = block.z & 0xF;

    return chunk.GetBlock(Vec3i(block_x, block.y, block_z));
}

void ChunkManager::UpdateBlock(Vec3i& block, int val) {
    //int chunk_x = block.x / 16;
    //int chunk_y = block.z / 16;
    ChunkCoordsFromBlock();
    Chunk& chunk = GetChunk(chunk_x, chunk_y);
    int block_x = block.x & 0xF;
    int block_z = block.z & 0xF;

    //std::cout << "Updated block in " << chunk_x << "/" << chunk_y << std::endl;
    block.y--;
    //std::cout << "\told block: " << GetBlock(block) << std::endl;
    // TODO: it seems that for the notchian client our chunk height is offset by one
    //       need to figure out if this is a really bad bug or not lol
    chunk.SetBlock(Vec3i(block_x, block.y, block_z), (unsigned char)val, true);
    //std::cout << "\tnew block: " << GetBlock(block) << std::endl;
    block.y++;
    updates.emplace(block, val);
}
