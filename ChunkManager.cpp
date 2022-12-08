#include "ChunkManager.h"

Chunk &ChunkManager::GetChunk(int x, int y, bool mark) {
    // Try and find chunk
    for (int i = 0; i < chunks.size(); i++) {
        if (chunks[i].x == x && chunks[i].y == y) {
            if (mark) { chunks[i].chunk_is_loaded_by_players = true; }
            return chunks[i];
        }
    }

    // Chunk not found, make new one
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

int ChunkManager::GetBlock(Vec3i block) {
    int chunk_x = block.x / 16;
    int chunk_y = block.z / 16;
    Chunk &chunk = GetChunk(chunk_x, chunk_y);
    int block_x = block.x % 16;
    int block_z = block.z % 16;

    return chunk.GetBlock(Vec3i(block_x, block.y, block_z));
}
