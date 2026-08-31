#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/gamestate.h"
#include "game/state/gamestate_serialize.h"
#include "game/state/rules/aequipmenttype.h"
#include "library/strings.h"
#include <iostream>

// Deliberately NOT "using namespace OpenApoc" - the namespace defines a templated
// operator== for shared_ptr which MSVC's ppltasks.h then picks up, failing the build.
using OpenApoc::AEquipmentType;
using OpenApoc::config;
using OpenApoc::Framework;
using OpenApoc::GameState;
using OpenApoc::mksp;
using OpenApoc::sp;
using OpenApoc::UString;

static bool checkWeapon(sp<GameState> state, const UString &weaponId, const UString &clipId)
{
	auto wit = state->agent_equipment.find(weaponId);
	if (wit == state->agent_equipment.end())
	{
		LogError("{0} not present in agent_equipment", weaponId);
		return false;
	}
	auto cit = state->agent_equipment.find(clipId);
	if (cit == state->agent_equipment.end())
	{
		LogError("{0} not present in agent_equipment", clipId);
		return false;
	}
	auto weapon = wit->second;
	auto clip = cit->second;

	if (weapon->type != AEquipmentType::Type::Weapon)
	{
		LogError("{0} is not a Weapon", weaponId);
		return false;
	}
	if (clip->type != AEquipmentType::Type::Ammo)
	{
		LogError("{0} is not Ammo", clipId);
		return false;
	}

	// initState() fills weapon->ammo_types from the clip's weapon_types - this is the
	// documented modder path for adding ammunition, so assert it end to end.
	bool linked = false;
	for (auto &at : weapon->ammo_types)
	{
		if (at.id == clipId)
		{
			linked = true;
			break;
		}
	}
	if (!linked)
	{
		LogError("{0} did not get {1} in its ammo_types", weaponId, clipId);
		return false;
	}

	if (!weapon->equipscreen_sprite)
	{
		LogError("{0} equip screen sprite failed to load", weaponId);
		return false;
	}
	if (!clip->equipscreen_sprite)
	{
		LogError("{0} equip screen sprite failed to load", clipId);
		return false;
	}
	if (clip->damage <= 0 || clip->fire_delay <= 0 || clip->max_ammo <= 0)
	{
		LogError("{0} ballistic stats look unset", clipId);
		return false;
	}
	if (state->economy.find(weaponId) == state->economy.end() ||
	    state->economy.find(clipId) == state->economy.end())
	{
		LogError("{0}/{1} missing from economy", weaponId, clipId);
		return false;
	}

	LogInfo("{0} verified: linked to {1}, dmg {2}, fire_delay {3}, {4} rounds", weaponId, clipId,
	        clip->damage, clip->fire_delay, clip->max_ammo);
	return true;
}

int main(int argc, char **argv)
{
	config().addPositionalArgument("common", "Common gamestate to load");
	config().addPositionalArgument("gamestate", "Gamestate to load");
	config().addPositionalArgument("mod", "Mod gamestate to load");

	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	auto common_name = config().getString("common");
	auto gamestate_name = config().getString("gamestate");
	auto mod_name = config().getString("mod");
	if (common_name.empty() || gamestate_name.empty() || mod_name.empty())
	{
		std::cerr << "Must provide common, gamestate and mod paths\n";
		config().showHelp();
		return EXIT_FAILURE;
	}

	Framework fw("OpenApoc", false);
	// The game binary mounts each mod's data directory during boot; the headless
	// framework does not, so mount them here or the mod's PNG paths won't resolve.
	fw.setupModDataPaths();

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
	if (!state->loadGame(mod_name))
	{
		LogError("Failed to load mod gamestate");
		return EXIT_FAILURE;
	}

	state->startGame();
	state->initState();

	if (!checkWeapon(state, "AEQUIPMENTTYPE_OA_109", "AEQUIPMENTTYPE_OA_109_CLIP"))
	{
		return EXIT_FAILURE;
	}
	if (!checkWeapon(state, "AEQUIPMENTTYPE_MARSEC_15A", "AEQUIPMENTTYPE_MARSEC_15A_CLIP"))
	{
		return EXIT_FAILURE;
	}

	LogInfo("Modern Weapons mod tests passed");
	return EXIT_SUCCESS;
}
