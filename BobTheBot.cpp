#include "BobTheBot.h"

void BobTheBot::OnGameStart() {
	Actions()->SendChat("Bob the Bot\nCan we fix it ?\nBob the Bot\nYes, we can!!");
}


void BobTheBot::OnStep() {
    // TODO: Migrate logic to manager
    //resourceManager.Step();
    size_t mineralCount = Observation()->GetMinerals();
    if (numDepots < 4 && mineralCount >= 100) {
        if (TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT)) {
            numDepots += 1;
        }
    }
}


void BobTheBot::OnUnitIdle(const Unit* unit) {
    switch (unit->unit_type.ToType()) {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
            const ObservationInterface* observation = Observation();
            // If we have enough minerals and supply
            if ((observation->GetMinerals() >= 50) && (observation->GetFoodUsed() < observation->GetFoodCap()))
                Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_SCV);
            break;
        }
        case UNIT_TYPEID::TERRAN_BARRACKS: {
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
        }
        case UNIT_TYPEID::TERRAN_SCV: {
            const Unit* mineral_target = FindNearestMineralPatch(unit->pos);
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


const Unit* BobTheBot::FindNearestMineralPatch(const Point2D& start) {
    Units units = Observation()->GetUnits(Unit::Alliance::Neutral);
    float distance = std::numeric_limits<float>::max();
    const Unit* target = nullptr;
    for (const auto& u : units) {
        if (u->unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD) {
            float d = DistanceSquared2D(u->pos, start);
            if (d < distance) {
                distance = d;
                target = u;
            }
        }
    }
    return target;
}


bool BobTheBot::TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type)
{
    const ObservationInterface* observation = Observation();
    // If a unit already is building a supply structure of this type, do nothing.
    const Unit* unit_to_build = nullptr;
    Units units = observation->GetUnits(Unit::Alliance::Self);
    for (const auto& unit : units) {
        for (const auto& order : unit->orders) {
            if (order.ability_id == ability_type_for_structure) {
                return false;
            }
        }
        if (unit->unit_type == unit_type) {
            unit_to_build = unit;
        }
    }

    // Keep finding random positions until you get a valid one for placement
    float rx = GetRandomScalar();
    float ry = GetRandomScalar();
    Point2D point(unit_to_build->pos.x + rx * 15.0f, unit_to_build->pos.y + ry * 15.0f);
    while (!Query()->Placement(ability_type_for_structure, point))
    {
        rx = GetRandomScalar();
        ry = GetRandomScalar();
        point.x = unit_to_build->pos.x + rx * 15.0f;
        point.y = unit_to_build->pos.y + ry * 15.0f;
    }

    Actions()->UnitCommand(unit_to_build, ability_type_for_structure, point);
    return true;
}


bool isCommandCenter(const Unit& unit)
{
    return unit.unit_type == UNIT_TYPEID::TERRAN_COMMANDCENTER;
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
                if (observation->GetMinerals() >= 50 && (observation->GetFoodUsed() < observation->GetFoodCap()))
                    Actions()->UnitCommand(commandCenter, ABILITY_ID::TRAIN_SCV);
            }
            break;
        }
        default:
            break;
    }
}