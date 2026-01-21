# Protocol Messages Module CMakeLists.txt

# Create Protocol Messages library
add_library(ProtocolMessages STATIC 
    Master2Slave.cpp
    Slave2Master.cpp
    Backend2Master.cpp
    Master2Backend.cpp
)

# Set include directories
target_include_directories(ProtocolMessages 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..  # For protocol headers
)

# Set target properties
set_target_properties(ProtocolMessages PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# 编译选项
if(MSVC)
    target_compile_options(ProtocolMessages PRIVATE /W4)
else()
    target_compile_options(ProtocolMessages PRIVATE -Wall -Wextra -pedantic)
endif()

# Install rules removed as requested 