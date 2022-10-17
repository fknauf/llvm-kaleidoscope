project(kaleidoscope)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_COMPILER clang++-14)
set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wall -Wextra -Werror -fstack-protector -fstack-protector-all -fsanitize=address -fsanitize=undefined)
add_link_options(-fsanitize=address -fsanitize=undefined)

link_libraries(c++abi LLVM-14)
add_library(kaleidoscope
            kaleidoscope/ast.cpp
            kaleidoscope/codegen.cpp
            kaleidoscope/jit.cpp
            kaleidoscope/lexer.cpp
            kaleidoscope/optimizer.cpp
            kaleidoscope/parser.cpp
            kaleidoscope/api_functions.cpp
            kaleidoscope/symbols.cpp
            kaleidoscope/objcode.cpp
)

include_directories(BEFORE .)
link_libraries(kaleidoscope)
set(CMAKE_ENABLE_EXPORTS 1)

add_executable(   parser_test frontends/parser_test.cpp)
add_executable(  codegen_test frontends/codegen_test.cpp)
add_executable(optimizer_test frontends/optimizer_test.cpp)
add_executable(      jit_test frontends/jit_test.cpp)
add_executable(  objcode_test frontends/objcode_test.cpp)
