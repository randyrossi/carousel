cmake_minimum_required(VERSION 2.6)
project(Carousel)

set(CMAKE_BUILD_TYPE,Debug)

# Use our modified FindSDL2* modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${Carousel_SOURCE_DIR}/cmake")
# Set an output directory for our binaries
set(BIN_DIR ${Carousel_SOURCE_DIR}/bin)

# Bump up warning levels appropriately for clang, gcc & msvc
# Also set debug/optimization flags depending on the build type. IDE users choose this when
# selecting the build mode in their IDE
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} -g")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} -O2")
else()
  message(FATAL_ERROR "Unsupported compiler, CMake will exit." )
endif()

find_package(ALSA)
find_package(SDL2 REQUIRED)
find_package(LibConfig REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

if(ALSA_FOUND)
  add_definitions(-DALSA_FOUND=1)
endif()

add_executable(Carousel src/main.cpp src/res_path.cpp src/res_path.h src/carousel.cpp src/carousel.h src/audio.cpp src/audio.h)
target_link_libraries(Carousel ${ALSA_LIBRARY} ${SDL2_LIBRARY} ${LIBCONFIG_LIBRARY})
install(TARGETS Carousel RUNTIME DESTINATION ${BIN_DIR})
install(FILES carousel.cfg DESTINATION ${BIN_DIR})
install(PROGRAMS carousel.sh DESTINATION ${BIN_DIR})
