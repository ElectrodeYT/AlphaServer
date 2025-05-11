#include "PlayerManager.h"

void PlayerManager::DoChunkLoading() {
    for (auto & client : clients) {
        // Only load chunks if in game state
        if (client->state != AlphaClient::GAME) { continue; }

        if (client->kicked) { continue; } // Do not process kicked clients

        client->DoChunks();
    }
}

void PlayerManager::BroadcastChat(const std::string& message) {
    for (auto & client : clients) {
        if (client->kicked) { continue; } // Do not process kicked clients
        try {
            client->SendChat(message);
        } catch (...) {
            client->DoKick("Exception while sending chat message!");
        }
    }
}

void PlayerManager::HandlePlayers() {
    for (int i = 0; i < clients.size(); i++) {
        // Remove this client if it was kicked
        if (clients[i]->kicked) {
            clients.erase(clients.begin() + i);
            i--;
            continue;
        }

        try {
            clients[i]->DoClient();
            if (!clients[i]->chat_buffer.empty()) {
                BroadcastChat(clients[i]->name + ": " + clients[i]->chat_buffer[0]);
                clients[i]->chat_buffer.erase(clients[i]->chat_buffer.begin());
            }
        } catch (...) {
            clients[i]->DoKick("Error handling this client!");
            clients.erase(clients.begin() + i);
            i--;
        }
    }

    // If we have chunk updates, we now do them here
    while(!chunks.updates.empty()) {
        BroadcastBlockUpdate(chunks.updates.front());
        chunks.updates.pop();
    }

    DoChunkLoading();
}

void PlayerManager::HandleGameTick() {
    for (int i = 0; i < clients.size(); i++) {
        // Remove this client if it was kicked
        if (clients[i]->kicked) {
            clients.erase(clients.begin() + i);
            i--;
            continue;
        }

        try {
            clients[i]->DoGameTick();
        } catch (...) {
            clients[i]->DoKick("Error during gametick with this client!");
            delete clients[i];
            clients.erase(clients.begin() + i);
            i--;
        }
    }
}

void PlayerManager::BroadcastBlockUpdate(ChunkManager::BlockUpdate update) {
    for(auto & client : clients) {
        client->SendBlockUpdate(update);
    }
}
