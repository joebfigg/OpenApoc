# Pack the mod's source XML into a single zip file that the engine loads via
# modinfo.xml's <statepath>. Same pattern as the crashing_vehicles mod.

set(MOD_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(MOD_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/modern_weapons_gamestate)

set(MOD_SOURCES
	${MOD_SRC_DIR}/gamestate.xml
	${MOD_SRC_DIR}/gamestate/agent_equipment.xml
	${MOD_SRC_DIR}/gamestate/economy.xml)

add_custom_command(OUTPUT ${MOD_OUTPUT}
	COMMAND ${CMAKE_COMMAND} -E tar cf ${MOD_OUTPUT} --format=zip
		gamestate.xml gamestate
	DEPENDS ${MOD_SOURCES}
	WORKING_DIRECTORY ${MOD_SRC_DIR}
	COMMENT "Packing modern_weapons mod"
	VERBATIM)

add_custom_target(mod-modern-weapons ALL DEPENDS ${MOD_OUTPUT})
