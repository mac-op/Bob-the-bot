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
	QueryInterface* query = Query();

	// Bot managers
	void ContinuousSCVSpawn(int leeway);
	void SupplyDepotManager(int sensitivity);
	void RefineryManager();
	void CommandCenterManager();
	bool MineMinerals(const Unit* scv);

	// Helpers
	bool TryBuildStructure(ABILITY_ID ability_type_for_structure, Point2D locationApprox = Point2D(0, 0), const Unit* unit_to_build_on = nullptr);
	const Unit* FindNearest(const Point2D& start, bool (*filterType)(const Unit&));
	const Unit* FindSecondNearest(const Point2D& start, bool (*filterType)(const Unit&));
	const Unit* getAvailableSCV();
	Point2D getValidNearbyLocation(Point2D location, ABILITY_ID ability_type_for_structure);
	void sendSCVScout(const Unit* SCV, Point2D location);
	std::vector<Point3D> getExpansionLocations();
	std::vector<Point3D> expansionLocations;
	std::vector<const Unit*> geysersToBuildOn;
	bool compareDistance(const Point3D& p1, const Point3D& p2, const Point3D& referencePoint);
	const Unit* initialCommCen;

	// Filters
	static bool isCommandCenter(const Unit& unit) { return unit.unit_type == UNIT_TYPEID::TERRAN_COMMANDCENTER; }
	static bool isSCV(const Unit& unit) { return unit.unit_type == UNIT_TYPEID::TERRAN_SCV; }
	static bool isMineralField(const Unit& unit) { return unit.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD; }
	static bool isGeyser(const Unit& unit) { return unit.unit_type == UNIT_TYPEID::NEUTRAL_VESPENEGEYSER; }

    size_t CountUnitType(UNIT_TYPEID unit_type) {
        return observer->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
    }

    void ManageOffensiveStructures();

    void Scout();
    const Unit *GetRandomUnit(UnitTypeID unit_type);

    GameInfo gameInfo;

    bool FindEnemyPosition(Point2D &target_pos);

    bool FindRandomLocation(const Unit *unit, Point2D &target_pos);

    void AttackWithUnit(const Unit *unit);

    void ManageOffensive();
};

#endif