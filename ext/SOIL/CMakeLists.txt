add_library(SOIL
	src/SOIL.c
	src/image_DXT.c
	src/image_helper.c
	src/stb_image_aug.c
)

target_compile_options(SOIL
	PUBLIC 
		"$<$<CONFIG:DEBUG>:${PROJECT_DEBUG_OPTIONS}>"
)

target_compile_options(SOIL
	PUBLIC 
		"$<$<CONFIG:RELEASE>:${PROJECT_RELEASE_OPTIONS}>"
)