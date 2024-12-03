cmake_minimum_required(VERSION 3.16)

project(Reaper LANGUAGES C CXX)
# by default CMake selects "ALL_BUILD" as the startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
	PROPERTY VS_STARTUP_PROJECT Reaper)

# currently using unicode in some libraries on win32 but will change soon
ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)

# CMake FXC shader compilation, add any shaders you want compiled here
set(VERTEX_SHADERS 
	# add vertex shader (.hlsl) files here
)
set(PIXEL_SHADERS 
	# add pixel shader (.hlsl) files here
)

# Add any new C/C++ source code here
add_executable (Reaper 
	# Header & CPP files go here
	${CMAKE_SOURCE_DIR}/libraries/gateware/gateware.h
	${CMAKE_SOURCE_DIR}/libraries/tinyxml2/tinyxml2.h
	${CMAKE_SOURCE_DIR}/libraries/tinyxml2/tinyxml2.cpp
	main.cpp
	renderer.h
	d3dx12.h
	sprite.h
	sprite.cpp
	spritesheet.h
	spritesheet.cpp
	font.h
	font.cpp
	hud.h
	${VERTEX_SHADERS}
	${PIXEL_SHADERS}
)

# This code will tell visual studio to convert above shader .hlsl to .h files
# The produced .h files can be included and contain the compiled shader byte code
set_source_files_properties( ${VERTEX_SHADERS} PROPERTIES 
	VS_SHADER_TYPE Vertex 
	VS_SHADER_MODEL 5.1 
	VS_SHADER_ENTRYPOINT main 
	#VS_SHADER_OUTPUT_HEADER_FILE "%(RelativeDir)%(Filename).h"
	#VS_SHADER_VARIABLE_NAME "%(Filename)"
	#VS_SHADER_ENABLE_DEBUG $<$<CONFIG:Debug>:true>
)
set_source_files_properties( ${PIXEL_SHADERS} PROPERTIES 
	VS_SHADER_TYPE Pixel 
	VS_SHADER_MODEL 5.1 
	VS_SHADER_ENTRYPOINT main 
	#VS_SHADER_OUTPUT_HEADER_FILE "%(RelativeDir)%(Filename).h"
	#VS_SHADER_VARIABLE_NAME "%(Filename)"
	#VS_SHADER_ENABLE_DEBUG $<$<CONFIG:Debug>:true>
)

# add support for dds texture loading
include_directories(${CMAKE_SOURCE_DIR}/libraries/directxtk12/include)

target_include_directories(Reaper PUBLIC ${CMAKE_SOURCE_DIR})

# Find the libraries
find_library(DDS_LIB_D NAMES DirectXTK12_x64_Debug PATHS ${CMAKE_SOURCE_DIR}/libraries/directxtk12/lib/)
find_library(DDS_LIB_R NAMES DirectXTK12_x64_Release PATHS ${CMAKE_SOURCE_DIR}/libraries/directxtk12/lib/)
# link the ktx sdk include and lib files
target_link_libraries(Reaper debug ${DDS_LIB_D} optimized ${DDS_LIB_R})

set_target_properties(Reaper PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin
)
