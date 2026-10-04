set(file_test_sources test.c "${PROJECT_SOURCE_DIR}/tests/fixture.c"
	"${PROJECT_SOURCE_DIR}/tests/assert.c")
add_executable(file_sync_tests ${file_test_sources})
target_include_directories(file_sync_tests PRIVATE
	"${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(file_sync_tests PRIVATE FileSync::file_sync)
sync_warnings(file_sync_tests)

add_library(file_sync_instrumented STATIC ../src/file_sync.c test_controls.c)
target_compile_definitions(file_sync_instrumented PRIVATE FILE_SYNC_TESTING)
target_include_directories(file_sync_instrumented PUBLIC ../include
	PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(file_sync_instrumented PUBLIC Sync::sync)
sync_warnings(file_sync_instrumented)
add_executable(file_sync_fault_tests ${file_test_sources})
target_compile_definitions(file_sync_fault_tests PRIVATE FILE_SYNC_TESTING)
target_include_directories(file_sync_fault_tests PRIVATE
	"${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(file_sync_fault_tests PRIVATE file_sync_instrumented)
sync_warnings(file_sync_fault_tests)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/cases.def"
	"${PROJECT_SOURCE_DIR}/features/file_sync/file_sync.feature")
file(STRINGS cases.def cases REGEX "^CASE\\([a-z0-9_]+\\)$")
foreach(case IN LISTS cases)
	string(REGEX REPLACE "^CASE\\((.*)\\)$" "\\1" name "${case}")
	list(APPEND case_names "${name}")
	if(name MATCHES "^file_fault_")
		set(runner file_sync_fault_tests)
	else()
		set(runner file_sync_tests)
	endif()
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/cases/${name}")
	add_test(NAME "file_sync.${name}" COMMAND ${runner} "${name}")
	set_tests_properties("file_sync.${name}" PROPERTIES LABELS contract
		TIMEOUT 120 WORKING_DIRECTORY
		"${CMAKE_CURRENT_BINARY_DIR}/cases/${name}")
endforeach()
file(STRINGS
	"${PROJECT_SOURCE_DIR}/features/file_sync/file_sync.feature"
	scenarios REGEX "Scenario: [a-z0-9_]+$")
foreach(scenario IN LISTS scenarios)
	string(REGEX REPLACE ".*Scenario: " "" name "${scenario}")
	list(APPEND feature_names "${name}")
endforeach()
list(SORT case_names)
list(SORT feature_names)
if(NOT "${case_names}" STREQUAL "${feature_names}")
	message(FATAL_ERROR "FileSync features and case registry differ")
endif()

add_subdirectory(rpg)

add_executable(file_sync_link_c link.c)
target_link_libraries(file_sync_link_c PRIVATE FileSync::file_sync)
sync_warnings(file_sync_link_c)
add_test(NAME file_sync.link_c COMMAND file_sync_link_c)
set_tests_properties(file_sync.link_c PROPERTIES LABELS infrastructure)
if(SYNC_CHECK_CXX)
	add_executable(file_sync_link_cxx link.cpp)
	target_compile_features(file_sync_link_cxx PRIVATE cxx_std_17)
	target_link_libraries(file_sync_link_cxx PRIVATE FileSync::file_sync)
	sync_warnings(file_sync_link_cxx)
	add_test(NAME file_sync.link_cxx COMMAND file_sync_link_cxx)
	set_tests_properties(file_sync.link_cxx PROPERTIES
		LABELS infrastructure)
endif()
