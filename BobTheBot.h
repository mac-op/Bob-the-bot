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

    size_t CountUnitType(UNIT_TYPEID unit_type) {
        return observer->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
    }

    void ManageBarracks(int maxBarracks);

    void Scout();
    const Unit *GetRandomUnit(UnitTypeID unit_type);

    GameInfo gameInfo;

    bool FindEnemyPosition(Point2D &target_pos);

    bool FindRandomLocation(const Unit *unit, Point2D &target_pos);

    void AttackWithUnit(const Unit *unit);

    void ManageOffensive();
};

#endif