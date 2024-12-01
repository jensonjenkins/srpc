Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0 # or a later release
)

FetchContent_MakeAvailable(Catch2)
set (TEST_LIST
    parser_test.cpp
    lexer_test.cpp
    )
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_executable(unit_tests ${TEST_LIST}) 
target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain)

