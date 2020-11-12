#pragma once
#include <vector>
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
	int GetBlock(Vec3i block);
private:
	std::vector<Chunk> chunks;
};

