include(FetchContent)

FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
)
FetchContent_MakeAvailable(doctest)

add_executable(test_meta_compose test_meta_compose.cpp)
target_link_libraries(test_meta_compose PRIVATE gst-metadata doctest::doctest)
add_test(NAME meta_compose COMMAND test_meta_compose)
