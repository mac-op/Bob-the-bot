#include "BobTheBot.h"

void BobTheBot::OnGameStart() {
    actions->SendChat("\nBob the Bot\nCan we fix it ?\nBob the Bot\nYes, we can!!");

    // Get all possible locations we can expand to
    getExpansionLocations();

    // Add the geysers near the initial command senter to the queue of geysers to build a refinery on
    geysersToBuildOn.insert(geysersToBuildOn.begin(), FindNearest(initialCommCen->pos, isGeyser));
    geysersToBuildOn.insert(geysersToBuildOn.begin(), FindSecondNearest(initialCommCen->pos, isGeyser));
}


void BobTheBot::OnStep() {
    //Throttle some behavior that can wait to avoid duplicate orders.
    int frames_to_skip = 4;
    if (observer->GetFoodUsed() >= observer->GetFoodCap()) {
        frames_to_skip = 6;
    }

    if (Observation()->GetGameLoop() % frames_to_skip != 0) {
        return;
    }

    SupplyDepotManager(7);
    ContinuousSCVSpawn(2);
    CommandCenterManager();
    RefineryManager();
    ManageOffensive();
}


void BobTheBot::OnUnitIdle(const Unit* unit) {
    switch (unit->unit_type.ToType()) {
        case UNIT_TYPEID::TERRAN_BARRACKS: {
            actions->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
            break;
        }

        case UNIT_TYPEID::TERRAN_SCV: {
            MineMinerals(unit);
            break;
        }
        default: {
            break;
        }
    }
}

