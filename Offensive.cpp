#include "BobTheBot.h"
#include <random>

void BobTheBot::ManageOffensiveStructures() {
    Units bases = observer->GetUnits(Unit::Alliance::Self, IsTownHall());
    Units barracks = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_BARRACKS));
    Units factories = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_FACTORY));
    Units depots = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_SUPPLYDEPOT));
    Units armory = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_ARMORY));
    Units engBay = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_ENGINEERINGBAY));

    if (depots.size() > 1 && barracks.size() < bases.size() * 2) {
        if (observer->GetMinerals() > 150) {
            if (TryBuildStructure(ABILITY_ID::BUILD_BARRACKS)) return;
        }
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
        if (factories.size() < bases.size() && minerals > 150 && gas > 100) {
            if (TryBuildStructure(ABILITY_ID::BUILD_FACTORY)) return;
        }
        if (armory.size() < 1 && !factories.empty() && minerals >= 200){
            if (TryBuildStructure(ABILITY_ID::BUILD_ARMORY)) return;
        }
        if (engBay.size() < 1 && minerals > 150 && gas > 100) {
            if (TryBuildStructure(ABILITY_ID::BUILD_ENGINEERINGBAY)) return;
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
    if (gameInfo.enemy_start_locations.empty() || expansionLocations.size() <= 8) {
        return false;
    }
    if (observer->GetGameLoop() % 3) {
        target_pos = GetRandomEntry(expansionLocations);
    } else {
        target_pos = GetRandomEntry(gameInfo.enemy_start_locations);
    }
    return true;
}

bool BobTheBot::FindRandomCorner(const Unit* unit, Point2D& target_pos) {
    // First, find a random point inside the playable area of the map.
    float playable_w = gameInfo.playable_max.x - gameInfo.playable_min.x;
    float playable_h = gameInfo.playable_max.y - gameInfo.playable_min.y;

    // The case where game_info_ does not provide a valid answer
    if (playable_w == 0 || playable_h == 0) {
        playable_w = 236;
        playable_h = 228;
    }
    std::vector<Point2D> corners {{0.05, 0.05}, {0.05, 0.95}, {0.95, 0.05}, {0.95, 0.95}};
    Point2D corner = GetRandomEntry(corners);
    std::uniform_real_distribution<double> dist_x(corner.x-0.05, corner.x +0.05);
    std::uniform_real_distribution<double> dist_y(corner.y-0.05, corner.y +0.05);
    std::default_random_engine generator;

    target_pos.x = playable_w * dist_x(generator) + gameInfo.playable_min.x;
    target_pos.y = playable_h * dist_y(generator) + gameInfo.playable_min.y;

    float distance = Query()->PathingDistance(unit, target_pos);

    return distance > 0.1f;
}

void BobTheBot::AttackWithUnit(const Unit* unit) {
    //If unit isn't doing anything make it attack.
    Units enemy_units = Observation()->GetUnits(Unit::Alliance::Enemy);
    if (enemy_units.empty()) {
        return;
    }
    Point2D originalLocation = unit->pos;

    if (unit->orders.empty()) {
        actions->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front()->pos);
//        actions->UnitCommand(unit, ABILITY_ID::SMART, originalLocation, true);
        return;
    }

    //If the unit is doing something besides attacking, make it attack.
    if (unit->orders.front().ability_id != ABILITY_ID::ATTACK) {
        actions->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front()->pos);
//        actions->UnitCommand(unit, ABILITY_ID::SMART, originalLocation, true);
    }
}

void BobTheBot::ManageOffensive() {
    Units bases = observer->GetUnits(Unit::Alliance::Self, IsTownHall());
    if (observer->GetFoodArmy() < bases.size() * 30) {
        BuildArmy();
    }
    ManageOffensiveStructures();

    UpgradeArmy();
    Attack();
}

void BobTheBot::BuildArmy() {
    Units bases = observer->GetUnits(Unit::Self, IsTownHall());
    Units barracks = observer->GetUnits(Unit::Self, IsUnits({UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_BARRACKSREACTOR}));
    Units factories = observer->GetUnits(Unit::Self, IsUnits({UNIT_TYPEID::TERRAN_FACTORY, UNIT_TYPEID::TERRAN_FACTORYTECHLAB}));
    Units armories = observer->GetUnits(Unit::Self, IsUnit(UNIT_TYPEID::TERRAN_ARMORY));

    Units tanks = observer->GetUnits(Unit::Self, IsUnit(UNIT_TYPEID::TERRAN_SIEGETANK));
    Units thors = observer->GetUnits(Unit::Self, IsUnit(UNIT_TYPEID::TERRAN_THOR));
    Units hellions = observer->GetUnits(Unit::Self, IsUnits({UNIT_TYPEID::TERRAN_HELLION, UNIT_TYPEID::TERRAN_HELLIONTANK}));
    Units marines = observer->GetUnits(Unit::Self, IsUnit(UNIT_TYPEID::TERRAN_MARINE));
    Units reapers = observer->GetUnits(Unit::Self, IsUnit(UNIT_TYPEID::TERRAN_REAPER));

    uint32_t minerals = observer->GetMinerals();
    uint32_t vespene = observer->GetVespene();

    for (auto factory : factories) {
        if (Observation()->GetUnit(factory->add_on_tag) == nullptr) {
            if (!factory->orders.empty() || hellions.size() >= 3 * factories.size() || minerals < 100) continue;
            if (!armories.empty()) {
                actions->UnitCommand(factory, ABILITY_ID::TRAIN_HELLBAT);
            } else {
                actions->UnitCommand(factory, ABILITY_ID::TRAIN_HELLION);
            }
        } else {            // Factory with Tech Lab
            if (!factory->orders.empty()) continue;

            if (tanks.size() < 6) {
                actions->UnitCommand(factory, ABILITY_ID::TRAIN_SIEGETANK);
            }
            if (thors.size() < 5) {
                actions->UnitCommand(factory, ABILITY_ID::TRAIN_THOR);
            }
            if (!armories.empty() || hellions.size() < 3 * factories.size()) {
                if (minerals >= 100)
                    actions->UnitCommand(factory, ABILITY_ID::TRAIN_HELLION);
            }
        }
    }

    for (auto barrack : barracks) {
        if (observer->GetUnit(barrack->add_on_tag) == nullptr) {
            if (barrack->orders.empty() && marines.size() < 3 * barracks.size()) {
                actions->UnitCommand(barrack, ABILITY_ID::TRAIN_MARINE);
            }
        } else {                            // If Barracks Reactor
            if (barrack->orders.size() == 2) continue;
            if (marines.size() < 3 * barracks.size()) {
                actions->UnitCommand(barrack, ABILITY_ID::TRAIN_MARINE);
            }
            if (reapers.size() * 2 < marines.size()) {
                actions->UnitCommand(barrack, ABILITY_ID::TRAIN_REAPER);
            }
        }
    }
}

void BobTheBot::Attack() {
    Units enemy = observer->GetUnits(Unit::Alliance::Enemy);
    if (enemy.empty()) {
        Scout();
        return;
    }

    Units army = observer->GetUnits(Unit::Alliance::Self, IsUnits(armyTypes));
    if (army.size() < 7) return;
    for (auto unit : army) {
        AttackWithUnit(unit);
    }
}

/*
 * Build addon for a given structure
 */
void BobTheBot::BuildAddOn(ABILITY_ID ability, const Unit *unit) {
    if (unit->build_progress != 1) return;

    if (query->Placement(ability, unit->pos, unit)) {
        actions->UnitCommand(unit, ability, unit->pos);
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
                actions->UnitCommand(engBay, ABILITY_ID::RESEARCH_TERRANINFANTRYARMOR, true);
            }
        }
        if (std::find(upgrades.begin(), upgrades.end(), UPGRADE_ID::TERRANINFANTRYWEAPONSLEVEL3) == upgrades.end()) {
            if (bases.size() >= upgrades.size() / 2) {
                actions->UnitCommand(engBay, ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONS, true);
            }
        }
    }

    if (!armory.empty() && !upgrades.empty()) {
        if (std::find(upgrades.begin(), upgrades.end(), UPGRADE_ID::TERRANVEHICLEARMORSLEVEL3) == upgrades.end()) {
            if (bases.size() >= upgrades.size() / 2) {
                actions->UnitCommand(armory, ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATING, true);
            }
        }
        if (std::find(upgrades.begin(), upgrades.end(), UPGRADE_ID::TERRANINFANTRYWEAPONSLEVEL3) == upgrades.end()) {
            if (bases.size() >= upgrades.size() / 2) {
                actions->UnitCommand(armory, ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONS, true);
            }
        }
    }
}

void BobTheBot::Scout() {
    Units enemy_units = observer->GetUnits(Unit::Alliance::Enemy);
    const Unit* unit = GetRandomUnit(UNIT_TYPEID::TERRAN_MARINE);
    if (!unit) return;

    Point2D originalLocation = unit->pos;
    if (!unit->orders.empty()) {
        return;
    }
    Point2D target_pos;

    if (Observation()->GetGameLoop() >= ScoutDelay.baseScoutEnabled) {
        if (FindRandomCorner(unit, target_pos)) {
            actions->UnitCommand(unit, ABILITY_ID::SMART, target_pos);
        }
        ScoutDelay.baseScoutEnabled *= 2;
        return;
    }
    uint32_t currentGameLoop = Observation()->GetGameLoop();
    if ( currentGameLoop < ScoutDelay.gameLoop) return;

    ScoutDelay.gameLoop = currentGameLoop + ScoutDelay.delay;

    if (FindEnemyPosition(target_pos)) {
        Actions()->UnitCommand(unit, ABILITY_ID::SMART, target_pos);
    }
    else {
        for (int i = 0; i < 2; i++) {
            if (FindRandomCorner(unit, target_pos)) {
                actions->UnitCommand(unit, ABILITY_ID::SMART, target_pos);
            }
        }
        actions->UnitCommand(unit, ABILITY_ID::SMART, originalLocation, true);
    }
}