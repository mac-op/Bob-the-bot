#include "BobTheBot.h"

void BobTheBot::ManageOffensiveStructures() {
    Units bases = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_COMMANDCENTER));
    Units barracks = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_BARRACKS));


    if (CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) > 1 && barracks.size() <= bases.size() * 3) {
        TryBuildStructure(ABILITY_ID::BUILD_BARRACKS);
    }

    // Build some advanced structures once barracks are built
    if (!barracks.empty()){
        if (CountUnitType(UNIT_TYPEID::TERRAN_ARMORY) < 1){
            TryBuildStructure(ABILITY_ID::BUILD_ARMORY);
        }
        if (CountUnitType(UNIT_TYPEID::TERRAN_GHOSTACADEMY) < 1 &&
            observer->GetMinerals() > 150 && observer->GetVespene() > 100) {
            TryBuildStructure(ABILITY_ID::BUILD_GHOSTACADEMY);
        }
        if (CountUnitType(UNIT_TYPEID::TERRAN_ENGINEERINGBAY) < 1 &&
            observer->GetMinerals() > 150 && observer->GetVespene() > 100) {
            TryBuildStructure(ABILITY_ID::BUILD_ENGINEERINGBAY);
        }
        if (CountUnitType(UNIT_TYPEID::TERRAN_FACTORY) < bases.size() * 2 &&
            observer->GetMinerals() > 150 && observer->GetVespene() > 100) {
            TryBuildStructure(ABILITY_ID::BUILD_ENGINEERINGBAY);
        }
    }
}

const Unit* BobTheBot::GetRandomUnit(UnitTypeID unit_type) {
    Units my_units = observer->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
    const Unit* result = nullptr;
    if (!my_units.empty()) {
        result = GetRandomEntry(my_units);
    }
    return result;
}

bool BobTheBot::FindEnemyPosition(Point2D& target_pos) {
    if (gameInfo.enemy_start_locations.empty()) {
        return false;
    }
    target_pos = GetRandomEntry(gameInfo.enemy_start_locations);
    return true;
}
bool BobTheBot::FindRandomLocation(const Unit* unit, Point2D& target_pos) {
    // First, find a random point inside the playable area of the map.
    float playable_w = gameInfo.playable_max.x - gameInfo.playable_min.x;
    float playable_h = gameInfo.playable_max.y - gameInfo.playable_min.y;

    // The case where game_info_ does not provide a valid answer
    if (playable_w == 0 || playable_h == 0) {
        playable_w = 236;
        playable_h = 228;
    }

    target_pos.x = playable_w * GetRandomFraction() + gameInfo.playable_min.x;
    target_pos.y = playable_h * GetRandomFraction() + gameInfo.playable_min.y;

    // Now send a pathing query from the unit to that point. Can also query from point to point,
    // but using a unit tag wherever possible will be more accurate.
    // Note: This query must communicate with the game to get a result which affects performance.
    // Ideally batch up the queries (using PathingDistanceBatched) and do many at once.
    float distance = Query()->PathingDistance(unit, target_pos);

    return distance > 0.1f;
}

void BobTheBot::AttackWithUnit(const Unit* unit) {
    //If unit isn't doing anything make it attack.
    Units enemy_units = observer->GetUnits(Unit::Alliance::Enemy);
    if (enemy_units.empty()) {
        return;
    }

    if (unit->orders.empty()) {
        Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front()->pos);
        return;
    }

    //If the unit is doing something besides attacking, make it attack.
    if (unit->orders.front().ability_id != ABILITY_ID::ATTACK) {
        Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front()->pos);
    }
}

void BobTheBot::ManageOffensive() {
    ManageOffensiveStructures();
    Scout();

    Units enemies = observer->GetUnits(Unit::Alliance::Enemy);
    if (!enemies.empty()) {
        Units marines = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_MARINE));
        for (auto marine: marines) {
            AttackWithUnit(marine);
        }
    }
}

void BobTheBot::Scout() {
    Units enemy_units = observer->GetUnits(Unit::Alliance::Enemy);
    const Unit* unit = GetRandomUnit(UNIT_TYPEID::TERRAN_MARINE);
    if (!unit) return;
    if (!unit->orders.empty()) {
        return;
    }
    Point2D target_pos;

    if (FindEnemyPosition(target_pos)) {
        if (Distance2D(unit->pos, target_pos) < 20 && enemy_units.empty()) {
            if (FindRandomLocation(unit, target_pos)) {
                Actions()->UnitCommand(unit, ABILITY_ID::SMART, target_pos);
                return;
            }
        }
        else if (!enemy_units.empty())
        {
            Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front());
            return;
        }
        Actions()->UnitCommand(unit, ABILITY_ID::SMART, target_pos);
    }
    else {
        if (FindRandomLocation(unit, target_pos)) {
            Actions()->UnitCommand(unit, ABILITY_ID::SMART, target_pos);
        }
    }
}