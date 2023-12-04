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
	virtual void OnUnitDestroyed(const Unit* unit);

    const std::vector<UNIT_TYPEID> armyTypes {
        UNIT_TYPEID::TERRAN_REAPER, UNIT_TYPEID::TERRAN_MARINE, UNIT_TYPEID::TERRAN_SIEGETANK,
        UNIT_TYPEID::TERRAN_THOR, UNIT_TYPEID::TERRAN_HELLIONTANK, UNIT_TYPEID::TERRAN_HELLION
    };
private:
	// Game state
	const ObservationInterface* observer = Observation();
	ActionInterface* actions = Actions();
	QueryInterface* query = Query();
    GameInfo gameInfo;
    const Unit* latestCommCen;
    std::vector<Point3D> expansionLocations;
    std::vector<Point3D> expansionLocationsLeft;
    std::vector<const Unit*> geysersToBuildOn;

	// Bot managers
	void ContinuousSCVSpawn(int leeway);
	void SupplyDepotManager(int sensitivity);
	void RefineryManager();
	void CommandCenterManager();
	bool MineMinerals(const Unit* scv);

	// Helpers
	bool TryBuildStructure(ABILITY_ID ability_type_for_structure, const Unit* unit_to_build_on = nullptr);
	const Unit* FindNearest(const Point2D& start, UNIT_TYPEID unitType);
	const Unit* FindSecondNearest(const Point2D& start, UNIT_TYPEID unitType);
	const Unit* getAvailableSCV();
	Point2D getValidNearbyLocation(Point2D location, ABILITY_ID ability_type_for_structure);
	void sendSCVScout(const Unit* SCV, Point2D location);
	void getExpansionLocations();
	static bool compareDistance(const Point3D& p1, const Point3D& p2, const Point3D& referencePoint);

    void ManageOffensiveStructures();

    void Scout();
    const Unit *GetRandomUnit(UnitTypeID unit_type);

    bool FindEnemyPosition(Point2D &target_pos);

    bool FindRandomLocation(const Unit *unit, Point2D &target_pos);

    void AttackWithUnit(const Unit *unit);

    void ManageOffensive();

    void BuildAddOn(ABILITY_ID ability, const Unit *unit);

    void UpgradeArmy();

    void Attack();

    struct {
        uint32_t gameLoop = 350;
        uint32_t delay = 150;
    } ScoutDelay;

    void BuildArmy();
};

#endif