cmake_minimum_required(VERSION 3.0)
project(polychora)

# based largely on: 
# https://github.com/Polytonic/Glitter/blob/master/CMakeLists.txt

# turn off compiler warnings
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif()

# setup GLFW CMake project
add_subdirectory("${PROJECT_SOURCE_DIR}/external/glfw")

# include headers
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/external/glfw/include/GLFW")
include_directories("${PROJECT_SOURCE_DIR}/external/imgui/include")
include_directories("${PROJECT_SOURCE_DIR}/external/glad/include")
include_directories("${PROJECT_SOURCE_DIR}/external/glm/glm")
include_directories("${PROJECT_SOURCE_DIR}/external/qhull/src/libqhull_r")
include_directories("${PROJECT_SOURCE_DIR}/external/qhull/src")
file(GLOB PROJECT_HEADERS "include/*.h")

# include source files
file(GLOB PROJECT_SOURCES "src/*.cpp")
file(GLOB IMGUI_SOURCES "external/imgui/src/*.cpp")
file(GLOB GLAD_SOURCES "external/glad/src/*.c")
file(GLOB QHULL_SOURCES "external/qhull/src/libqhull_r/*.c"
						"external/qhull/src/libqhull_r/*.cpp"
						"external/qhull/src/libqhullcpp/*.cpp")

# group files in IDE
source_group("include" FILES ${PROJECT_HEADERS})
source_group("src" FILES ${PROJECT_SOURCES})
source_group("external" FILES ${IMGUI_SOURCES} ${GLAD_SOURCES} ${QHULL_SOURCES})

# create the executable
add_executable(polychora ${PROJECT_SOURCES}
						 ${PROJECT_HEADERS} # if not included here, it won't show up in the IDE?
					     ${IMGUI_SOURCES}
						 ${GLAD_SOURCES}
						 ${QHULL_SOURCES})

# add libraries
target_link_libraries(polychora glfw ${GLFW_LIBRARIES})

# force C++17
set_target_properties(polychora PROPERTIES CXX_STANDARD 17)

if(MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT polychora)

	# this is necessary to build QHull for some reason
	add_link_options("/FORCE")
endif()