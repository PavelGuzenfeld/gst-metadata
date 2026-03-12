include(FetchContent)

FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
)
FetchContent_MakeAvailable(doctest)

add_executable(test_meta_compose test_meta_compose.cpp)
target_link_libraries(test_meta_compose PRIVATE gst-metadata doctest::doctest)
target_include_directories(test_meta_compose PRIVATE ${CMAKE_SOURCE_DIR}/examples/include)
add_test(NAME meta_compose COMMAND test_meta_compose)

add_executable(test_edge_cases test_edge_cases.cpp)
target_link_libraries(test_edge_cases PRIVATE gst-metadata doctest::doctest)
target_include_directories(test_edge_cases PRIVATE ${CMAKE_SOURCE_DIR}/examples/include)
add_test(NAME edge_cases COMMAND test_edge_cases)

add_executable(test_fuzz test_fuzz.cpp)
target_link_libraries(test_fuzz PRIVATE gst-metadata doctest::doctest)
target_include_directories(test_fuzz PRIVATE ${CMAKE_SOURCE_DIR}/examples/include)
add_test(NAME fuzz COMMAND test_fuzz)
