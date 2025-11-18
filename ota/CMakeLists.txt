# OTA Module CMakeLists.txt

# Create ltlp library
add_library(ltlp STATIC
    ltlp/ltlp_api.c
    ltlp/ltlp_common.c
    ltlp/ltlp_kernel.c
    ltlp/ltlp_receiver.c
    ltlp/ltlp_sender.c
)

# Set include directories for ltlp
target_include_directories(ltlp
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ltlp
)

# Create printf library
add_library(printf STATIC
    printf/printf.c
)

# Set include directories for printf
target_include_directories(printf
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/printf
)

# Create get_id library
add_library(get_id STATIC
    get_id.c
)

# Set include directories for get_id
target_include_directories(get_id
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Create main OTA library (combines all OTA components)
add_library(OTA INTERFACE)

target_link_libraries(OTA
    INTERFACE
    ltlp
    printf
    get_id
)

target_include_directories(OTA
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ltlp
    ${CMAKE_CURRENT_SOURCE_DIR}/printf
)

