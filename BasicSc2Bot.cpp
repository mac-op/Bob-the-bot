#include "BasicSc2Bot.h"

void BasicSc2Bot::OnGameStart() {
	Actions()->SendChat("Bob the Bot\nCan we fix it ?\nBob the Bot\nYes, we can!!");
	return; 
}


void BasicSc2Bot::OnStep() {
    size_t mineralCount = Observation()->GetMinerals();
    if (numDepots < 1 && mineralCount >= 100) {
        if (TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT)) {
            numDepots += 1;
        }
    }
	return;
}


void BasicSc2Bot::OnUnitIdle(const Unit* unit) {
    switch (unit->unit_type.ToType()) {

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


const Unit* BasicSc2Bot::FindNearestMineralPatch(const Point2D& start) {
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


bool BasicSc2Bot::TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type)
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