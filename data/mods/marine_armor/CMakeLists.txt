# Pack the mod's source XML into a single zip the engine loads via modinfo.xml's
# <statepath>. Same pattern as the crashing_vehicles mod.

set(MOD_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(MOD_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/marine_armor_gamestate)

set(MOD_SOURCES
	${MOD_SRC_DIR}/gamestate.xml
	${MOD_SRC_DIR}/gamestate/agent_equipment.xml
	${MOD_SRC_DIR}/gamestate/economy.xml)

add_custom_command(OUTPUT ${MOD_OUTPUT}
	COMMAND ${CMAKE_COMMAND} -E tar cf ${MOD_OUTPUT} --format=zip
		gamestate.xml gamestate
	DEPENDS ${MOD_SOURCES}
	WORKING_DIRECTORY ${MOD_SRC_DIR}
	COMMENT "Packing marine_armor mod"
	VERBATIM)

add_custom_target(mod-marine-armor ALL DEPENDS ${MOD_OUTPUT})
