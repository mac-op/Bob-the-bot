#include "BobTheBot.h"

bool depotBuilding = false;

const ObservationInterface* observer;
ActionInterface* actions;

bool isCommandCenter(const Unit& unit)
{
    return unit.unit_type == UNIT_TYPEID::TERRAN_COMMANDCENTER;
}


bool isSCV(const Unit& unit)
{
    return unit.unit_type == UNIT_TYPEID::TERRAN_SCV;
}


void BobTheBot::SupplyDepotManager(int sensitivity) {
    const ObservationInterface* observation = Observation();

    size_t mineralCount = observation->GetMinerals();
    int supplyLeft = observation->GetFoodCap() - observation->GetFoodUsed();

    bool shouldBuildDepot = !depotBuilding && (mineralCount >= 100) && (supplyLeft <= sensitivity);
    if (shouldBuildDepot) {
        if (TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT)) {
            numDepots += 1;
            depotBuilding = true;
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


void ContinuousSCVSpawn(int leeway) {
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


void BobTheBot::OnGameStart() {
	Actions()->SendChat("\nBob the Bot\nCan we fix it ?\nBob the Bot\nYes, we can!!");
    
    observer = Observation();
    actions = Actions();
}


void BobTheBot::OnStep() {
    SupplyDepotManager(7);
    ContinuousSCVSpawn(2);
}


void BobTheBot::OnUnitIdle(const Unit* unit) {
    const ObservationInterface* observation = Observation();
    ActionInterface* actions = Actions();

    switch (unit->unit_type.ToType()) {
        case UNIT_TYPEID::TERRAN_BARRACKS: {
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
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


bool BobTheBot::TryBuildStructure(ABILITY_ID ability_type_for_structure)
{
    const Unit* unit_to_build = getAvailableSCV();
    if (!unit_to_build) {
        return false;
    }
    Point2D point(unit_to_build->pos.x, unit_to_build->pos.y);

    // Edge case for refineries, you need to provide the nearest geyser
    if (ability_type_for_structure == ABILITY_ID::BUILD_REFINERY) {
        const Unit* nearest_geyser = FindNearest(point, UNIT_TYPEID::NEUTRAL_VESPENEGEYSER);
        Actions()->UnitCommand(unit_to_build, ability_type_for_structure, nearest_geyser);
    }

    else {
        // Keep finding random positions until you get a valid one for placement
        float rx = GetRandomScalar();
        float ry = GetRandomScalar();
        point.x += rx * 15.0f;
        point.y += ry * 15.0f;
        while (!Query()->Placement(ability_type_for_structure, point))
        {
            rx = GetRandomScalar();
            ry = GetRandomScalar();
            point.x = unit_to_build->pos.x + rx * 15.0f;
            point.y = unit_to_build->pos.y + ry * 15.0f;
        }
        Actions()->UnitCommand(unit_to_build, ability_type_for_structure, point);
    }

    return true;
}


void BobTheBot::OnBuildingConstructionComplete(const Unit* unit)
{
    switch (unit->unit_type.ToType()) {

        // Immediately start building more SCVS when he have space
    case UNIT_TYPEID::TERRAN_SUPPLYDEPOT: {
        const ObservationInterface* observation = Observation();

        depotBuilding = false;

        //Units commandCenters = observation->GetUnits(Unit::Alliance::Self, isCommandCenter);
        //for (auto commandCenter: commandCenters) {
        //    // If we have enough minerals and supply
        //    if (observation->GetMinerals() >= 150 && (observation->GetFoodUsed() < observation->GetFoodCap()))
        //        if (TryBuildStructure(ABILITY_ID::BUILD_BARRACKS))
        //            numBarracks += 1;
        //    if (numBarracks == 1)
        //        if (TryBuildStructure(ABILITY_ID::BUILD_REFINERY))
        //            numRefineries += 1;

        //}
        break;
    }

    default:
        break;
    }
}


const Unit* BobTheBot::FindNearest(const Point2D& start, UNIT_TYPEID type) {
    Units units = Observation()->GetUnits(Unit::Alliance::Neutral);
    float distance = std::numeric_limits<float>::max();
    const Unit* target = nullptr;
    for (const auto& u : units) {
        if (u->unit_type == type) {
            float d = DistanceSquared2D(u->pos, start);
            if (d < distance) {
                distance = d;
                target = u;
            }
        }
    }
    return target;
}


const Unit* BobTheBot::getAvailableSCV() {
    Units allSCVs = Observation()->GetUnits(Unit::Alliance::Self, isSCV);
    for (const Unit* scv : allSCVs) {
        if ((scv->orders).size() == 1) {
            return scv;
        }
    }
    return nullptr;
}