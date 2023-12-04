#include "BobTheBot.h"

void BobTheBot::OnGameStart() {
    actions->SendChat("\nBob the Bot\nCan we fix it ?\nBob the Bot\nYes, we can!!");

    // Get all possible locations we can expand to
    getExpansionLocations();

    latestCommCen = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_COMMANDCENTER))[0];

    // Add the geysers near the initial command senter to the queue of geysers to build a refinery on
    geysersToBuildOn.insert(geysersToBuildOn.begin(), FindNearest(observer->GetStartLocation(), UNIT_TYPEID::NEUTRAL_VESPENEGEYSER));
    geysersToBuildOn.insert(geysersToBuildOn.begin(), FindSecondNearest(observer->GetStartLocation(), UNIT_TYPEID::NEUTRAL_VESPENEGEYSER));
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
    Units bases = observer->GetUnits(Unit::Alliance::Self, IsTownHall());

    SupplyDepotManager(7);
    if (bases.size() > 1 ){
        ContinuousSCVSpawn(5);
    } else{
        ContinuousSCVSpawn(2);
    }
    ManageOffensive();

    RefineryManager();
    CommandCenterManager();
}


void BobTheBot::OnUnitIdle(const Unit* unit) {
    switch (unit->unit_type.ToType()) {
        case UNIT_TYPEID::TERRAN_SCV: {
            MineMinerals(unit);
            break;
        }
        default: {
            break;
        }
    }
}

