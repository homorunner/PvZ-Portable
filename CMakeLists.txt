cmake_minimum_required(VERSION 3.5)

project(re-plants-vs-zombies)

file(GLOB SOURCES
	*.cpp
	SexyAppFramework/*.cpp
	SexyAppFramework/graphics/*.cpp
	SexyAppFramework/imagelib/*.cpp
	SexyAppFramework/misc/*.cpp
	SexyAppFramework/paklib/*.cpp
	SexyAppFramework/sound/*.cpp
	SexyAppFramework/widget/*.cpp
	SexyAppFramework/ogg/*.c
	Sexy.TodLib/*.cpp
	Lawn/*.cpp
	Lawn/Widget/*.cpp
	Lawn/System/*.cpp
	ImageLib/*.cpp
	PakLib/*.cpp
	SexyAppFramework/LawnProject.rc
)

# these files are included and must not be built in the project
list(REMOVE_ITEM SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/Sexy.TodLib/TodDrawTriangle.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Sexy.TodLib/TodDrawTriangleInc.cpp
)


option(CONSOLE "Show console on Windows" ON)

find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)

add_executable(re-plants-vs-zombies ${SOURCES})
target_include_directories(re-plants-vs-zombies PRIVATE ${PROJECT_SOURCE_DIR}/SexyAppFramework)
target_compile_definitions(re-plants-vs-zombies PRIVATE _DEBUG)

target_link_libraries(re-plants-vs-zombies jpeg png z)
#target_link_options(re-plants-vs-zombies PRIVATE -static-libstdc++)

if (WIN32)
	target_link_libraries(re-plants-vs-zombies ws2_32 ddraw dinput8 dxguid dxerr8 user32 gdi32 winmm)
	target_compile_definitions(re-plants-vs-zombies PRIVATE WINDOWS)

	if(NOT CONSOLE)
		target_link_options(re-plants-vs-zombies PRIVATE -mwindows)
	endif()
endif()
