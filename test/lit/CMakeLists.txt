file(CHMOD ${CMAKE_CURRENT_SOURCE_DIR}/lit.sh
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
add_custom_target(check-lit
    COMMAND ./lit.sh ${CMAKE_CURRENT_BINARY_DIR}
        $<TARGET_FILE_DIR:assemble> $<TARGET_FILE_DIR:dump>
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(check-lit assemble dump)
