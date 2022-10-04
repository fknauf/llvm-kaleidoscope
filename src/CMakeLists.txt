project(kaleidoscope)

set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_CXX_COMPILER clang++-16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

link_libraries(LLVM-14)
add_library(kaleidoscope
            kaleidoscope/ast.cpp
            kaleidoscope/codegen.cpp
            kaleidoscope/jit.cpp
            kaleidoscope/lexer.cpp
            kaleidoscope/optimizer.cpp
            kaleidoscope/parser.cpp
            kaleidoscope/api_functions.cpp
            kaleidoscope/symbols.cpp
)

include_directories(BEFORE .)
link_libraries(kaleidoscope)
set(CMAKE_ENABLE_EXPORTS 1)

add_executable(   parser_test frontends/parser_test.cpp)
add_executable(  codegen_test frontends/codegen_test.cpp)
add_executable(optimizer_test frontends/optimizer_test.cpp)
add_executable(      jit_test frontends/jit_test.cpp)
