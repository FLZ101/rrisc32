add_subdirectory(unit)
add_subdirectory(lit)

add_custom_target(rrisc32-check-all
    DEPENDS rrisc32-check-unit rrisc32-check-lit)
