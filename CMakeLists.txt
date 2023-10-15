cmake_minimum_required ( VERSION 3.16 )

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(EFIGenieESP32)

set(LIBRARIES libs)

include_directories(include)
include_directories(${LIBRARIES}/EFIGenie/libs/EmbeddedIOServices/HardwareAbstractions/Esp32Idf/include)

file(GLOB_RECURSE USER_CSRCS src/*.c)
file(GLOB_RECURSE USER_CSRCS_Templates src/*_template.c)
IF(USER_CSRCS_Templates)
        list(REMOVE_ITEM USER_CSRCS ${USER_CSRCS_Templates})
ENDIF()
file(GLOB_RECURSE USER_CXXSRCS src/*.cpp)
file(GLOB_RECURSE USER_CXXSRCS_Templates src/*_template.cpp)
IF(USER_CXXSRCS_Templates)
        list(REMOVE_ITEM USER_CXXSRCS ${USER_CXXSRCS_Templates})
ENDIF()
file(GLOB_RECURSE EmbeddedIOService_SRCS ${LIBRARIES}/EFIGenie/libs/EmbeddedIOServices/HardwareAbstractions/Esp32Idf/src/*.cpp)
file(GLOB_RECURSE EmbeddedIOService_SRCS_Templates ${LIBRARIES}/EFIGenie/libs/EmbeddedIOServices/HardwareAbstractions/Esp32Idf/src/*_template.cpp)
IF(EmbeddedIOService_SRCS_Templates)
        list(REMOVE_ITEM EmbeddedIOService_SRCS ${EmbeddedIOService_SRCS_Templates})
ENDIF()

set(SRCS ${USER_CSRCS} ${USER_CXXSRCS} ${EmbeddedIOService_SRCS})
target_sources(${CMAKE_PROJECT_NAME}.elf PUBLIC ${SRCS})

add_subdirectory (${LIBRARIES}/EFIGenie)
target_link_libraries(${CMAKE_PROJECT_NAME}.elf PUBLIC EFIGenie)

target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "tune.bin" BINARY)

# set(HEX_FILE ${PROJECT_SOURCE_DIR}/build/firmware.hex)
# set(BIN_FILE ${PROJECT_SOURCE_DIR}/build/firmware.bin)
# add_custom_command(TARGET firmware.elf POST_BUILD
#         COMMAND ${CMAKE_OBJCOPY} ARGS -O ihex $<TARGET_FILE:firmware.elf> ${HEX_FILE}
#         COMMAND ${CMAKE_OBJCOPY} ARGS -O binary $<TARGET_FILE:firmware.elf> ${BIN_FILE}
#         COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}")
