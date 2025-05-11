#pragma once

#include <vector>
#include <queue>
#include "Chunk.h"
#include "Vectors.h"

class ChunkManager {
public:
    // Returns a chunk.
    Chunk& GetChunk(int x, int y, bool mark = true);

    Chunk& GetChunk(Vec2i pos, bool mark = true) {
        return GetChunk(pos.x, pos.y, mark);
    }

    // Call before doing player chunk loading logic.
    void MarkAllAsNotLoadedByClients();

    // Call after doing player chunk loading logic
    void RemoveAllNotMarkedChunks();

    // Get a specific block, and generate the chunk if not yet available
    int GetBlock(const Vec3i& block);

    // Update a block in a chunk
    // This will also mark the chunk dirty, and add a block to the queue for the player manager to send out
    // single block updates for
    void UpdateBlock(Vec3i& block, int val);

    struct BlockUpdate {
        Vec3i block;
        int val = 0;

        BlockUpdate() = default;
        BlockUpdate(const Vec3i& b, int v) : block(b), val(v) { }
    };

    std::queue<BlockUpdate> updates;

    ChunkManager() = default;

private:
    ChunkManager(const ChunkManager&);
    ChunkManager& operator=(const ChunkManager&);

    std::vector<Chunk> chunks;
};

