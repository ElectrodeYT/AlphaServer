#include "PlayerManager.h"

void PlayerManager::DoChunkLoading() {
    for (int i = 0; i < clients.size(); i++) {
        // Only load chunks if in game state
        if (clients[i].state != AlphaClient::GAME) { continue; }

        if (clients[i].kicked) { continue; } // Do not process kicked clients

        clients[i].DoChunks(chunks);
    }
}

void PlayerManager::BroadcastChat(std::string message) {
    for (int i = 0; i < clients.size(); i++) {
        if (clients[i].kicked) { continue; } // Do not process kicked clients
        try {
            clients[i].SendChat(message);
        } catch (...) {
            clients[i].DoKick("Exception while sending chat message!");
        }
    }
}

void PlayerManager::HandlePlayers() {
    for (int i = 0; i < clients.size(); i++) {
        // Remove this client if it was kicked
        if (clients[i].kicked) {
            clients.erase(clients.begin() + i);
            i--;
            continue;
        }

        try {
            clients[i].DoClient(chunks);
            if (clients[i].chat_buffer.size() > 0) {
                BroadcastChat(clients[i].name + ": " + clients[i].chat_buffer[0]);
                clients[i].chat_buffer.erase(clients[i].chat_buffer.begin());
            }
        } catch (...) {
            clients[i].DoKick("Error handling this client!");
            clients.erase(clients.begin() + i);
            i--;
        }
    }

    DoChunkLoading();
}

void PlayerManager::HandleGameTick() {
    for (int i = 0; i < clients.size(); i++) {
        // Remove this client if it was kicked
        if (clients[i].kicked) {
            clients.erase(clients.begin() + i);
            i--;
            continue;
        }

        try {
            clients[i].DoGameTick(chunks);
        } catch (...) {
            clients[i].DoKick("Error during gametick with this client!");
            clients.erase(clients.begin() + i);
            i--;
        }
    }
}
