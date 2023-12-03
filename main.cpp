#include <iostream>
#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_lib.h"
#include "sc2utils/sc2_manage_process.h"
#include "sc2utils/sc2_arg_parser.h"

#include "bot_examples.h"
#include "BobTheBot.h"
#include "LadderInterface.h"

// LadderInterface allows the bot to be tested against the built-in AI or
// played against other bots
int main(int argc, char* argv[]) {
    auto agent = new TerranMultiplayerBot();
	RunBot(argc, argv, agent, sc2::Race::Terran);
    delete agent;
	return 0;
}