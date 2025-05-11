//
// Created by alexander on 12/9/22.
//

#ifndef ALPHASERVER_ENTITYMANAGER_H
#define ALPHASERVER_ENTITYMANAGER_H


#include <string>
#include "Vectors.h"

class EntityManager {
public:
    int AddNewPlayerEntity(std::string name, Vec3f startingpos);


    void UpdateEntityPos(int id, Vec3f new_pos);

    void BroadcastUpdates(int socket);
    void ClearUpdates();
private:


};


#endif //ALPHASERVER_ENTITYMANAGER_H
