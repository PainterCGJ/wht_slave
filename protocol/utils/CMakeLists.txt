# Protocol Utils Module CMakeLists.txt

# Create Protocol Utils library
add_library(ProtocolUtils STATIC 
    ByteUtils.cpp
    ByteUtils.h
)

# Set include directories
target_include_directories(ProtocolUtils 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set target properties
set_target_properties(ProtocolUtils PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# 编译选项
if(MSVC)
    target_compile_options(ProtocolUtils PRIVATE /W4)
else()
    target_compile_options(ProtocolUtils PRIVATE -Wall -Wextra -pedantic)
endif()

# Install rules removed as requested 