if(NOT EXISTS "${RUBY}")
	message(FATAL_ERROR "Cucumber unavailable: install Ruby >=3.2, "
		"RubyGems >=3.2.8 and Bundler, then bundle install in Sync/.")
endif()
if(NOT EXISTS "${BUNDLE}")
	message(FATAL_ERROR "Cucumber unavailable: Bundler missing. "
		"Install Bundler 2.6.9 and run bundle install in Sync/.")
endif()
string(CONCAT ruby_check
	"v=Gem::Version; "
	"exit(v.new(RUBY_VERSION)>=v.new('3.2') && "
	"Gem.rubygems_version>=v.new('3.2.8') ? 0 : 1)")
execute_process(COMMAND "${RUBY}" -e "${ruby_check}"
	RESULT_VARIABLE ruby_result)
if(NOT ruby_result EQUAL 0)
	message(FATAL_ERROR "Cucumber needs Ruby >=3.2 and RubyGems >=3.2.8")
endif()
execute_process(COMMAND "${BUNDLE}" --version
	RESULT_VARIABLE bundle_result OUTPUT_VARIABLE bundle_version)
string(STRIP "${bundle_version}" bundle_version)
if(NOT bundle_result EQUAL 0 OR
	NOT bundle_version STREQUAL "Bundler version 2.6.9")
	message(FATAL_ERROR "Install Bundler 2.6.9: gem install bundler -v 2.6.9")
endif()
set(env "BUNDLE_GEMFILE=${SOURCE}/Gemfile" "BUNDLE_FROZEN=true"
	"SYNC_FIXTURE=${FIXTURE}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env ${env}
	"${BUNDLE}" exec cucumber --version WORKING_DIRECTORY "${SOURCE}"
	RESULT_VARIABLE tools_result OUTPUT_VARIABLE version
	ERROR_VARIABLE error)
string(STRIP "${version}" version)
if(NOT tools_result EQUAL 0 OR NOT version STREQUAL "11.1.1")
	message(FATAL_ERROR "Cucumber 11.1.1 unavailable: run bundle install "
		"using the locked Sync/Gemfile. ${error}")
endif()
if(GROUP STREQUAL "loopback")
	set(tags "@loopback")
	set(feature_dir enet_sync)
else()
	set(tags "@${GROUP}")
	set(feature_dir "${GROUP}")
endif()
if(NOT FILE_SYNC)
	set(tags "(${tags}) and not @file_sync")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E env ${env}
	"${BUNDLE}" exec cucumber --publish-quiet --strict --format progress
	--tags "${tags}" "features/${feature_dir}"
	--require features/support --require features/step_definitions
	WORKING_DIRECTORY "${SOURCE}"
	RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
message("${output}${error}")
string(REGEX MATCH "([0-9]+) scenarios" count "${output}")
if(NOT CMAKE_MATCH_1 STREQUAL "${EXPECTED_SCENARIOS}")
	message(FATAL_ERROR "Expected ${EXPECTED_SCENARIOS} scenarios in "
		"${GROUP}; runner reported ${count}")
endif()
if(NOT result EQUAL 0)
	message(FATAL_ERROR "Cucumber ${GROUP} failed (exit ${result}); "
		"See fixture diagnostics and assertions above.")
endif()
