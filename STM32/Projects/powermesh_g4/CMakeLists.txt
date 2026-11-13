cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME powermesh_g4)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})

set(POWERMESH_ROLE "ENDPOINT" CACHE STRING "G4 board role")
set_property(CACHE POWERMESH_ROLE PROPERTY STRINGS GATEWAY ENDPOINT)

if(NOT POWERMESH_ROLE STREQUAL "GATEWAY" AND
   NOT POWERMESH_ROLE STREQUAL "ENDPOINT")
    message(FATAL_ERROR "Invalid PowerMesh role: ${POWERMESH_ROLE}")
endif()

set(POWERMESH_TRANSPORT "WIRED" CACHE STRING "PowerMesh transport")
set_property(CACHE POWERMESH_TRANSPORT PROPERTY STRINGS WIRED ST8500)

if(NOT POWERMESH_TRANSPORT STREQUAL "WIRED" AND
   NOT POWERMESH_TRANSPORT STREQUAL "ST8500")
    message(FATAL_ERROR "Invalid transport: ${POWERMESH_TRANSPORT}")
endif()

message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ../../Common/Src/uart_frame.c
    ../../Common/Src/packet_queue.c
    ../../Common/Src/packet_router.c
    ../../Common/Src/uart_transport.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Inc
    ../../Common/Inc
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    POWERMESH_ROLE_${POWERMESH_ROLE}
    POWERMESH_TRANSPORT_${POWERMESH_TRANSPORT}
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

if(POWERMESH_TRANSPORT STREQUAL "ST8500")
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    set(ST8500_IMAGE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../General_ST/ST8500_Project_Files/G3_a0/ImageGenerator&Loader"
    )

    set(ST8500_RTE_IMAGE
        "${ST8500_IMAGE_DIR}/STARCOM_G3_RT_FW_r2_2_rc0_BA_key0.img"
    )

    set(ST8500_PE_IMAGE
        "${ST8500_IMAGE_DIR}/ST8500_G3_v19a065a0_6.3.12.img"
    )

    set(ST8500_IMAGES_ASM
        "${CMAKE_CURRENT_BINARY_DIR}/st8500_images.S"
    )

    add_custom_command(
        OUTPUT "${ST8500_IMAGES_ASM}"
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/Tools/embed_images.py"
            "${ST8500_RTE_IMAGE}"
            "${ST8500_PE_IMAGE}"
            "${ST8500_IMAGES_ASM}"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/Tools/embed_images.py"
            "${ST8500_RTE_IMAGE}"
            "${ST8500_PE_IMAGE}"
        VERBATIM
    )

    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        Core/Src/st8500_boot.c
        "${ST8500_IMAGES_ASM}"
    )
endif()
