#include "BobTheBot.h"
#include <iostream>

bool depotBuilding = false; // Helps keep track of a depot thats being built, so we dont accidentally build multiple at a time
void BobTheBot::SupplyDepotManager(int sensitivity) {
    // Sensitivity is how close we need to get to the supply cap before building a new supply depot

    size_t mineralCount = observer->GetMinerals();
    int supplyLeft = observer->GetFoodCap() - observer->GetFoodUsed();

    bool shouldBuildDepot = !depotBuilding && (mineralCount >= 100) && (supplyLeft <= sensitivity);
    if (shouldBuildDepot) {
        if (TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT, initialCommCen->pos)) {
            depotBuilding = true;
        }
    }
}

bool commandCenterBuilding = false;
void BobTheBot::CommandCenterManager() {
    if (observer->GetMinerals() > 1150 && expansionLocations.size() > 0 && !commandCenterBuilding) {
        commandCenterBuilding = true;
        Point3D closestLocation = expansionLocations.back();
        expansionLocations.pop_back();
        if (closestLocation.x == 0 && closestLocation.y == 0)
            return;
        TryBuildStructure(ABILITY_ID::BUILD_COMMANDCENTER, closestLocation);
    }
}


bool BobTheBot::MineMinerals(const Unit* scv) {
    const Unit* mineral_target = FindNearest(scv->pos, isMineralField);
    if (!mineral_target) {
        return false;
    }
    actions->UnitCommand(scv, ABILITY_ID::SMART, mineral_target);
    return true;
}


void BobTheBot::ContinuousSCVSpawn(int leeway) {
    // leeway is how much space we should reserve for other units when we are nearing the supply limit
    const Units commandCenters = observer->GetUnits(Unit::Alliance::Self, isCommandCenter);
    for (const Unit* commandCenter : commandCenters)
    {
        bool enoughMinerals = observer->GetMinerals() > 50;
        bool enoughSpace = (observer->GetFoodCap() - observer->GetFoodUsed()) > leeway;
        if (enoughMinerals && enoughSpace) {
            actions->UnitCommand(commandCenter, ABILITY_ID::TRAIN_SCV);
        }
    }
}


void BobTheBot::RefineryManager() {
    bool enoughMinerals = observer->GetMinerals() > 75;
    bool enoughSpace = observer->GetFoodCap() > observer->GetFoodUsed();
    if (geysersToBuildOn.size() > 0 && enoughMinerals && enoughSpace) {
        TryBuildStructure(ABILITY_ID::BUILD_REFINERY, Point2D(0, 0), geysersToBuildOn.back());
        geysersToBuildOn.pop_back();
    }
}



// Comparison function for sorting based on distance to a given point
bool BobTheBot::compareDistance(const Point3D& p1, const Point3D& p2, const Point3D& referencePoint) {
    return DistanceSquared2D(p1, referencePoint) > DistanceSquared2D(p2, referencePoint);
}


std::vector<Point3D> BobTheBot::getExpansionLocations() {
    search::ExpansionParameters ep;
    DebugInterface* d = Debug();
    ep.debug_ = d;
    std::vector<Point3D> locations = search::CalculateExpansionLocations(observer, query, ep);
    d->SendDebug();
    initialCommCen = observer->GetUnits(Unit::Alliance::Self, isCommandCenter)[0];
    Point3D initialCommCenLocation(initialCommCen->pos.x, initialCommCen->pos.y, initialCommCen->pos.z);

    // Sorting the vector based on distance to the initial command center
    sort(locations.begin(), locations.end(), [&initialCommCenLocation, this](const Point3D& p1, const Point3D& p2) {
        return compareDistance(p1, p2, initialCommCenLocation);
        });

    return locations;
}


void BobTheBot::OnGameStart() {
    actions->SendChat("\nBob the Bot\nCan we fix it ?\nBob the Bot\nYes, we can!!");

    // Get all possible locations we can expand to
    expansionLocations = getExpansionLocations();

    // Add the geysers near the initial command senter to the queue of geysers to build a refinery on
    geysersToBuildOn.insert(geysersToBuildOn.begin(), FindNearest(initialCommCen->pos, isGeyser));
    geysersToBuildOn.insert(geysersToBuildOn.begin(), FindSecondNearest(initialCommCen->pos, isGeyser));
}


void BobTheBot::OnStep() {
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

    return approxLocation;
}

bool BobTheBot::TryBuildStructure(ABILITY_ID ability_type_for_structure, Point2D location, const Unit* unit_to_build_on)
{
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

        // Find the closest valid position to build on
        Point2D approxLocation = getValidNearbyLocation(location, ability_type_for_structure);
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
        geysersToBuildOn.insert(geysersToBuildOn.begin(), FindNearest(unit->pos, isGeyser));
        geysersToBuildOn.insert(geysersToBuildOn.begin(), FindSecondNearest(unit->pos, isGeyser));
        commandCenterBuilding = false;
        break;
    }

    default:
        break;
    }
}


const Unit* BobTheBot::FindNearest(const Point2D& start, bool (*filterType)(const Unit&)) {
    Units units = observer->GetUnits(Unit::Alliance::Neutral, filterType);
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


const Unit* BobTheBot::FindSecondNearest(const Point2D& start, bool (*filterType)(const Unit&)) {
    Units units = observer->GetUnits(Unit::Alliance::Neutral, filterType);
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
    Units allSCVs = observer->GetUnits(Unit::Alliance::Self, isSCV);
    for (const Unit* scv : allSCVs) {
        bool isMining = (scv->orders).size() == 1 && (scv->orders[0].ability_id == ABILITY_ID::HARVEST_GATHER || scv->orders[0].ability_id == ABILITY_ID::HARVEST_RETURN);
        if (isMining) {    // If the scv is either doing nothing or mining
            return scv;
        }
    }
    return nullptr;
}