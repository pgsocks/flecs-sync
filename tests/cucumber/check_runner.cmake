if(NOT EXISTS "${RUBY}" OR NOT EXISTS "${BUNDLE}")
	message(FATAL_ERROR "Runner checks require Ruby and Bundler 2.6.9; "
		"run bundle _2.6.9_ install in Sync/.")
endif()
execute_process(COMMAND "${RUBY}" "${SOURCE}/tests/cucumber/check_runner.rb"
	"${BUNDLE}" "${SOURCE}" RESULT_VARIABLE result TIMEOUT 45)
if(NOT result EQUAL 0)
	message(FATAL_ERROR "Cucumber failure detection check failed: ${result}")
endif()
