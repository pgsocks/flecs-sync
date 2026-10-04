if(NOT EXISTS "${RUBY}")
	message(FATAL_ERROR "Protocol checks require Ruby: install Ruby >=3.2")
endif()
execute_process(COMMAND "${RUBY}" "${SOURCE}/tests/cucumber/check_protocol.rb"
	"${FIXTURE}" "${SOURCE}" RESULT_VARIABLE result TIMEOUT 20)
if(NOT result EQUAL 0)
	message(FATAL_ERROR "Fixture protocol check failed: ${result}")
endif()
