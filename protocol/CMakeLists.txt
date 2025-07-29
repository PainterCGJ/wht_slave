# Protocol Module CMakeLists.txt

# Add subdirectories
add_subdirectory(utils)
add_subdirectory(messages)

# Create Protocol Core library
add_library(ProtocolCore STATIC 
    DeviceStatus.cpp
    Frame.cpp
    ProtocolProcessor.cpp
)

# Set include directories for ProtocolCore
target_include_directories(ProtocolCore 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/messages
    ${CMAKE_CURRENT_SOURCE_DIR}/../app
)

# Link with dependencies
target_link_libraries(ProtocolCore 
    PUBLIC
    ProtocolUtils
    ProtocolMessages
    easylogger
)

# Set target properties
set_target_properties(ProtocolCore PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Create main Protocol library (combines all protocol components)
add_library(WhtsProtocol INTERFACE)

target_link_libraries(WhtsProtocol 
    INTERFACE
    ProtocolCore
    ProtocolMessages  
    ProtocolUtils
)

target_include_directories(WhtsProtocol 
    INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/messages
)