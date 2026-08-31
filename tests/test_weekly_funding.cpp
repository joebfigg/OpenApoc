#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/gamestate.h"
#include "game/state/gamestate_serialize.h"
#include "game/state/shared/organisation.h"
#include "library/strings.h"
#include <iostream>

// Deliberately NOT "using namespace OpenApoc" - the namespace defines a templated
// operator== for shared_ptr which MSVC's ppltasks.h then picks up, failing the build.
using OpenApoc::config;
using OpenApoc::Framework;
using OpenApoc::GameState;
using OpenApoc::mksp;
using OpenApoc::sp;

// With a positive weekly rating, the assessment must snapshot this week's income and
// the applied adjustment, and roll player->income forward by exactly that adjustment.
static bool testPositiveRating(sp<GameState> state)
{
	auto player = state->getPlayer();

	state->weekScore.reset();
	state->weekScore.tacticalMissions = 100000;

	const int baseIncome = player->income;
	const int modifier = state->calculateFundingModifier();
	if (modifier == 0)
	{
		LogError("Expected a nonzero funding modifier for a huge positive rating");
		return false;
	}
	const int expectedAdjustment = baseIncome / modifier;

	state->updateEndOfWeek(false);

	if (state->fundingReportIncome > baseIncome)
	{
		LogError("Reported income {0} exceeds base income {1}", state->fundingReportIncome,
		         baseIncome);
		return false;
	}
	if (state->fundingReportAdjustment != expectedAdjustment)
	{
		LogError("Reported adjustment {0}, expected {1}", state->fundingReportAdjustment,
		         expectedAdjustment);
		return false;
	}
	if (player->income != baseIncome + expectedAdjustment)
	{
		LogError("Income rolled to {0}, expected {1}", player->income,
		         baseIncome + expectedAdjustment);
		return false;
	}

	LogInfo("Positive rating: income {0}, adjustment {1}, next week {2}", baseIncome,
	        expectedAdjustment, player->income);
	return true;
}

// With a neutral rating the adjustment must be zero and income must not move.
static bool testNeutralRating(sp<GameState> state)
{
	auto player = state->getPlayer();

	state->weekScore.reset();
	const int baseIncome = player->income;

	if (state->calculateFundingModifier() != 0)
	{
		LogError("Expected zero funding modifier for a neutral rating");
		return false;
	}

	state->updateEndOfWeek(false);

	if (state->fundingReportAdjustment != 0)
	{
		LogError("Neutral rating produced adjustment {0}", state->fundingReportAdjustment);
		return false;
	}
	if (player->income != baseIncome)
	{
		LogError("Neutral rating moved income {0} -> {1}", baseIncome, player->income);
		return false;
	}

	LogInfo("Neutral rating: income {0} unchanged, adjustment 0", baseIncome);
	return true;
}

int main(int argc, char **argv)
{
	config().addPositionalArgument("common", "Common gamestate to load");
	config().addPositionalArgument("gamestate", "Gamestate to load");

	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	auto common_name = config().getString("common");
	auto gamestate_name = config().getString("gamestate");
	if (common_name.empty() || gamestate_name.empty())
	{
		std::cerr << "Must provide common gamestate and gamestate\n";
		config().showHelp();
		return EXIT_FAILURE;
	}

	Framework fw("OpenApoc", false);

	auto state = mksp<GameState>();
	if (!state->loadGame(common_name))
	{
		LogError("Failed to load common gamestate");
		return EXIT_FAILURE;
	}
	if (!state->loadGame(gamestate_name))
	{
		LogError("Failed to load supplied gamestate");
		return EXIT_FAILURE;
	}

	state->startGame();
	state->initState();
	state->fillPlayerStartingProperty();

	if (!testPositiveRating(state))
	{
		LogError("Positive rating funding test failed");
		return EXIT_FAILURE;
	}

	if (!testNeutralRating(state))
	{
		LogError("Neutral rating funding test failed");
		return EXIT_FAILURE;
	}

	LogInfo("Weekly funding tests passed");
	return EXIT_SUCCESS;
}
