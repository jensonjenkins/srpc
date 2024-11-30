Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0 # or a later release
)

FetchContent_MakeAvailable(Catch2)
set (TEST_LIST
    lexer.cpp
    )
add_executable(tests ${TEST_LIST}) 
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

