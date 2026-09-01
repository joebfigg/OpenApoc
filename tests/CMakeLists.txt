PROJECT (OpenApoc_Tests CXX C)

set (TEST_LIST test_rect test_voxel test_tilemap test_rng test_images
		test_unicode test_colour test_backtrace test_strings)

foreach(TEST ${TEST_LIST})
		add_executable(${TEST} ${TEST}.cpp)
		# Just add all libs?
		target_link_libraries(${TEST} OpenApoc_Library OpenApoc_Framework
				OpenApoc_GameState)
		target_compile_definitions(${TEST} PRIVATE -DUNIT_TEST)
		add_test(NAME ${TEST} COMMAND ${TEST}
				--Framework.CD=${CD_PATH} --Framework.Data=${CMAKE_SOURCE_DIR}/data)
endforeach()

# test_serialize requires args so needs to be slightly separate
set(TEST test_serialize)
add_executable(${TEST} ${TEST}.cpp)
# Just add all libs?
target_link_libraries(${TEST} OpenApoc_Library OpenApoc_Framework
		OpenApoc_GameState)
target_compile_definitions(${TEST} PRIVATE -DUNIT_TEST)
add_test(NAME ${TEST}_difficulty0 COMMAND ${TEST}
		${CMAKE_SOURCE_DIR}/data/mods/base/data/submods/org.openapoc.base/difficulty0
		${CMAKE_SOURCE_DIR}/data/mods/base/base_gamestate
		--Framework.CD=${CD_PATH} --Framework.Data=${CMAKE_SOURCE_DIR}/data)

# test_weekly_funding requires args (same as test_serialize)
set(TEST test_weekly_funding)
add_executable(${TEST} ${TEST}.cpp)
target_link_libraries(${TEST} OpenApoc_Library OpenApoc_Framework
		OpenApoc_GameState)
target_compile_definitions(${TEST} PRIVATE -DUNIT_TEST)
add_test(NAME ${TEST} COMMAND ${TEST}
		${CMAKE_SOURCE_DIR}/data/mods/base/data/submods/org.openapoc.base/difficulty0
		${CMAKE_SOURCE_DIR}/data/mods/base/base_gamestate
		--Framework.CD=${CD_PATH} --Framework.Data=${CMAKE_SOURCE_DIR}/data)

# test_org_tech_level requires args (same as test_serialize)
set(TEST test_org_tech_level)
add_executable(${TEST} ${TEST}.cpp)
target_link_libraries(${TEST} OpenApoc_Library OpenApoc_Framework
		OpenApoc_GameState)
target_compile_definitions(${TEST} PRIVATE -DUNIT_TEST)
add_test(NAME ${TEST} COMMAND ${TEST}
		${CMAKE_SOURCE_DIR}/data/mods/base/data/submods/org.openapoc.base/difficulty0
		${CMAKE_SOURCE_DIR}/data/mods/base/base_gamestate
		--Framework.CD=${CD_PATH} --Framework.Data=${CMAKE_SOURCE_DIR}/data)

# test_mods verifies the content mods load, link and coexist
set(TEST test_mods)
add_executable(${TEST} ${TEST}.cpp)
target_link_libraries(${TEST} OpenApoc_Library OpenApoc_Framework
		OpenApoc_GameState)
target_compile_definitions(${TEST} PRIVATE -DUNIT_TEST)
add_test(NAME ${TEST} COMMAND ${TEST}
		${CMAKE_SOURCE_DIR}/data/mods/base/data/submods/org.openapoc.base/difficulty0
		${CMAKE_SOURCE_DIR}/data/mods/base/base_gamestate
		${CMAKE_SOURCE_DIR}/data/mods/modern_weapons/modern_weapons_gamestate
		${CMAKE_SOURCE_DIR}/data/mods/agent_voices/agent_voices_gamestate
		--Game.Mods=base:modern_weapons:agent_voices
		--Game.ModPath=${CMAKE_SOURCE_DIR}/data/mods
		--Framework.CD=${CD_PATH} --Framework.Data=${CMAKE_SOURCE_DIR}/data)

# test_lab_assignment requires args (same as test_serialize)
set(TEST test_lab_assignment)
add_executable(${TEST} ${TEST}.cpp)
target_link_libraries(${TEST} OpenApoc_Library OpenApoc_Framework
		OpenApoc_GameState)
target_compile_definitions(${TEST} PRIVATE -DUNIT_TEST)
add_test(NAME ${TEST} COMMAND ${TEST}
		${CMAKE_SOURCE_DIR}/data/mods/base/data/submods/org.openapoc.base/difficulty0
		${CMAKE_SOURCE_DIR}/data/mods/base/base_gamestate
		--Framework.CD=${CD_PATH} --Framework.Data=${CMAKE_SOURCE_DIR}/data)
