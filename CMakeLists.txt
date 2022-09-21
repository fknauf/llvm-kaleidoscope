project(kaleidoscope)

set(CMAKE_BUILD_TYPE Debug)

add_executable(parser_test ast.cpp lexer.cpp parser.cpp parser_test.cpp)
