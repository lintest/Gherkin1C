cmake_minimum_required(VERSION 3.10)
set(TARGET GherkinReflex)
project(GherkinReflex)

# Create target for the parser
ADD_CUSTOM_TARGET(GherkinReflex_Parser echo Creating GherkinLexer)

if (WIN32)
    set(REFLEX_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/RE-flex/bin/win64/reflex.exe" CACHE PATH "Reflex executable")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/RE-flex/include)
endif(WIN32)

add_custom_command(TARGET GherkinReflex_Parser
    COMMAND ${REFLEX_EXECUTABLE} 
    ARGS --unicode --lexer=GherkinLexer --header-file ${GherkinReflex_SOURCE_DIR}/src/gherkin.l
    DEPENDS ${GherkinReflex_SOURCE_DIR}/src/gherkin.l
    OUTPUTS lex.yy.cpp lex.yy.h
    VERBATIM
)

set(GherkinReflex_SRCS 
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
    src/console.cpp
    src/gherkin.cpp
    src/gherkin.h
    src/json.hpp
    lex.yy.cpp
    lex.yy.h
)

set_source_files_properties( 
    lex.yy.cpp
    lex.yy.h
    GENERATED
)

# Include binary directory to include lexer.c in parser.c
include_directories(${GherkinReflex_SOURCE_DIR}/src)
include_directories(${GherkinReflex_BINARY_DIR})
add_executable(${TARGET} ${GherkinReflex_SRCS})
add_dependencies(${TARGET} GherkinReflex_Parser)
