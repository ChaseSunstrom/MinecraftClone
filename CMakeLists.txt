cmake_minimum_required(VERSION 3.12)
project(MinecraftClone C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set the source files
set(SOURCES src/main.c
        # Add other source files here
)

include(FetchContent)

# Fetch and configure GLFW
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.8)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Fetch and configure GLAD
FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG v0.1.36)

FetchContent_GetProperties(glad)
if(NOT glad_POPULATED)
  FetchContent_Populate(glad)

  set(GLAD_PROFILE "core" CACHE STRING "OpenGL profile")
  set(GLAD_API "gl=3.3" CACHE STRING "API type/version pairs, like \"gl=3.2,gles=\", no version means latest")
  set(GLAD_GENERATOR "c" CACHE STRING "Language to generate the binding for")

  # Add GLAD subdirectory
  add_subdirectory(${glad_SOURCE_DIR} ${glad_BINARY_DIR})
endif()

# Ensure GLAD target is created
if(NOT TARGET glad)
  message(FATAL_ERROR "GLAD target not created")
endif()

# Get GLAD include directories
get_target_property(GLAD_INCLUDE_DIRS glad INCLUDE_DIRECTORIES)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(
        ${PROJECT_NAME}
        PRIVATE
        ${GLAD_INCLUDE_DIRS}
        ${glfw_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad)

# Find and link OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)

# Platform-specific settings
if(APPLE)
  target_link_libraries(
          ${PROJECT_NAME} PRIVATE "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
elseif(UNIX AND NOT APPLE)
  find_package(X11 REQUIRED)
  if(X11_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES})
  endif()
  find_package(Threads REQUIRED)
  target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
elseif(WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE gdi32)
endif()

# Enable high DPI awareness on Windows
if(WIN32)
  set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DPI_AWARE "PerMonitor")
endif()