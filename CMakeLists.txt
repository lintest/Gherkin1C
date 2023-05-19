cmake_minimum_required(VERSION 3.10)
project(Gherkin1C)
set(TARGET Gherkin1C)

option(WITH_BOOST "Use boost library" ON)

# Create target for the parser
ADD_CUSTOM_TARGET(GherkinLexer echo Creating GherkinLexer)

if (UNIX)
    set(REFLEX_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/RE-flex/bin/reflex" CACHE PATH "Reflex executable")
else(UNIX)
    set(REFLEX_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/RE-flex/bin/win64/reflex.exe" CACHE PATH "Reflex executable")
endif(WIN32)

add_custom_command(TARGET GherkinLexer
    COMMAND ${REFLEX_EXECUTABLE} 
    ARGS --unicode --lexer=GherkinLexer 
        --outfile=gherkin.lex.cpp --header-file=gherkin.lex.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gherkin.l
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/gherkin.l
    OUTPUTS gherkin.lex.cpp gherkin.lex.h
    VERBATIM
)

set(GherkinLexer_SRCS
    RE-flex/lib/convert.cpp
    RE-flex/lib/debug.cpp
    RE-flex/lib/error.cpp
    RE-flex/lib/input.cpp
    RE-flex/lib/matcher.cpp
    RE-flex/lib/pattern.cpp
    RE-flex/lib/posix.cpp
    RE-flex/lib/unicode.cpp
    RE-flex/lib/utf8.cpp
    RE-flex/unicode/block_scripts.cpp
    RE-flex/unicode/language_scripts.cpp
    RE-flex/unicode/letter_scripts.cpp
    gherkin.lex.cpp
    gherkin.lex.h
    src/gherkin.cpp
    src/gherkin.h
    src/json.hpp
)

set_source_files_properties( 
    gherkin.lex.cpp
    gherkin.lex.h
    GENERATED
)

add_library(${TARGET} SHARED
	${GherkinLexer_SRCS}
	src/AddInNative.cpp
	src/AddInNative.def
	src/AddInNative.h
	src/AddInNative.rc
	src/GherkinParser.cpp
	src/GherkinParser.h
	src/stdafx.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/RE-flex/include)
target_compile_definitions(${TARGET} PRIVATE UNICODE _UNICODE)
target_include_directories(${TARGET} PRIVATE include)
add_dependencies(${TARGET} GherkinLexer)

if (UNIX)
	if (APPLE)
		set(MySuffix1 "Mac")
	else(APPLE)
		set(MySuffix1 "Lin")
	endif(APPLE)
	if (TARGET_PLATFORM_32)
		set(MySuffix2 "32")
	else()
		set(MySuffix2 "64")
	endif()
else(UNIX)
	if (NOT MSVC)
		message(FATAL_ERROR "Must be compiled with MSVC on Windows")
	endif(NOT MSVC)
	set(MySuffix1 "Win")
endif(UNIX)

set (LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin${MySuffix2})
set (EXECUTABLE_OUTPUT_PATH ${LIBRARY_OUTPUT_PATH})
set (CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH})

set_target_properties( ${PROJECT_NAME} PROPERTIES
	PREFIX ""
	OUTPUT_NAME ${PROJECT_NAME}${MySuffix1}${MySuffix2} 
	POSITION_INDEPENDENT_CODE ON 
	CXX_STANDARD_REQUIRED ON
	CXX_STANDARD 17
)

if (UNIX)
	if (TARGET_PLATFORM_32)
		set(CMAKE_C_FLAGS "-m32 ${CMAKE_C_FLAGS}")
		set(CMAKE_CXX_FLAGS "-m32 ${CMAKE_CXX_FLAGS}")
	else(TARGET_PLATFORM_32)
		set(CMAKE_C_FLAGS "-m64 ${CMAKE_C_FLAGS}")
		set(CMAKE_CXX_FLAGS "-m64 ${CMAKE_CXX_FLAGS}")
	endif ()
	set_target_properties(${TARGET} PROPERTIES PREFIX "")
else(UNIX)
	add_definitions(/MT)
	set(CMAKE_SUPPRESS_REGENERATION 1)
	set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
	target_compile_definitions(${TARGET} PRIVATE _WINDOWS
			_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
	target_compile_options(${TARGET} PRIVATE /utf-8)
	add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")	
	add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
endif(UNIX)

set(TESTER GherkinConsole)
add_executable(${TESTER} ${GherkinLexer_SRCS} src/console.cpp)
target_compile_definitions(${TESTER} PRIVATE UNICODE _UNICODE)
add_dependencies(${TESTER} GherkinLexer)
if (NOT UNIX)
	target_compile_definitions(${TARGET} PRIVATE _WINDOWS)
	target_compile_options(${TARGET} PRIVATE /utf-8)
endif(NOT UNIX)

set_target_properties(${TESTER} PROPERTIES
	POSITION_INDEPENDENT_CODE ON 
	CXX_STANDARD_REQUIRED ON
	CXX_STANDARD 17
)

if (WITH_BOOST)
	MESSAGE(STATUS "WITH_BOOST")
	add_definitions(-DUSE_BOOST)
	set(Boost_USE_STATIC_LIBS ON) 
	set(Boost_USE_MULTITHREADED ON)  
	set(Boost_USE_STATIC_RUNTIME ON) 
	set(Boost_NO_SYSTEM_PATHS TRUE) 
	set(BOOST_ROOT "${CMAKE_SOURCE_DIR}/boost_1_82_0")
	set(BOOST_INCLUDE_DIRS "${BOOST_ROOT}/include")
	set(BOOST_LIBRARY_DIRS "${BOOST_ROOT}/stage/lib")
	find_package(Boost REQUIRED regex date_time filesystem)
	include_directories(${Boost_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
	target_link_libraries(${TESTER} ${Boost_LIBRARIES})
endif (WITH_BOOST)

# Include binary directory to include lexer.c in parser.c
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${Gherkin1C_BINARY_DIR})
