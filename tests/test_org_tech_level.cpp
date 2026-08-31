#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/gamestate.h"
#include "game/state/gamestate_serialize.h"
#include "game/state/shared/organisation.h"
#include "library/strings.h"
#include <algorithm>
#include <iostream>
#include <map>

// Deliberately NOT "using namespace OpenApoc" - the namespace defines a templated
// operator== for shared_ptr which MSVC's ppltasks.h then picks up, failing the build.
using OpenApoc::config;
using OpenApoc::Framework;
using OpenApoc::GameState;
using OpenApoc::mksp;
using OpenApoc::Organisation;
using OpenApoc::sp;
using OpenApoc::UString;

// Snapshot every non-player, non-alien organisation's tech level.
static std::map<UString, int> snapshotTechLevels(sp<GameState> state)
{
	std::map<UString, int> levels;
	for (auto &[id, org] : state->organisations)
	{
		if (id != state->player.id && id != state->aliens.id)
		{
			levels[id] = org->tech_level;
		}
	}
	return levels;
}

// Human organisations gain one tech level at the start of each week. This is what
// escalates the equipment their guards carry over the course of a campaign.
static bool testTechLevelRisesEachWeek(sp<GameState> state)
{
	auto before = snapshotTechLevels(state);
	if (before.empty())
	{
		LogError("No human organisations found in gamestate");
		return false;
	}

	state->updateEndOfWeek(false);
	auto after = snapshotTechLevels(state);

	for (auto &[id, levelBefore] : before)
	{
		const int levelAfter = after[id];
		const int expected =
		    std::min(levelBefore + 1, Organisation::MAX_TECH_LEVEL);

		if (levelAfter != expected)
		{
			LogError("Org {0}: tech level went {1} -> {2}, expected {3}", id, levelBefore,
			         levelAfter, expected);
			return false;
		}
	}

	LogInfo("Tech level rose for {0} organisations", (unsigned)before.size());
	return true;
}

// A fully infiltrated organisation gains an immediate +3 over its current level, once,
// on the transition into being taken over.
static bool testTakeoverBonus(sp<GameState> state)
{
	sp<OpenApoc::Organisation> target;
	for (auto &[id, org] : state->organisations)
	{
		if (id != state->player.id && id != state->aliens.id && !org->takenOver)
		{
			target = org;
			break;
		}
	}
	if (!target)
	{
		LogError("No takeover candidate found");
		return false;
	}

	const int before = target->tech_level;
	const int expected = std::min(before + 3, Organisation::MAX_TECH_LEVEL);

	target->takeOver(*state, true);
	if (target->tech_level != expected)
	{
		LogError("Org {0}: takeover moved tech level {1} -> {2}, expected {3}", target->id,
		         before, target->tech_level, expected);
		return false;
	}

	// A second (forced) takeover must not stack the bonus.
	target->takeOver(*state, true);
	if (target->tech_level != expected)
	{
		LogError("Org {0}: repeated takeover stacked bonus to {1}, expected {2}", target->id,
		         target->tech_level, expected);
		return false;
	}

	LogInfo("Takeover bonus: {0} went {1} -> {2} once", target->id, before, expected);
	return true;
}

// The ceiling is twelve, matching the twelve EQUIPMENTSET_HUMAN_n sets the extractor
// builds from the original data. Running well past it must not push any org higher.
static bool testTechLevelCaps(sp<GameState> state)
{
	for (int week = 0; week < Organisation::MAX_TECH_LEVEL + 6; week++)
	{
		state->updateEndOfWeek(false);
	}

	for (auto &[id, level] : snapshotTechLevels(state))
	{
		if (level != Organisation::MAX_TECH_LEVEL)
		{
			LogError("Org {0}: tech level {1} after saturation, expected {2}", id, level,
			         Organisation::MAX_TECH_LEVEL);
			return false;
		}
	}

	LogInfo("All organisations capped at {0}", Organisation::MAX_TECH_LEVEL);
	return true;
}

// The player draws no equipment from tech level and alien loadouts are score-driven,
// so neither should be advanced by the weekly update.
static bool testPlayerAndAliensUntouched(sp<GameState> state)
{
	const int playerBefore = state->getPlayer()->tech_level;
	const int aliensBefore = state->getAliens()->tech_level;

	state->updateEndOfWeek(false);

	if (state->getPlayer()->tech_level != playerBefore)
	{
		LogError("Player tech level changed {0} -> {1}", playerBefore,
		         state->getPlayer()->tech_level);
		return false;
	}
	if (state->getAliens()->tech_level != aliensBefore)
	{
		LogError("Alien tech level changed {0} -> {1}", aliensBefore,
		         state->getAliens()->tech_level);
		return false;
	}

	LogInfo("Player and alien tech levels untouched");
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

	if (!testPlayerAndAliensUntouched(state))
	{
		LogError("Player/alien tech level test failed");
		return EXIT_FAILURE;
	}

	if (!testTechLevelRisesEachWeek(state))
	{
		LogError("Weekly tech level test failed");
		return EXIT_FAILURE;
	}

	if (!testTakeoverBonus(state))
	{
		LogError("Takeover bonus test failed");
		return EXIT_FAILURE;
	}

	if (!testTechLevelCaps(state))
	{
		LogError("Tech level cap test failed");
		return EXIT_FAILURE;
	}

	LogInfo("Org tech level tests passed");
	return EXIT_SUCCESS;
}
