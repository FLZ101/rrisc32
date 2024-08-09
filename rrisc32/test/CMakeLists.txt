add_subdirectory(unit)
add_subdirectory(lit)

add_custom_target(check-all
    DEPENDS check-unit check-lit)
