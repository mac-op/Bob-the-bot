#include "BobTheBot.h"

void BobTheBot::ManageBarracks(int maxBarracks) {
    if (CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) < 1 ||
        CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS) > maxBarracks)
    { return; }
    TryBuildStructure(ABILITY_ID::BUILD_BARRACKS);
}