#include "BobTheBot.h"

void BobTheBot::ManageOffensiveStructures() {
    Units bases = observer->GetUnits(Unit::Alliance::Self, IsTownHall());
    Units barracks = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_BARRACKS));
    Units factories = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_FACTORY));
    Units depots = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_SUPPLYDEPOT));
    Units armory = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_ARMORY));
    Units engBay = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_ENGINEERINGBAY));

    if (depots.size() > 1 && barracks.size() <= bases.size() * 3) {
        if (observer->GetMinerals() > 150)
            if (TryBuildStructure(ABILITY_ID::BUILD_BARRACKS)) return;
    }

    // Build Reactor for barracks
    for (auto barrack : barracks) {
        if (observer->GetMinerals() >= 100)
            BuildAddOn(ABILITY_ID::BUILD_REACTOR_BARRACKS, barrack);
    }

    // Build Tech Lab for factories
    for (auto factory : factories) {
        if (observer->GetMinerals() >= 100)
            BuildAddOn(ABILITY_ID::BUILD_TECHLAB_FACTORY, factory);
    }

    uint32_t minerals = observer->GetMinerals();
    uint32_t gas = observer->GetVespene();
    // Build some advanced structures once barracks are built
    if (!barracks.empty()){
        if (armory.size() < 1 && !factories.empty() && minerals >= 200){
            if (TryBuildStructure(ABILITY_ID::BUILD_ARMORY)) return;
        }
        if (engBay.size() < 1 && minerals > 150 && gas > 100) {
            if (TryBuildStructure(ABILITY_ID::BUILD_ENGINEERINGBAY)) return;
        }
        if (factories.size() < bases.size() && minerals > 200 && gas > 100) {
            if (TryBuildStructure(ABILITY_ID::BUILD_FACTORY)) return;
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
    UpgradeArmy();
//    Scout();

    Units enemies = observer->GetUnits(Unit::Alliance::Enemy);
//    if (!enemies.empty()) {
//        Units marines = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_MARINE));
//        for (auto marine: marines) {
//            AttackWithUnit(marine);
//        }
//    }
    Units bases = observer->GetUnits(Unit::Alliance::Self, IsTownHall());
    Units barracks = observer->GetUnits(Unit::Alliance::Self, IsUnits({UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_BARRACKSREACTOR}));
    Units factories = observer->GetUnits(Unit::Alliance::Self, IsUnits({UNIT_TYPEID::TERRAN_FACTORY, UNIT_TYPEID::TERRAN_FACTORYTECHLAB}));
    Units armories = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_ARMORY));

    Units tanks = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_SIEGETANK));
    Units thors = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_THOR));
    Units hellions = observer->GetUnits(Unit::Alliance::Self, IsUnits({UNIT_TYPEID::TERRAN_HELLION, UNIT_TYPEID::TERRAN_HELLIONTANK}));
    Units marines = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_MARINE));
    Units reapers = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_REAPER));

    for (auto barrack : barracks) {
        if (observer->GetUnit(barrack->add_on_tag) == nullptr) {
            if (barrack->orders.empty() && marines.size() < 5 * barracks.size()) {
                actions->UnitCommand(barrack, ABILITY_ID::TRAIN_MARINE);
            }
        } else {                            // If Barracks Reactor
            if (barrack->orders.size() == 2) continue;
            if (marines.size() < 5 * barracks.size()) {
                actions->UnitCommand(barrack, ABILITY_ID::TRAIN_MARINE);
            }
            if (reapers.size() * 3 < marines.size()) {
                actions->UnitCommand(barrack, ABILITY_ID::TRAIN_REAPER);
            }
        }
    }

    for (auto factory : factories) {
        if (observer->GetUnit(factory->add_on_tag) == nullptr) {
            if (!factory->orders.empty() || hellions.size() >= 7 * bases.size()) continue;
            if (!armories.empty()) {
                actions->UnitCommand(factory, ABILITY_ID::TRAIN_HELLBAT);
            } else {
                actions->UnitCommand(factory, ABILITY_ID::TRAIN_HELLION);
            }
        } else {            // Factory with Tech Lab
            if (!factory->orders.empty()) continue;

            uint32_t minerals = observer->GetMinerals();
            uint32_t vespene = observer->GetVespene();
            if (!armories.empty() || thors.size() < bases.size()) {
                if (minerals >= 350 && vespene >= 250)
                    actions->UnitCommand(factory, ABILITY_ID::TRAIN_THOR);
            } else if (tanks.size() < bases.size() * 2) {
                if (minerals >= 150 && vespene >125)
                    actions->UnitCommand(factory, ABILITY_ID::TRAIN_SIEGETANK);
            } else {
                if (minerals >= 100 && hellions.size() >= 7 * bases.size())
                    actions->UnitCommand(factory, ABILITY_ID::TRAIN_HELLION);
            }
        }
    }

//    for (auto)
}

/*
 * Build addon for a given structure
 */
void BobTheBot::BuildAddOn(ABILITY_ID ability, const Unit *unit) {
    float rx = GetRandomScalar();
    float ry = GetRandomScalar();

    if (unit->build_progress != 1) return;

    Point2D location = Point2D(unit->pos.x + rx * 10, unit->pos.y + ry * 10);
    if (query->Placement(ability, location, unit)) {
        actions->UnitCommand(unit, ability, location);
    }
}

void BobTheBot::UpgradeArmy() {
    auto upgrades = observer->GetUpgrades();
    Units bases = observer->GetUnits(Unit::Alliance::Self, IsTownHall());
    Units armory = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_ARMORY));
    Units engBay = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_ENGINEERINGBAY));

    if (observer->GetMinerals() < bases.size() * 150 || observer->GetVespene() < bases.size() * 100) return;
    if (!engBay.empty() && !armory.empty()) {
        // Not very costly since upgrades is small
        if (std::find(upgrades.begin(), upgrades.end(), UPGRADE_ID::TERRANINFANTRYARMORSLEVEL3) == upgrades.end()) {
            if (bases.size() >= upgrades.size() / 2) {
                actions->UnitCommand(engBay, ABILITY_ID::RESEARCH_TERRANINFANTRYARMOR);
            }
        }
        if (std::find(upgrades.begin(), upgrades.end(), UPGRADE_ID::TERRANINFANTRYWEAPONSLEVEL3) == upgrades.end()) {
            if (bases.size() >= upgrades.size() / 2) {
                actions->UnitCommand(engBay, ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONS);
            }
        }
    }

    if (!armory.empty()) {
        if (std::find(upgrades.begin(), upgrades.end(), UPGRADE_ID::TERRANVEHICLEARMORSLEVEL3) == upgrades.end()) {
            if (bases.size() >= upgrades.size() / 2) {
                actions->UnitCommand(armory, ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATING);
            }
        }
        if (std::find(upgrades.begin(), upgrades.end(), UPGRADE_ID::TERRANINFANTRYWEAPONSLEVEL3) == upgrades.end()) {
            if (bases.size() >= upgrades.size() / 2) {
                actions->UnitCommand(armory, ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONS);
            }
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