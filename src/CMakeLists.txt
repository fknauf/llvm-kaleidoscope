project(kaleidoscope)

set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_CXX_COMPILER clang++-14)
set(CMAKE_CXX_STANDARD 20)

option(USE_ASAN "use adress sanitizer" on)
option(USE_STACK_PROTECTOR "use stack protector" on)

add_compile_options(-Wall -Wextra -Werror)

if(USE_STACK_PROTECTOR)
    add_compile_options(-fstack-protector -fstack-protector-all)
endif(USE_STACK_PROTECTOR)

if(USE_ASAN) 
    add_compile_options(-fsanitize=address -fsanitize=undefined)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif(USE_ASAN)

link_libraries(c++abi LLVM-14)
add_library(kaleidoscope
            kaleidoscope/api_functions.cpp
            kaleidoscope/ast.cpp
            kaleidoscope/codegen.cpp
            kaleidoscope/debug.cpp
            kaleidoscope/jit.cpp
            kaleidoscope/lexer.cpp
            kaleidoscope/objcode.cpp
            kaleidoscope/optimizer.cpp
            kaleidoscope/parser.cpp
            kaleidoscope/sourcelocation.cpp
            kaleidoscope/symbols.cpp
)

include_directories(BEFORE .)
link_libraries(kaleidoscope)
set(CMAKE_ENABLE_EXPORTS 1)

add_executable(   parser_test frontends/parser_test.cpp)
add_executable(  codegen_test frontends/codegen_test.cpp)
add_executable(optimizer_test frontends/optimizer_test.cpp)
add_executable(      jit_test frontends/jit_test.cpp)
add_executable(  objcode_test frontends/objcode_test.cpp)
add_executable(    debug_test frontends/debug_test.cpp)
