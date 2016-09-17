# Try to find XCB library and include path.
# Once done this will define
#
# XCB_FOUND
# XCB_INCLUDE_DIR
# XCB_LIBRARY

include(FindPackageHandleStandardArgs)

if (${CMAKE_HOST_UNIX})
	find_path( XCB_INCLUDE_DIR
		NAMES
			xcb/xcb.h
		PATHS
			${XCB_LOCATION}/include
			$ENV{XCB_LOCATION}/include
			/usr/include
			/usr/local/include
			NO_DEFAULT_PATH
		DOC "The directory where vulkan.h resides"
	)
	find_library( XCB_LIBRARY
	NAMES
		xcb
	PATHS
		${XCB_LOCATION}/lib
		$ENV{XCB_LOCATION}/lib
		/usr/lib64
		/usr/lib
		/usr/local/lib64
		/usr/local/lib
		/sw/lib
		/opt/local/lib
		/usr/lib/x86_64-linux-gnu
		NO_DEFAULT_PATH
	DOC "The XCB library")
endif ()

find_package_handle_standard_args(XCB DEFAULT_MSG
	XCB_INCLUDE_DIR
	XCB_LIBRARY
)

mark_as_advanced( XCB_FOUND )

