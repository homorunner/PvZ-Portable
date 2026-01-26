cmake_minimum_required(VERSION 3.5...4.0)

project(pvz-portable LANGUAGES C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (CMAKE_GENERATOR MATCHES "Ninja")
	set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
endif()

file(GLOB SOURCES
	src/*.cpp
	src/SexyAppFramework/*.cpp
	src/SexyAppFramework/graphics/*.cpp
	src/SexyAppFramework/imagelib/*.cpp
	src/SexyAppFramework/misc/*.cpp
	src/SexyAppFramework/paklib/*.cpp
	src/SexyAppFramework/sound/*.cpp
	src/SexyAppFramework/widget/*.cpp
	#src/SexyAppFramework/ogg/*.c
	src/SexyAppFramework/fcaseopen/*.c
	src/Sexy.TodLib/*.cpp
	src/Lawn/*.cpp
	src/Lawn/Widget/*.cpp
	src/Lawn/System/*.cpp
	#src/SexyAppFramework/LawnProject.rc
)

# these files are included and must not be built in the project
list(REMOVE_ITEM SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/Sexy.TodLib/TodDrawTriangle.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Sexy.TodLib/TodDrawTriangleInc.cpp
)

set(NO_GLEW OFF CACHE BOOL "Disable GLEW")
set(LOW_MEMORY OFF CACHE BOOL "Enable low memory mode")
set(DOWNSCALE_COUNT 1 CACHE STRING "Image downscale count")
if(NINTENDO_SWITCH)
	set(NO_GLEW ON)
	set(VORBIS_LIB vorbisfile vorbis)
	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/switch/graphics/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/switch/graphics/GLInterface.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/switch/Input.cpp
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/switch
	)
elseif(NINTENDO_3DS)
	set(NO_GLEW ON)
	set(LOW_MEMORY ON)
	set(DOWNSCALE_COUNT 2)
	ctr_add_shader_library(colored ${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/3ds/graphics/colored.v.pica)
	ctr_add_shader_library(textured ${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/3ds/graphics/textured.v.pica)
	dkp_add_embedded_binary_library(shaders colored textured)
	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/3ds/graphics/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/3ds/graphics/GLInterface.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/3ds/Input.cpp
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/3ds
	)
else()
	if(MSVC)
		find_package(Vorbis CONFIG REQUIRED)
		find_package(Ogg CONFIG REQUIRED)
		find_package(libopenmpt CONFIG REQUIRED)
		find_package(mpg123 CONFIG REQUIRED)
		set(VORBIS_LIB Vorbis::vorbis Vorbis::vorbisfile)
		set(OGG_LIB Ogg::ogg)
		set(OPENMPT_LIB libopenmpt::libopenmpt)
		set(MPG123_LIB MPG123::libmpg123)
	else()
		find_library(OGG_LIBRARY NAMES ogg libogg REQUIRED)
		find_library(VORBIS_LIBRARY NAMES vorbis libvorbis REQUIRED)
		find_library(VORBISFILE_LIBRARY NAMES vorbisfile libvorbisfile REQUIRED)
		find_library(OPENMPT_LIBRARY NAMES openmpt libopenmpt REQUIRED)
		find_library(MPG123_LIBRARY NAMES mpg123 libmpg123 REQUIRED)

		set(VORBIS_LIB ${VORBISFILE_LIBRARY} ${VORBIS_LIBRARY})
		set(OGG_LIB ${OGG_LIBRARY})
		set(OPENMPT_LIB ${OPENMPT_LIBRARY})
		set(MPG123_LIB ${MPG123_LIBRARY})
	endif()
	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/pc/graphics/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/pc/graphics/GLInterface.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/pc/Input.cpp
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/pc
	)
endif()

if(CMAKE_BUILD_TYPE AND CMAKE_BUILD_TYPE MATCHES "^[Dd][Ee][Bb][Uu][Gg]$")
	set(WIN_CONSOLE_DEFAULT ON)
else()
	set(WIN_CONSOLE_DEFAULT OFF)
endif()

option(PVZ_DEBUG "Enable _PVZ_DEBUG macro to enable cheats, hidden features, etc." ON)
option(LIMBO_PAGE "Enable limbo page to access hidden levels" ON)
option(CONSOLE "Show console on Windows" ${WIN_CONSOLE_DEFAULT})
option(DO_FIX_BUGS "Define DO_FIX_BUGS macro (Community fixes for original game bugs of 1.2.0.1073 GOTY Edition)" OFF)

set(OpenGL_GL_PREFERENCE GLVND)

find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
if (NOT NO_GLEW)
	find_package(GLEW REQUIRED)
endif()
if(MSVC)
	find_package(PThreads4W CONFIG REQUIRED)
	find_path(DIRENT_INCLUDE_DIRS "dirent.h" REQUIRED)
endif()

set(SDL_MIXER_X_STATIC ON CACHE BOOL "Build static library of SDL Mixer X")
set(SDL_MIXER_X_SHARED OFF CACHE BOOL "Build shared library of SDL Mixer X")
set(USE_MIDI OFF CACHE BOOL "SDL Mixer X midi")
set(USE_MODPLUG OFF CACHE BOOL "SDL Mixer X modplug")
set(USE_XMP OFF CACHE BOOL "SDL Mixer X xmp")
set(USE_OPUS OFF CACHE BOOL "SDL Mixer X opus")
set(USE_DRFLAC OFF CACHE BOOL "SDL Mixer X flac")
set(USE_PXTONE OFF CACHE BOOL "SDL Mixer X pxtone")
add_subdirectory(src/SexyAppFramework/sound/SDL-Mixer-X)

add_executable(pvz-portable ${SOURCES})
target_include_directories(pvz-portable PRIVATE
	${PROJECT_SOURCE_DIR}/src
	${PROJECT_SOURCE_DIR}/src/SexyAppFramework
	${PROJECT_SOURCE_DIR}/src/SexyAppFramework/sound/SDL-Mixer-X/include
	${SDL2_INCLUDE_DIRS}
	${PLAT_INCLUDES}
)
if(MSVC)
	target_include_directories(pvz-portable PRIVATE ${DIRENT_INCLUDE_DIRS})
endif()
target_compile_definitions(pvz-portable PRIVATE
	$<$<BOOL:${PVZ_DEBUG}>:_PVZ_DEBUG>
	$<$<BOOL:${LIMBO_PAGE}>:_PVZ_LIMBO_PAGE>
	$<$<BOOL:${DO_FIX_BUGS}>:DO_FIX_BUGS>
)
target_compile_features(pvz-portable PRIVATE cxx_std_20)

if (LOW_MEMORY)
	target_compile_definitions(pvz-portable PRIVATE LOW_MEMORY)
endif()
target_compile_definitions(pvz-portable PRIVATE IMG_DOWNSCALE=${DOWNSCALE_COUNT})

target_link_libraries(pvz-portable PRIVATE
	SDL2_mixer_ext_Static 
	${OPENMPT_LIB} 
	${MPG123_LIB} 
	${VORBIS_LIB} 
	${OGG_LIB} 
	PNG::PNG
	JPEG::JPEG
	ZLIB::ZLIB
	SDL2::SDL2main
	SDL2::SDL2
)

if (WIN32)
	if(MSVC)
		target_compile_options(pvz-portable PRIVATE /utf-8)
		target_link_libraries(pvz-portable PRIVATE PThreads4W::PThreads4W)
		target_compile_definitions(pvz-portable PRIVATE GLEW_STATIC)
		set_property(TARGET pvz-portable PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	else()
		target_link_options(pvz-portable PRIVATE -static -static-libgcc -static-libstdc++)
	endif()
	
	target_link_libraries(pvz-portable PRIVATE OpenGL::GL ws2_32 user32 gdi32 winmm imm32 shlwapi)
	target_compile_definitions(pvz-portable PRIVATE WINDOWS)

	if(NOT CONSOLE)
		if(MSVC)
			set_target_properties(pvz-portable PROPERTIES WIN32_EXECUTABLE TRUE)
		else()
			target_link_options(pvz-portable PRIVATE -mwindows)
		endif()
	endif()
elseif (NINTENDO_SWITCH)
	target_link_libraries(pvz-portable PRIVATE glad EGL glapi drm_nouveau)

	nx_generate_nacp(pvz-portable.nacp
		NAME "Plants vs. Zombies"
		AUTHOR "PopCap Games, unofficial Switch port by the community"
		VERSION "0.1"
	)

	nx_create_nro(pvz-portable
		NACP pvz-portable.nacp
		ICON ${CMAKE_CURRENT_SOURCE_DIR}/icon-switch.jpg
	)
elseif (NINTENDO_3DS)
	target_link_libraries(pvz-portable PRIVATE citro3d shaders)

	ctr_generate_smdh(pvz-portable.smdh
		NAME "Plants vs. Zombies"
		DESCRIPTION "GOTY Edition"
		AUTHOR "PopCap Games, unofficial 3DS port by the community"
		VERSION "0.1"
		ICON "${CMAKE_CURRENT_SOURCE_DIR}/icon-3ds.png"
	)

	ctr_create_3dsx(pvz-portable
		SMDH pvz-portable.smdh
	)
else()
	target_link_libraries(pvz-portable PRIVATE OpenGL::GL)
endif()

if (NOT NO_GLEW)
	target_link_libraries(pvz-portable PRIVATE GLEW::GLEW)
endif()
