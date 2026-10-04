find_program(SYNC_RUBY NAMES ruby ruby32 ruby33 ruby34)
find_program(SYNC_BUNDLE NAMES bundle bundler)
if(NOT SYNC_RUBY OR NOT SYNC_BUNDLE)
	message(WARNING "Cucumber requires Ruby >=3.2 and Bundler 2.6.9. "
		"Tests remain registered and report missing tools.")
endif()
set(cucumber_dir "${PROJECT_SOURCE_DIR}/tests/cucumber")
add_executable(sync_cucumber_fixture
	${cucumber_dir}/core_driver.c ${cucumber_dir}/core_module.c
	${cucumber_dir}/core_replication.c ${cucumber_dir}/core_stream.c
	${cucumber_dir}/core_schema.c ${cucumber_dir}/core_automatic.c
	${cucumber_dir}/core_fault.c ${cucumber_dir}/protocol.c
	${PROJECT_SOURCE_DIR}/tests/fixture.c
	${PROJECT_SOURCE_DIR}/tests/assert.c)
target_compile_definitions(sync_cucumber_fixture PRIVATE SYNC_TESTING)
target_include_directories(sync_cucumber_fixture PRIVATE tests)
target_link_libraries(sync_cucumber_fixture PRIVATE sync_instrumented)
sync_warnings(sync_cucumber_fixture)

set(cucumber_groups core)
if(SYNC_BUILD_FILE_SYNC)
	add_executable(file_sync_cucumber_fixture
		${cucumber_dir}/file_sync_driver.c
		${cucumber_dir}/file_sync_test.c ${cucumber_dir}/protocol.c
		${PROJECT_SOURCE_DIR}/tests/fixture.c
		${PROJECT_SOURCE_DIR}/tests/assert.c)
	target_compile_definitions(file_sync_cucumber_fixture PRIVATE
		FILE_SYNC_TESTING)
	target_include_directories(file_sync_cucumber_fixture PRIVATE
		tests adapters/file_sync/tests)
	target_link_libraries(file_sync_cucumber_fixture PRIVATE
		file_sync_instrumented)
	sync_warnings(file_sync_cucumber_fixture)
	add_executable(rpg_cucumber_fixture ${cucumber_dir}/rpg_driver.c
		${cucumber_dir}/rpg_recording.c ${cucumber_dir}/rpg_gameplay.c
		${cucumber_dir}/rpg_replay.c ${cucumber_dir}/rpg_automatic.c
		${cucumber_dir}/rpg_failures.c ${cucumber_dir}/protocol.c
		${PROJECT_SOURCE_DIR}/adapters/file_sync/tests/rpg/model.c
		${cucumber_dir}/rpg_observations.c)
	target_compile_definitions(rpg_cucumber_fixture PRIVATE FILE_SYNC_TESTING)
	target_include_directories(rpg_cucumber_fixture PRIVATE
		adapters/file_sync/tests/rpg adapters/file_sync/tests)
	target_link_libraries(rpg_cucumber_fixture PRIVATE file_sync_instrumented)
	sync_warnings(rpg_cucumber_fixture)
	list(APPEND cucumber_groups file_sync rpg)
endif()
if(SYNC_BUILD_ENET_SYNC)
	foreach(target IN ITEMS enet_sync_cucumber_fixture
		enet_sync_udp_cucumber_fixture)
		add_executable(${target} ${cucumber_dir}/enet_sync_contract.c
			${cucumber_dir}/protocol.c
			${PROJECT_SOURCE_DIR}/adapters/enet_sync/tests/model.c)
		target_include_directories(${target} PRIVATE
			adapters/enet_sync/tests)
		if(target STREQUAL enet_sync_cucumber_fixture)
			target_compile_definitions(${target} PRIVATE ENET_SYNC_TESTING)
			target_link_libraries(${target} PRIVATE enet_sync_instrumented)
		else()
			target_link_libraries(${target} PRIVATE EnetSync::enet_sync)
		endif()
		if(SYNC_BUILD_FILE_SYNC)
			target_compile_definitions(${target} PRIVATE ES_FILE_SYNC)
			target_link_libraries(${target} PRIVATE FileSync::file_sync)
		endif()
		sync_warnings(${target})
	endforeach()
	list(APPEND cucumber_groups enet_sync loopback)
endif()
# Compare the runner's executed count with the complete native registry.
file(STRINGS "${PROJECT_SOURCE_DIR}/tests/cases.def" core_registry
	REGEX "^CASE\\(")
list(LENGTH core_registry core_scenarios)
if(SYNC_BUILD_FILE_SYNC)
	file(STRINGS "${PROJECT_SOURCE_DIR}/adapters/file_sync/tests/cases.def"
		file_registry REGEX "^CASE\\(")
	file(STRINGS "${PROJECT_SOURCE_DIR}/adapters/file_sync/tests/rpg/cases.def"
		rpg_registry REGEX "^CASE\\(")
	list(LENGTH file_registry file_sync_scenarios)
	list(LENGTH rpg_registry rpg_scenarios)
endif()
if(SYNC_BUILD_ENET_SYNC)
	file(STRINGS "${PROJECT_SOURCE_DIR}/adapters/enet_sync/tests/cases.def"
		enet_registry REGEX "^CASE\\(")
	if(NOT SYNC_BUILD_FILE_SYNC)
		list(FILTER enet_registry EXCLUDE REGEX "^CASE\\(recording_")
	endif()
	list(LENGTH enet_registry enet_sync_scenarios)
	set(loopback_scenarios 8)
	if(NOT SYNC_BUILD_FILE_SYNC)
		set(loopback_scenarios 7)
	endif()
endif()
foreach(group IN LISTS cucumber_groups)
	set(fixture "${group}_cucumber_fixture")
	if(group STREQUAL core)
		set(fixture sync_cucumber_fixture)
	elseif(group STREQUAL loopback)
		set(fixture enet_sync_udp_cucumber_fixture)
	endif()
	add_test(NAME sync.cucumber_${group} COMMAND ${CMAKE_COMMAND}
		-DRUBY=${SYNC_RUBY} -DBUNDLE=${SYNC_BUNDLE}
		-DFIXTURE=$<TARGET_FILE:${fixture}>
		-DSOURCE=${PROJECT_SOURCE_DIR} -DGROUP=${group}
		-DFILE_SYNC=${SYNC_BUILD_FILE_SYNC}
		-DEXPECTED_SCENARIOS=${${group}_scenarios}
		-P ${cucumber_dir}/run.cmake)
	set(labels "cucumber;${group}")
	if(group STREQUAL loopback)
		list(APPEND labels enet_sync)
	else()
		list(APPEND labels contract)
	endif()
	set_tests_properties(sync.cucumber_${group} PROPERTIES
		LABELS "${labels}" TIMEOUT 120)
endforeach()
add_test(NAME sync.cucumber_protocol COMMAND ${CMAKE_COMMAND}
	-DRUBY=${SYNC_RUBY} -DFIXTURE=$<TARGET_FILE:sync_cucumber_fixture>
	-DSOURCE=${PROJECT_SOURCE_DIR}
	-P ${cucumber_dir}/check_protocol.cmake)
set_tests_properties(sync.cucumber_protocol PROPERTIES
	LABELS "cucumber;infrastructure" TIMEOUT 30)
add_test(NAME sync.cucumber_runner COMMAND ${CMAKE_COMMAND}
	-DRUBY=${SYNC_RUBY} -DBUNDLE=${SYNC_BUNDLE}
	-DSOURCE=${PROJECT_SOURCE_DIR}
	-P ${cucumber_dir}/check_runner.cmake)
set_tests_properties(sync.cucumber_runner PROPERTIES
	LABELS "cucumber;infrastructure" TIMEOUT 60)
