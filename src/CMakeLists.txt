project(kaleidoscope)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)

link_libraries(LLVM-14)
add_library(kaleidoscope
            kaleidoscope/ast.cpp
            kaleidoscope/lexer.cpp
            kaleidoscope/parser.cpp
            kaleidoscope/codegen.cpp
            kaleidoscope/optimizer.cpp
            kaleidoscope/jit.cpp
)

include_directories(BEFORE .)
link_libraries(kaleidoscope)
add_executable(   parser_test frontends/parser_test.cpp)
add_executable(  codegen_test frontends/codegen_test.cpp)
add_executable(optimizer_test frontends/optimizer_test.cpp)
add_executable(      jit_test frontends/jit_test.cpp)