#include "BasicSc2Bot.h"

void BasicSc2Bot::OnGameStart() {
	Actions()->SendChat("Bob the Bot\nCan we fix it ?\nBob the Bot\nYes, we can!!");
	return; 
}

void BasicSc2Bot::OnStep() {
	return;
}

void BasicSc2Bot::OnUnitIdle(const Unit* unit) {
    switch (unit->unit_type.ToType()) {
        case UNIT_TYPEID::TERRAN_SCV: {
            TryBuildSupplyDepot();
            break;
        }
        default: {
            break;
        }
    }
}

bool BasicSc2Bot::TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type)
{
    const ObservationInterface* observation = Observation();
    // If a unit already is building a supply structure of this type, do nothing.
    // Also get an scv to build the structure.
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
    float rx = GetRandomScalar();
    float ry = GetRandomScalar();
    Actions()->UnitCommand(unit_to_build,
        ability_type_for_structure,
        Point2D(unit_to_build->pos.x + rx * 15.0f, unit_to_build->pos.y + ry * 15.0f));
    return true;
}

bool isSupplyDepot(const Unit& unit)
{
    return unit.unit_type == UNIT_TYPEID::TERRAN_SUPPLYDEPOT;
}

bool BasicSc2Bot::TryBuildSupplyDepot() {
    size_t numDepos = Observation()->GetUnits(Unit::Alliance::Self, isSupplyDepot).size();
    if (numDepos < 2)
        return TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT);
    return false;
}


