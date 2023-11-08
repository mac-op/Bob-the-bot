#include "BobTheBot.h"


bool isCommandCenter(const Unit& unit)
{
    return unit.unit_type == UNIT_TYPEID::TERRAN_COMMANDCENTER;
}


bool isSCV(const Unit& unit)
{
    return unit.unit_type == UNIT_TYPEID::TERRAN_SCV;
}


void BobTheBot::OnGameStart() {
	Actions()->SendChat("Bob the Bot\nCan we fix it ?\nBob the Bot\nYes, we can!!");
    availableSCVs = Observation()->GetUnits(Unit::Alliance::Self, isSCV);
}


void BobTheBot::OnStep() {
    size_t mineralCount = Observation()->GetMinerals();
    if (numDepots < 1 && mineralCount >= 100) {
        if (TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT)) {
            numDepots += 1;
        }
    }
}


void BobTheBot::OnUnitIdle(const Unit* unit) {
    switch (unit->unit_type.ToType()) {
        // Endless SCV training
        //case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
        //    const ObservationInterface* observation = Observation();
        //    // If we have enough minerals and supply
        //    if ((observation->GetMinerals() >= 50) && (observation->GetFoodUsed() < observation->GetFoodCap()))
        //        Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_SCV);
        //    break;
        //}
        case UNIT_TYPEID::TERRAN_BARRACKS: {
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
        }
        case UNIT_TYPEID::TERRAN_SCV: {
            const Unit* mineral_target = FindNearest(unit->pos, UNIT_TYPEID::NEUTRAL_MINERALFIELD);
            if (!mineral_target) {
                break;
            }
            Actions()->UnitCommand(unit, ABILITY_ID::SMART, mineral_target);
            break;
        }
        default: {
            break;
        }
    }
}


bool BobTheBot::TryBuildStructure(ABILITY_ID ability_type_for_structure)
{
    const Unit* unit_to_build = getAvailableSCV();  // CRITICAL TO-DO: Free up unit when you are done with it
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
            Units commandCenters = observation->GetUnits(Unit::Alliance::Self, isCommandCenter);
            for (auto commandCenter: commandCenters) {
                // If we have enough minerals and supply
                if (observation->GetMinerals() >= 150 && (observation->GetFoodUsed() < observation->GetFoodCap()))
                    if (TryBuildStructure(ABILITY_ID::BUILD_BARRACKS))
                        numBarracks += 1;
                if (numBarracks == 1)
                    if (TryBuildStructure(ABILITY_ID::BUILD_REFINERY))
                        numRefineries += 1;

                    // Actions()->UnitCommand(commandCenter, ABILITY_ID::TRAIN_SCV);    // Disable infinite scv training for now
            }
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
    const Unit* available = availableSCVs.back();
    availableSCVs.pop_back();
    return available;
}


void BobTheBot::freeSCV(const Unit* scv) {
    availableSCVs.push_back(scv);
}