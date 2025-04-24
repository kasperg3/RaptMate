idf_component_register(SRCS "main.cpp" "src/RaptMateServer.cpp" "src/RaptPillBLE.cpp" INCLUDE_DIRS "." "src" )

# Build react spiffs image
add_custom_target(dep
    COMMAND npm run build
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/rapt-mate
    COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_SOURCE_DIR}/rapt-mate/build/static/js/*LICENSE.txt # Remove the license files, they are too long for the spiffs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/rapt-mate/build ${CMAKE_SOURCE_DIR}/binaries
)
spiffs_create_partition_image(storage ${CMAKE_SOURCE_DIR}/binaries FLASH_IN_PROJECT DEPENDS dep)
