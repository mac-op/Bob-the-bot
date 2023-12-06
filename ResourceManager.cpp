#include "BobTheBot.h"
#include <algorithm>
#include <random>

bool depotBuilding = false; // Helps keep track of a depot thats being built, so we dont accidentally build multiple at a time
void BobTheBot::SupplyDepotManager(int sensitivity) {
    // Sensitivity is how close we need to get to the supply cap before building a new supply depot
    size_t mineralCount = observer->GetMinerals();
    int supplyLeft = observer->GetFoodCap() - observer->GetFoodUsed();

    bool shouldBuildDepot = !depotBuilding && (mineralCount >= 100) && (supplyLeft <= sensitivity);
    if (shouldBuildDepot) {
        if (TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT)) {
            depotBuilding = true;
        }
    }
}


void BobTheBot::CommandCenterManager() {
    // If we have enough minerals, and we aren't out of locations to expand to
    if (observer->GetMinerals() > 400 && expansionLocations.size() > 0) {
        Point3D closestLocation = expansionLocations.back();
        
        // Expansion location (0, 0) is invalid, we ignore it if it appears
        if (closestLocation.x == 0 && closestLocation.y == 0) {
            expansionLocations.pop_back();
            return;
        }
        
        if (TryBuildStructure(ABILITY_ID::BUILD_COMMANDCENTER)) {
            expansionLocations.pop_back();
        }
    }

    // Upgrade any command center to orbital command as soon as we have enough mineral
    const Units commandCenters = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_COMMANDCENTER));
    for (auto commandCenter : commandCenters) {
        if (observer->GetMinerals() > 150) {
            actions->UnitCommand(commandCenter, ABILITY_ID::MORPH_ORBITALCOMMAND);
        }
    }
    
    // Spawn a MULE as often as possible at all orbital commands
    const Units orbitalCommands = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_ORBITALCOMMAND));
    for (auto orbitalCommand : orbitalCommands) {
        if (orbitalCommand->energy > 50) {
            const Unit* mineral_target = FindNearest(orbitalCommand->pos, UNIT_TYPEID::NEUTRAL_MINERALFIELD);
            actions->UnitCommand(orbitalCommand, ABILITY_ID::EFFECT_CALLDOWNMULE, mineral_target);
        }
    }
}


bool BobTheBot::MineMinerals(const Unit* scv) {
    const Unit* mineral_target = FindNearest(scv->pos, UNIT_TYPEID::NEUTRAL_MINERALFIELD);
    if (!mineral_target) {
        return false;
    }
    actions->UnitCommand(scv, ABILITY_ID::SMART, mineral_target);
    return true;
}


void BobTheBot::ContinuousSCVSpawn(int leeway) {
    // leeway is how much space we should reserve for other units when we are nearing the supply limit
    const Units commandCenters = observer->GetUnits(Unit::Alliance::Self, IsTownHall());

    for (const Unit* commandCenter : commandCenters)
    {
        bool enoughMinerals = observer->GetMinerals() > 50;
        bool enoughSpace = ((observer->GetFoodCap() - observer->GetFoodUsed()) > leeway) && (commandCenter->assigned_harvesters < commandCenter->ideal_harvesters);
        bool overTraining = (commandCenter->orders).size() > 1; // We only need to queue 1 scv at a time. It doesn't benefit us to train more than 1 and we get to keep more minerals at hand.
        if (enoughMinerals && enoughSpace && !overTraining) {
            actions->UnitCommand(commandCenter, ABILITY_ID::TRAIN_SCV);
        }
    }
}


void BobTheBot::RefineryManager() {
    // Build new refineries, if possible, and if there is any to be built
    bool enoughMinerals = observer->GetMinerals() > 75;
    bool enoughSpace = observer->GetFoodCap() > observer->GetFoodUsed();
    if (geysersToBuildOn.size() > 0 && enoughMinerals && enoughSpace) {
        TryBuildStructure(ABILITY_ID::BUILD_REFINERY, geysersToBuildOn.back());
        geysersToBuildOn.pop_back();
    }

    // Keep assigning SCVs to the refinery, until we reach the reduced limit of ideal_harvesters - 2 that we set
    const Units refineries = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_REFINERY));
    for (auto refinery : refineries) {
        if (refinery->assigned_harvesters < refinery->ideal_harvesters - 2) {   // Our strategy doesn't need as much gas, so we place a limit
            const Unit* scv = getAvailableSCV();
            actions->UnitCommand(scv, ABILITY_ID::SMART, refinery);
        }
    }
}


// Comparison function for sorting based on distance to a given point
bool BobTheBot::compareDistance(const Point3D& p1, const Point3D& p2, const Point3D& referencePoint) {
    return DistanceSquared2D(p1, referencePoint) > DistanceSquared2D(p2, referencePoint);
}


void BobTheBot::getExpansionLocations() {
    expansionLocations = search::CalculateExpansionLocations(observer, query);
    
    Point3D refPoint = observer->GetStartLocation();
    // Sorting the vector based on distance to the initial command center
    sort(expansionLocations.begin(), expansionLocations.end(),
         [&refPoint, this](const Point3D& p1, const Point3D& p2) {
                    return compareDistance(p1, p2, refPoint);
               }
    );
}


void BobTheBot::sendSCVScout(const Unit* SCV, Point2D location) {
    actions->UnitCommand(SCV, ABILITY_ID::GENERAL_MOVE, location);
}


Point2D BobTheBot::getValidNearbyLocation(Point2D location, ABILITY_ID ability_type_for_structure) {
    // Keep finding random positions until you get a valid one for placement
    float rx, ry;
    Point2D approxLocation(location);
    int attempts = 0;
    while (!query->Placement(ability_type_for_structure, approxLocation) && attempts < 100)
    {
        rx = GetRandomScalar();
        ry = GetRandomScalar();
        approxLocation.x = location.x + rx * 20.0f;
        approxLocation.y = location.y + ry * 20.0f;
        ++attempts;
    }

    // No valid location found
    if (!query->Placement(ability_type_for_structure, approxLocation)) {
        return Point2D(-1, -1);
    }

    return approxLocation;
}


// Referenced from: https://github.com/Blizzard/s2client-api/blob/master/examples/common/bot_examples.cc
bool BobTheBot::TryBuildStructure(ABILITY_ID ability_type_for_structure, const Unit* unit_to_build_on)
{
    Point2D location;
    // If we are building a new command center, grab the closest location from the expansionLocations list
    if (ability_type_for_structure == ABILITY_ID::BUILD_COMMANDCENTER) {
        location = expansionLocations.back();
    }
    // else build at the latest commanc center
    else {
        location = latestCommCen->pos;
    }
   
    const Unit* unit_to_build = getAvailableSCV();
    if (!unit_to_build) {
        return false;
    }

    // Edge case for refineries, you need to provide the nearest geyser
    if (ability_type_for_structure == ABILITY_ID::BUILD_REFINERY) {
        actions->UnitCommand(unit_to_build, ability_type_for_structure, unit_to_build_on);
    }

    else {
        // If we are building a new command center, we need to scout the location we want to place it at first
        if (ability_type_for_structure == ABILITY_ID::BUILD_COMMANDCENTER) {
            sendSCVScout(unit_to_build, location);
        }

        // Find the closest valid position to build on (returns (-1, -1) if none found)
        Point2D approxLocation = getValidNearbyLocation(location, ability_type_for_structure);
        if (approxLocation.x == -1) {
            return false;
        }
        actions->UnitCommand(unit_to_build, ability_type_for_structure, approxLocation);
    }

    return true;
}


void BobTheBot::OnBuildingConstructionComplete(const Unit* unit)
{
    switch (unit->unit_type.ToType()) {

    // Immediately start building more SCVS when he have space
    case UNIT_TYPEID::TERRAN_SUPPLYDEPOT: {
        depotBuilding = false;
        break;
    }

    // Immediately start building more SCVS when he have space
    case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
        geysersToBuildOn.insert(geysersToBuildOn.begin(), FindNearest(unit->pos, UNIT_TYPEID::NEUTRAL_VESPENEGEYSER));
        geysersToBuildOn.insert(geysersToBuildOn.begin(), FindSecondNearest(unit->pos, UNIT_TYPEID::NEUTRAL_VESPENEGEYSER));
        latestCommCen = unit;
        break;
    }
    

    //case UNIT_TYPEID::TERRAN_REFINERY: {
    //    const Unit* scv1 = getAvailableSCV();
    //    const Unit* scv2 = getAvailableSCV();
    //    actions->UnitCommand(scv1, ABILITY_ID::SMART, unit);
    //    actions->UnitCommand(scv2, ABILITY_ID::SMART, unit);
    //    break;
    //}

    case UNIT_TYPEID::TERRAN_BARRACKS: {
        Units barracks = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_BARRACKS));
        if (barracks.size() == 1) ScoutDelay.baseScoutEnabled = Observation()->GetGameLoop() * 2;
    }
    default:
        break;
    }
}


void BobTheBot::OnUnitDestroyed(const Unit* unit) {
    switch (unit->unit_type.ToType()) {
    
    // If a command center is destroyed, add it back to the list of expansion location, and send some troops to the destroyed location
    case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
        expansionLocations.insert(expansionLocations.begin(), unit->pos);
        Units army = observer->GetUnits(Unit::Alliance::Self, IsUnits(armyTypes));
        for (auto fighter : army) {
            actions->UnitCommand(fighter, ABILITY_ID::SMART, unit->pos);
        }
        break;
        expansionLocations.push_back(unit->pos);
    }
    
    // We can now build on that geyser again
    case UNIT_TYPEID::TERRAN_REFINERY: {
        geysersToBuildOn.insert(geysersToBuildOn.begin(), FindNearest(unit->pos, UNIT_TYPEID::NEUTRAL_VESPENEGEYSER));
        break;
    }

    default:
        break;
    }
}

// Referenced from: https://github.com/Blizzard/s2client-api/blob/master/examples/common/bot_examples.cc
const Unit* BobTheBot::FindNearest(const Point2D& start, UNIT_TYPEID unitType) {
    Units units = observer->GetUnits(Unit::Alliance::Neutral, IsUnit(unitType));
    float distance = std::numeric_limits<float>::max();
    const Unit* target = nullptr;
    for (const auto& u : units) {
        float d = DistanceSquared2D(start, u->pos);
        if (d < distance) {
            distance = d;
            target = u;
        }
    }
    return target;
}


const Unit* BobTheBot::FindSecondNearest(const Point2D& start, UNIT_TYPEID unitType) {
    Units units = observer->GetUnits(Unit::Alliance::Neutral, IsUnit(unitType));
    float nearestDistance = std::numeric_limits<float>::max();
    float secondNearestDistance = std::numeric_limits<float>::max();
    const Unit* nearestTarget = nullptr;
    const Unit* secondNearestTarget = nullptr;

    for (const auto& u : units) {
        float d = DistanceSquared2D(start, u->pos);

        if (d < nearestDistance) {
            secondNearestDistance = nearestDistance;
            secondNearestTarget = nearestTarget;

            nearestDistance = d;
            nearestTarget = u;
        }
        else if (d < secondNearestDistance) {
            secondNearestDistance = d;
            secondNearestTarget = u;
        }
    }

    return secondNearestTarget;
}


const Unit* BobTheBot::getAvailableSCV() {
    Units allSCVs = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_SCV));
    // Use a random seed for shuffling
    std::random_device rd;
    std::mt19937 g(rd());
    // Shuffle the SCVs randomly
    std::shuffle(allSCVs.begin(), allSCVs.end(), g);

    for (const Unit* scv : allSCVs) {
        bool isMining = (scv->orders).size() == 1 && (scv->orders[0].ability_id == ABILITY_ID::HARVEST_GATHER || scv->orders[0].ability_id == ABILITY_ID::HARVEST_RETURN);
        if (isMining) {    // If the scv is either doing nothing or mining
            return scv;
        }
    }
    return nullptr;
}