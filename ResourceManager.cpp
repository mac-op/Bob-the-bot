#include "BobTheBot.h"
#include <iostream>
#include <algorithm>
#include <random>

bool depotBuilding = false; // Helps keep track of a depot thats being built, so we dont accidentally build multiple at a time
void BobTheBot::SupplyDepotManager(int sensitivity) {
    // Sensitivity is how close we need to get to the supply cap before building a new supply depot
    size_t mineralCount = observer->GetMinerals();
    int supplyLeft = observer->GetFoodCap() - observer->GetFoodUsed();

    bool shouldBuildDepot = !depotBuilding && (mineralCount >= 100) && (supplyLeft <= sensitivity);
    if (shouldBuildDepot) {
        if (TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT, latestCommCen->pos)) {
            depotBuilding = true;
        }
    }
}

bool commandCenterBuilding = false;
void BobTheBot::CommandCenterManager() {
    if (observer->GetMinerals() > 500 && expansionLocations.size() > 0 && !commandCenterBuilding) {
        //commandCenterBuilding = true;
        Point3D closestLocation = expansionLocations.back();
        if (closestLocation.x == 0 && closestLocation.y == 0) {
            expansionLocations.pop_back();
            return;
        }
        if (TryBuildStructure(ABILITY_ID::BUILD_COMMANDCENTER, closestLocation)) {
            expansionLocations.pop_back();
        }
    }

    const Units commandCenters = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_COMMANDCENTER));
    for (auto commandCenter : commandCenters) {
        if (observer->GetMinerals() > 150) {
            Actions()->UnitCommand(commandCenter, ABILITY_ID::MORPH_ORBITALCOMMAND);
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
    const Units commandCenters = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_COMMANDCENTER));
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
    // Build new refineries, if possible
    bool enoughMinerals = observer->GetMinerals() > 75;
    bool enoughSpace = observer->GetFoodCap() > observer->GetFoodUsed();
    if (geysersToBuildOn.size() > 0 && enoughMinerals && enoughSpace) {
        TryBuildStructure(ABILITY_ID::BUILD_REFINERY, Point2D(0, 0), geysersToBuildOn.back());
        geysersToBuildOn.pop_back();
    }

    const Units refineries = observer->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_REFINERY));
    for (auto refinery : refineries) {
        if (refinery->assigned_harvesters < refinery->ideal_harvesters) {
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
    search::ExpansionParameters ep;
    DebugInterface* d = Debug();
    ep.debug_ = d;
    expansionLocations = search::CalculateExpansionLocations(observer, query, ep);
    d->SendDebug();
    
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
        approxLocation.x = location.x + rx * 9.0f;
        approxLocation.y = location.y + ry * 9.0f;
        ++attempts;
    }

    // No valid location found
    if (!query->Placement(ability_type_for_structure, approxLocation)) {
        return Point2D(-1, -1);
    }

    return approxLocation;
}

bool BobTheBot::TryBuildStructure(ABILITY_ID ability_type_for_structure, Point2D location, const Unit* unit_to_build_on)
{
    const Unit* unit_to_build = getAvailableSCV();
    if (!unit_to_build) {
        return false;
    }

    if (location.x == 0 && location.y == 0) {
        location.x = unit_to_build->pos.x;
        location.y = unit_to_build->pos.y;
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
        commandCenterBuilding = false;
        break;
    }

    case UNIT_TYPEID::TERRAN_REFINERY: {
        const Unit* scv1 = getAvailableSCV();
        const Unit* scv2 = getAvailableSCV();
        actions->UnitCommand(scv1, ABILITY_ID::SMART, unit);
        actions->UnitCommand(scv2, ABILITY_ID::SMART, unit);
        break;
    }

    default:
        break;
    }
}


void BobTheBot::OnUnitDestroyed(const Unit* unit) {
    switch (unit->unit_type.ToType()) {
    
    case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
        expansionLocations.insert(expansionLocations.begin(), unit->pos);
    }

    case UNIT_TYPEID::TERRAN_REFINERY: {
        geysersToBuildOn.insert(geysersToBuildOn.begin(), FindNearest(unit->pos, UNIT_TYPEID::NEUTRAL_VESPENEGEYSER));
    }

    default:
        break;
    }
}


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