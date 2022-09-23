project(kaleidoscope)

set(CMAKE_BUILD_TYPE Debug)

link_libraries(LLVM-14)
add_library(kaleidoscope
            kaleidoscope/ast.cpp
            kaleidoscope/lexer.cpp
            kaleidoscope/parser.cpp
            kaleidoscope/codegen.cpp
)

include_directories(BEFORE .)
link_libraries(kaleidoscope)
add_executable( parser_test frontends/parser_test.cpp)
add_executable(codegen_test frontends/codegen_test.cpp)
