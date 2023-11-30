#ifndef BASIC_SC2_BOT_H_
#define BASIC_SC2_BOT_H_

#include "sc2api/sc2_api.h"
#include "sc2api/sc2_args.h"
#include "sc2lib/sc2_lib.h"
#include "sc2utils/sc2_manage_process.h"
#include "sc2utils/sc2_arg_parser.h"
#include "sc2api/sc2_unit_filters.h"
using namespace sc2;

class BobTheBot : public Agent {
public:
	// API-Provided functions
	virtual void OnGameStart();
	virtual void OnStep();
	virtual void OnUnitIdle(const Unit* unit);
	virtual void OnBuildingConstructionComplete(const Unit* unit);

private:
	// Game state
	const ObservationInterface* observer = Observation();
	ActionInterface* actions = Actions();

	// Bot managers
	void ContinuousSCVSpawn(int leeway);
	void SupplyDepotManager(int sensitivity);
	bool MineMinerals(const Unit* scv);

	// Helpers
	bool TryBuildStructure(ABILITY_ID ability_type_for_structure);
	const Unit* FindNearest(const Point2D& start, UNIT_TYPEID type);
	const Unit* getAvailableSCV();

	// Filters
    // TODO: Remove this bc we don't need these anymore (check out IsUnit functor)
//	static bool isCommandCenter(const Unit& unit) { return unit.unit_type == UNIT_TYPEID::TERRAN_COMMANDCENTER; }
//	static bool isSCV(const Unit& unit) { return unit.unit_type == UNIT_TYPEID::TERRAN_SCV; }

    size_t CountUnitType(UNIT_TYPEID unit_type) {
        return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
    }

    void ManageBarracks(int maxBarracks);
};

#endif