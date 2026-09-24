cmake_minimum_required(VERSION 3.16)
project(csvlens VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(csvlens_lib src/csvlens.c)
target_include_directories(csvlens_lib PUBLIC include)
if(MSVC)
    target_compile_options(csvlens_lib PRIVATE /W4)
else()
    target_compile_options(csvlens_lib PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(csvlens src/main.c)
target_link_libraries(csvlens PRIVATE csvlens_lib)

enable_testing()
add_executable(csvlens_tests tests/test_csv.c)
target_link_libraries(csvlens_tests PRIVATE csvlens_lib)
add_test(NAME csvlens_unit_tests COMMAND csvlens_tests)

install(TARGETS csvlens RUNTIME DESTINATION bin)
