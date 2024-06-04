add_executable(test_lib
    test_assembly.cpp)

target_link_libraries(test_lib GTest::gtest_main)

add_custom_target(check-lib
    COMMAND test_lib)
