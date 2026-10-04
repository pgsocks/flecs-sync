include("${CMAKE_CURRENT_LIST_DIR}/CPM.cmake")
if(SYNC_BUILD_ENET_SYNC)
	CPMAddPackage(NAME flecs_enet
		GITHUB_REPOSITORY pgsocks/flecs-enet
		GIT_TAG 8b00bf290c7eb5d17c3b6b5858b7086405a224d0
		EXCLUDE_FROM_ALL YES
		OPTIONS "BUILD_TESTING OFF" "FLECS_ENET_CUCUMBER OFF")
	add_library(sync_flecs ALIAS flecs_enet_flecs)
else()
	CPMAddPackage(NAME flecs
		GITHUB_REPOSITORY SanderMertens/flecs
		GIT_TAG v4.1.1
		DOWNLOAD_ONLY YES)
	# Build the upstream distribution, with its default addons enabled.
	add_library(sync_flecs STATIC "${flecs_SOURCE_DIR}/distr/flecs.c")
	target_include_directories(sync_flecs SYSTEM PUBLIC
		"${flecs_SOURCE_DIR}/distr")
	set_target_properties(sync_flecs PROPERTIES C_EXTENSIONS ON)
	if(WIN32)
		target_link_libraries(sync_flecs PUBLIC ws2_32)
	elseif(UNIX)
		find_package(Threads REQUIRED)
		target_link_libraries(sync_flecs PUBLIC Threads::Threads m)
	endif()
endif()
