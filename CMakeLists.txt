cmake_minimum_required ( VERSION 3.9 )
project ( EFIGenie C CXX ASM )

add_definitions ( -DTARGET=esp32 )
# Include for ESP-IDF build system functions
include($ENV{IDF_PATH}/tools/cmake/idf.cmake)
# Create idf::esp32 and idf::freertos static libraries
idf_build_process(esp32
                # try and trim the build; additional components
                # will be included as needed based on dependency tree
                #
                # although esptool_py does not generate static library,
                # processing the component is needed for flashing related
                # targets and file generation
                COMPONENTS esp32 freertos esptool_py esp_adc_cal
                SDKCONFIG ${CMAKE_CURRENT_LIST_DIR}/sdkconfig
                BUILD_DIR ${CMAKE_BINARY_DIR})

set(LIBRARIES	libs)

include_directories(include)
include_directories(${LIBRARIES}/EFIGenie/libs/EmbeddedIOServices/HardwareAbstractions/Esp32Idf/include)

file(GLOB_RECURSE USER_CSOURCES src/*.c)
file(GLOB_RECURSE USER_CSOURCES_Templates src/*_template.c)
IF(USER_CSOURCES_Templates)
        list(REMOVE_ITEM USER_CSOURCES ${USER_CSOURCES_Templates})
ENDIF()
file(GLOB_RECURSE USER_CXXSOURCES src/*.cpp)
file(GLOB_RECURSE USER_CXXSOURCES_Templates src/*_template.cpp)
IF(USER_CXXSOURCES_Templates)
        list(REMOVE_ITEM USER_CXXSOURCES ${USER_CXXSOURCES_Templates})
ENDIF()
file(GLOB_RECURSE EmbeddedIOService_SOURCES ${LIBRARIES}/EFIGenie/libs/EmbeddedIOServices/HardwareAbstractions/Esp32Idf/src/*.cpp)
file(GLOB_RECURSE EmbeddedIOService_SOURCES_Templates ${LIBRARIES}/EFIGenie/libs/EmbeddedIOServices/HardwareAbstractions/Esp32Idf/src/*_template.cpp)
IF(EmbeddedIOService_SOURCES_Templates)
        list(REMOVE_ITEM EmbeddedIOService_SOURCES ${EmbeddedIOService_SOURCES_Templates})
ENDIF()

set(SOURCE_FILES ${USER_CSOURCES} ${USER_CXXSOURCES} ${EmbeddedIOService_SOURCES})
add_executable(${CMAKE_PROJECT_NAME}.elf ${SOURCE_FILES})

add_subdirectory (${LIBRARIES}/EFIGenie)
target_link_libraries(${CMAKE_PROJECT_NAME}.elf EFIGenie)

# Link the static libraries to the executable
target_link_libraries(${CMAKE_PROJECT_NAME}.elf idf::esp32 idf::freertos idf::spi_flash idf::esp_adc_cal)
# Attach additional targets to the executable file for flashing,
# linker script generation, partition_table generation, etc.
idf_build_executable(${CMAKE_PROJECT_NAME}.elf)

# set(HEX_FILE ${PROJECT_SOURCE_DIR}/build/firmware.hex)
# set(BIN_FILE ${PROJECT_SOURCE_DIR}/build/firmware.bin)
# add_custom_command(TARGET firmware.elf POST_BUILD
#         COMMAND ${CMAKE_OBJCOPY} ARGS -O ihex $<TARGET_FILE:firmware.elf> ${HEX_FILE}
#         COMMAND ${CMAKE_OBJCOPY} ARGS -O binary $<TARGET_FILE:firmware.elf> ${BIN_FILE}
#         COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}")
