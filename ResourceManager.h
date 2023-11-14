//
// Created by huyta on 14-Nov-23.
//
#pragma once

#include <queue>
#include "sc2api/sc2_interfaces.h"

namespace Strategy
{
    /*
     * ResourceManager has a one-to-one relationship to sc2::Agent
     */
    class ResourceManager
    {
    public:
        explicit ResourceManager(sc2::Agent& agent) : agent(agent){ }
        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        void Step();

    private:
        // Note: not sure if ActionRaw is appropriate, there may be other candidates
        // for our purposes ( could possibly use ABILITY_ID )
        std::queue<sc2::ActionRaw> actionQueue;
        sc2::Agent& agent;
    };
}

