# Try to find Vulkan library and include path.
# Once done this will define
#
# VULKAN_FOUND
# VULKAN_INCLUDE_DIR
# VULKAN_LIBRARY

include(FindPackageHandleStandardArgs)

if (${CMAKE_HOST_UNIX})
	find_path( VULKAN_INCLUDE_DIR
		NAMES
			vulkan/vulkan.h
		PATHS
			${VULKAN_LOCATION}/include
			$ENV{VULKAN_LOCATION}/include
			/usr/include
			/usr/local/include
			NO_DEFAULT_PATH
		DOC "The directory where vulkan.h resides"
	)
	find_library( VULKAN_LIBRARY
	NAMES
		vulkan
	PATHS
		${VULKAN_LOCATION}/lib
		$ENV{VULKAN_LOCATION}/lib
		/usr/lib64
		/usr/lib
		/usr/local/lib64
		/usr/local/lib
		/sw/lib
		/opt/local/lib
		/usr/lib/x86_64-linux-gnu
		NO_DEFAULT_PATH
	DOC "The VULKAN library")
endif ()

if (VULKAN_LIBRARY AND EXISTS ${VULKAN_LIBRARY})
	mark_as_advanced( VULKAN_FOUND )
endif()


find_package_handle_standard_args(VULKAN DEFAULT_MSG
	VULKAN_INCLUDE_DIR
	VULKAN_LIBRARY
)

