# Project Leary #
More than anything, Leary is my sandbox for experimentation. It started out as a
way of learning the more modern OpenGL 4+ and focusing on various data oriented
techniques and C philosophies. Since then, with the release of the Vulkan API,
I’ve used it as a way test and learn Vulkan.

Aside from learning more advanced graphics API, I’m taking the handmade approach
of building everything entirely from scratch, with no libraries what so ever, in
order to properly learn the various platforms’ API, quirks, and capabilities.

Currently there is one exception to this; the stb libraries are used for loading
and rasterising TTF fonts. I have future plans to replace this, either with
something platform-dependent using the platform APIs, or my entirely own
solution (if I'm feeling crazy enough one day).

## Dependencies ##
* Vulkan drivers
* Vulkan SDK (for validation layers)
* glslang (for building SPIR-V shaders from glsl)
* git-lfs (https://git-lfs.github.com/) (for binary asset files)
..* This needs to be installed before the repository is cloned

## Building ##
### Windows ###
The project is developed on Windows using the MSVC compiler from Visual Studio
2015. The build script assumes that the Visual Studio binary paths are set in
your user and/or shell environment.

1. Install appropriate graphics drivers that support Vulkan
1. Install the Vulkan SDK (https://www.lunarg.com/vulkan-sdk) and create a new
environment variable named VULKAN_LOCATION and point it to the root of the SDK
installation
1. Install glslang (https://github.com/KhronosGroup/glslang) and make its
glslangValidator binary available in your PATH
1. Execute build.bat

### Linux ###
The project is developed on Linux using the Clang 3.9.1 compiler.

1. Install appropriate graphics drivers that support Vulkan
1. Install the Vulkan SDK (https://www.lunarg.com/vulkan-sdk)
1. Install glslang (https://github.com/KhronosGroup/glslang) and make its
glslangValidator binary available in your PATH
1. Execute build.sh

## Running ##
The build scripts creates a build folder in the root of the script directory,
into which it builds the code and copies the assets to. The game should be able
to be run from any directory, though the testing has been minimal.

The engine creates a new folder in your user data folder ($XDG_DATA_HOME/leary
or ~/.local/share/leary on Linux and LOCAL_APPDATA on Windows) into which it
writes any engine-writable files, e.g. settings.



