#ifndef BASIC_SC2_BOT_H_
#define BASIC_SC2_BOT_H_

#include "sc2api/sc2_api.h"
#include "sc2api/sc2_args.h"
#include "sc2lib/sc2_lib.h"
#include "sc2utils/sc2_manage_process.h"
#include "sc2utils/sc2_arg_parser.h"

using namespace sc2;

class BobTheBot : public sc2::Agent {
public:
	virtual void OnGameStart();
	virtual void OnStep();
	virtual void OnUnitIdle(const Unit* unit);
	virtual void OnBuildingConstructionComplete(const Unit* unit);
	const Unit* getAvailableSCV();
	void BobTheBot::freeSCV(const Unit* scv);


private:
	bool TryBuildStructure(ABILITY_ID ability_type_for_structure);
	const Unit* FindNearest(const Point2D& start, UNIT_TYPEID type);
	Units availableSCVs;
	int numSCVs = 12;
	int numDepots = 0;
	int numBarracks = 0;
	int numRefineries = 0;
};

#endif