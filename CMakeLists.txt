cmake_minimum_required(VERSION 3.24)
project(MySDL3App C)

include(FetchContent)

# Fetch SDL3
FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.2.16
)
FetchContent_MakeAvailable(SDL3)

# Collect all source files in src/
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.c)

# Add your executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(app ${SRC_FILES})

# Link with SDL3
target_link_libraries(app PRIVATE SDL3::SDL3)

if(WIN32)
    add_custom_command(
        TARGET app POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:app>
        VERBATIM
    )
endif()
