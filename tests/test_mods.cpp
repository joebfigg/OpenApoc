#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/gamestate.h"
#include "game/state/gamestate_serialize.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/agenttype.h"
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

// The mod appends radio voice lines to the vanilla pools (3 male / 2 female each).
// List deserialization appends and map deserialization merges by key, so after the
// mod loads the pools must be larger than vanilla and every sample non-null.
static bool checkVoices(sp<GameState> state, const UString &agentTypeId)
{
	using Gender = OpenApoc::AgentType::Gender;
	auto it = state->agent_types.find(agentTypeId);
	if (it == state->agent_types.end())
	{
		LogError("{0} not present in agent_types", agentTypeId);
		return false;
	}
	auto type = it->second;

	struct Check
	{
		const char *name;
		Gender gender;
		size_t vanilla;
		std::list<sp<OpenApoc::Sample>> *pool;
	};
	const Check checks[] = {
	    {"damageSfx male", Gender::Male, 3, &type->damageSfx[Gender::Male]},
	    {"damageSfx female", Gender::Female, 2, &type->damageSfx[Gender::Female]},
	    {"fatalWoundSfx male", Gender::Male, 3, &type->fatalWoundSfx[Gender::Male]},
	    {"fatalWoundSfx female", Gender::Female, 2, &type->fatalWoundSfx[Gender::Female]},
	    {"dieSfx male", Gender::Male, 3, &type->dieSfx[Gender::Male]},
	    {"dieSfx female", Gender::Female, 2, &type->dieSfx[Gender::Female]},
	};
	for (const auto &c : checks)
	{
		if (c.pool->size() <= c.vanilla)
		{
			LogError("{0} {1}: pool size {2} not grown past vanilla {3}", agentTypeId, c.name,
			         (unsigned)c.pool->size(), (unsigned)c.vanilla);
			return false;
		}
		for (const auto &s : *c.pool)
		{
			if (!s)
			{
				LogError("{0} {1}: contains a null sample (a WAV failed to load)", agentTypeId,
				         c.name);
				return false;
			}
		}
	}
	LogInfo("{0} voice pools verified", agentTypeId);
	return true;
}

// The marine armor set is purchasable equipment, one item per body part, each
// binding the matching restyled image pack. Worn armor overrides the agent's
// base appearance per body part (Agent::getImagePack checks the armor slot
// first), so the marine look comes from equipping these rather than from
// replacing the unarmored body.
static bool checkArmorSet(sp<GameState> state)
{
	using BodyPart = OpenApoc::BodyPart;
	const struct
	{
		const char *id;
		BodyPart part;
		const char *pack;
	} pieces[] = {
	    {"AEQUIPMENTTYPE_MARINE_BODY_ARMOR", BodyPart::Body, "BATTLEUNITIMAGEPACK_marine1a"},
	    {"AEQUIPMENTTYPE_MARINE_LEG_ARMOR", BodyPart::Legs, "BATTLEUNITIMAGEPACK_marine1b"},
	    {"AEQUIPMENTTYPE_MARINE_HELMET", BodyPart::Helmet, "BATTLEUNITIMAGEPACK_marine1c"},
	    {"AEQUIPMENTTYPE_MARINE_LEFT_ARM", BodyPart::LeftArm, "BATTLEUNITIMAGEPACK_marine1d"},
	    {"AEQUIPMENTTYPE_MARINE_RIGHT_ARM", BodyPart::RightArm, "BATTLEUNITIMAGEPACK_marine1e"},
	};

	for (const auto &p : pieces)
	{
		auto it = state->agent_equipment.find(p.id);
		if (it == state->agent_equipment.end())
		{
			LogError("{0} not present in agent_equipment", p.id);
			return false;
		}
		auto item = it->second;
		if (item->type != AEquipmentType::Type::Armor)
		{
			LogError("{0} is not Armor", p.id);
			return false;
		}
		if (item->body_part != p.part)
		{
			LogError("{0} covers the wrong body part", p.id);
			return false;
		}
		if (item->body_image_pack.id != p.pack)
		{
			LogError("{0} binds {1}, expected {2}", p.id, item->body_image_pack.id, p.pack);
			return false;
		}
		if (!item->body_sprite || !item->equipscreen_sprite)
		{
			LogError("{0} paper doll or inventory art failed to load", p.id);
			return false;
		}
		if (item->armor <= 0)
		{
			LogError("{0} has no armor value", p.id);
			return false;
		}
		if (state->economy.find(p.id) == state->economy.end())
		{
			LogError("{0} is not purchasable - missing from economy", p.id);
			return false;
		}
	}

	// The unarmored appearance must be left alone: the marine look is worn, not innate.
	auto at = state->agent_types.find("AGENTTYPE_X-COM_AGENT_HUMAN");
	if (at != state->agent_types.end() && !at->second->image_packs.empty())
	{
		auto body = at->second->image_packs[0].find(BodyPart::Body);
		if (body != at->second->image_packs[0].end() &&
		    body->second.id.find("marine") != OpenApoc::UString::npos)
		{
			LogError("base unarmored appearance was overridden; it should stay vanilla");
			return false;
		}
	}

	LogInfo("marine armor set verified (5 purchasable pieces)");
	return true;
}

int main(int argc, char **argv)
{
	config().addPositionalArgument("common", "Common gamestate to load");
	config().addPositionalArgument("gamestate", "Gamestate to load");
	config().addPositionalArgument("mod", "modern_weapons gamestate to load");
	config().addPositionalArgument("mod2", "agent_voices gamestate to load");
	config().addPositionalArgument("mod3", "marine_armor gamestate to load");

	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	auto common_name = config().getString("common");
	auto gamestate_name = config().getString("gamestate");
	auto mod_name = config().getString("mod");
	auto mod2_name = config().getString("mod2");
	auto mod3_name = config().getString("mod3");
	if (common_name.empty() || gamestate_name.empty() || mod_name.empty() ||
	    mod2_name.empty() || mod3_name.empty())
	{
		std::cerr << "Must provide common, gamestate and all three mod paths\n";
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
		LogError("Failed to load modern_weapons gamestate");
		return EXIT_FAILURE;
	}
	if (!state->loadGame(mod2_name))
	{
		LogError("Failed to load agent_voices gamestate");
		return EXIT_FAILURE;
	}
	if (!state->loadGame(mod3_name))
	{
		LogError("Failed to load marine_armor gamestate");
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

	if (!checkVoices(state, "AGENTTYPE_X-COM_AGENT_HUMAN"))
	{
		return EXIT_FAILURE;
	}
	if (!checkVoices(state, "AGENTTYPE_X-COM_AGENT_HYBRID"))
	{
		return EXIT_FAILURE;
	}

	if (!checkArmorSet(state))
	{
		return EXIT_FAILURE;
	}

	LogInfo("Mod tests passed (modern_weapons + agent_voices + marine_armor)");
	return EXIT_SUCCESS;
}
