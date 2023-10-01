# CMakeList.txt : CMake project for chibi-tech, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

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
 "code/platform/platform_logger.cpp")

set(UTIL
	"code/util/str8.h" "code/util/str8.cpp"
 "code/util/bit.h")

# Add source to this project's executable.
add_executable (chibi-tech 
	"code/chibi-tech.cpp" "code/chibi-tech.h" "code/types.h"
	${PLATFORM_WIN32}
	${UTIL}
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET chibi-tech PROPERTY CXX_STANDARD 20)
endif()

target_include_directories(chibi-tech PRIVATE "code/")

target_link_libraries(chibi-tech
        PRIVATE
            Winmm.lib
)

add_compile_definitions(WIN32_LEAN_AND_MEAN)
add_compile_definitions(NOMINMAX)