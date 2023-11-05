# CMakeList.txt : CMake project for chibi-tech, include source and define
# project specific logic here.
#
# For now, to run the app, must manually pass the content directory path. If using
# Visual Studio, 
#	Debug -> Debug and Launch Settings for chibi-tech
#	This will open launch.vs.json
#	Add the following entry:
#		"args": [ "${workspaceRoot}\\content" ]
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("chibi-tech")

set(PLATFORM_WIN32 
	"code/platform/platform.h"
	"code/platform/win32/common_win32.h"
	"code/platform/win32/window_win32.cpp"
	"code/platform/win32/timer_win32.cpp"
	"code/platform/win32/logger_win32.cpp"
	"code/platform/win32/common_win32.cpp"
	"code/platform/platform_logger.cpp"
	"code/platform/win32/file_win32.cpp"
)

set(UTIL
	"code/util/str8.h"      "code/util/str8.cpp"
	"code/util/bit.h"
	"code/util/str16.h"     "code/util/str16.cpp" 
	"code/util/allocator.h" "code/util/allocator.cpp" 
	"code/util/array.h")

set(RENDERER
	"code/renderer/dx12/d3d12_common.h"
	                                                        "code/renderer/dx12/d3d12_export.cpp"
	"code/renderer/dx12/gpu_device.h"                       "code/renderer/dx12/gpu_device.cpp" 
	"code/renderer/simple_renderer.h"                       "code/renderer/simple_renderer.cpp"
	"code/renderer/dx12/gpu_command_queue.h"                "code/renderer/dx12/gpu_command_queue.cpp" 
	"code/renderer/dx12/gpu_command_list.h"                 "code/renderer/dx12/gpu_command_list.cpp" 
	"code/renderer/dx12/gpu_swapchain.h"                    "code/renderer/dx12/gpu_swapchain.cpp"
	"code/renderer/dx12/gpu_resource.h"                     "code/renderer/dx12/gpu_resource.cpp" 
	"code/renderer/dx12/gpu_utils.h"                        "code/renderer/dx12/gpu_utils.cpp" 
	"code/renderer/dx12/gpu_descriptor_allocator.h"         "code/renderer/dx12/gpu_descriptor_allocator.cpp"
	"code/renderer/dx12/gpu_dynamic_descriptor_heap.h"      "code/renderer/dx12/gpu_dynamic_descriptor_heap.cpp" 
	"code/renderer/dx12/gpu_root_signature.h"               "code/renderer/dx12/gpu_root_signature.cpp"
	"code/renderer/dx12/gpu_pso.h"                          "code/renderer/dx12/gpu_pso.cpp"
	"code/renderer/dx12/gpu_shader_utils.h"                 "code/renderer/dx12/gpu_shader_utils.cpp"
)

set(SYSTEMS
	"code/systems/resource_system.h" "code/systems/resource_system.cpp"
)

# Add source to this project's executable.
add_executable (chibi-tech 
	"code/chibi-tech.cpp" "code/types.h"
	${PLATFORM_WIN32}
	${UTIL}
	${RENDERER}
	${SYSTEMS}
 )

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET chibi-tech PROPERTY CXX_STANDARD 20)
endif()

target_include_directories(chibi-tech PRIVATE "code/" "vendor/" "vendor/agility_sdk/")

target_link_libraries(chibi-tech
        PRIVATE
            Winmm.lib
			dxgi.lib
			d3d12.lib 
			dxguid.lib 
			d3dcompiler.lib
)

add_compile_definitions(WIN32_LEAN_AND_MEAN)
add_compile_definitions(NOMINMAX)

SET(AGILITY_SDK_BIN "${CMAKE_CURRENT_SOURCE_DIR}/vendor/agility_sdk/bin/x64/")

file(
    COPY
		"${AGILITY_SDK_BIN}/D3D12Core.dll"
		"${AGILITY_SDK_BIN}/D3D12Core.pdb"
		"${AGILITY_SDK_BIN}/d3d12SDKLayers.dll"
		"${AGILITY_SDK_BIN}/d3d12SDKLayers.pdb"
    DESTINATION
        ${chibi-tech_BINARY_DIR}/D3D12
)