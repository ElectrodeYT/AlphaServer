#pragma once

#include <string>
#include <vector>

#include "ChunkManager.h"
#include "Client.h"

class PlayerManager {
public:
    PlayerManager(ChunkManager& chunk_manager) : chunks(chunk_manager) {}

    void DoChunkLoading();

    void BroadcastChat(const std::string& message);
    void BroadcastBlockUpdate(ChunkManager::BlockUpdate update);

    void HandlePlayers();

    void HandleGameTick();

    void AddConnectedClient(AlphaClient* client) {
        client->has_been_initialized = false;
        clients.push_back(client);
    }

private:
    std::vector<AlphaClient*> clients;
    ChunkManager& chunks;
};

