include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        f8d7d77c06936315286eb55f8de22cd23c188571 # v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

link_libraries(rrisc32)

add_executable(rrisc32-gtest
    test_assembly.cpp
    test_rrisc32.cpp)

target_link_libraries(rrisc32-gtest GTest::gtest_main)

add_custom_target(check-unit
    COMMAND rrisc32-gtest)
