cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once, and is not re-generated if converter is
# called multiple times.
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

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

set(UWB_CHIP_TYPE "CX310")
set(RS485_DEBUG ON)

if(UWB_CHIP_TYPE STREQUAL "DW1000")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC UWB_CHIP_TYPE_DW1000)
    target_link_libraries(${CMAKE_PROJECT_NAME} dw1000)
elseif(UWB_CHIP_TYPE STREQUAL "CX310")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC UWB_CHIP_TYPE_CX310)
    target_link_libraries(${CMAKE_PROJECT_NAME} cx310)
else()
    message(FATAL_ERROR "Unsupported chip type: ${UWB_CHIP_TYPE}")
endif()

if(RS485_DEBUG)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC RS485_DEBUG)
else()
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC RS232_DEBUG)
endif()

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)
add_subdirectory(User)
add_subdirectory(easylogger)
add_subdirectory(FreeRTOScpp)
add_subdirectory(protocol)
add_subdirectory(factory_test)
add_subdirectory(bootloader)

# Link directories setup
target_link_directories(
  ${CMAKE_PROJECT_NAME} PRIVATE # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE # Add user sources here
)

# Add include paths
target_include_directories(
  ${CMAKE_PROJECT_NAME} PRIVATE # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(
  ${CMAKE_PROJECT_NAME} PRIVATE # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    easylogger
    FreeRTOScpp
    WhtsProtocol
    cx310
    # Add user defined libraries
)

# 设置cmake install指令，烧录程序
include(${PROJECT_SOURCE_DIR}/cmake/flash_by_install.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/create_bin.cmake)

# 获取版本信息
# 优先从VERSION文件读取（由Git hooks自动更新）
set(VERSION_FILE "${CMAKE_SOURCE_DIR}/VERSION")
set(VERSION_FROM_FILE "")
set(USE_VERSION_FILE FALSE)
# 初始化版本变量为默认值
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)

if(EXISTS ${VERSION_FILE})
  file(READ ${VERSION_FILE} VERSION_FROM_FILE)
  # 移除所有空白字符（包括换行符、空格、制表符等）
  string(STRIP "${VERSION_FROM_FILE}" VERSION_FROM_FILE)
  string(REGEX REPLACE "[ \t\r\n]+" "" VERSION_FROM_FILE "${VERSION_FROM_FILE}")
  
  # 验证版本格式
  if(VERSION_FROM_FILE MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
    string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" 
                   VERSION_MATCH 
                   PROJECT_VERSION_MAJOR 
                   PROJECT_VERSION_MINOR 
                   PROJECT_VERSION_PATCH 
                   "${VERSION_FROM_FILE}")
    if(VERSION_MATCH)
      # 确保提取的值是数字
      if(PROJECT_VERSION_MAJOR MATCHES "^[0-9]+$" AND 
         PROJECT_VERSION_MINOR MATCHES "^[0-9]+$" AND 
         PROJECT_VERSION_PATCH MATCHES "^[0-9]+$")
        message(STATUS "Version from VERSION file: ${VERSION_FROM_FILE}")
        set(USE_VERSION_FILE TRUE)
      else()
        message(WARNING "VERSION file contains non-numeric values, using default 0.0.0")
      endif()
    else()
      message(WARNING "VERSION file regex match failed: ${VERSION_FROM_FILE}, using default 0.0.0")
    endif()
  else()
    message(WARNING "VERSION file format is invalid: '${VERSION_FROM_FILE}', using default 0.0.0")
  endif()
endif()

# 如果VERSION文件无效，回退到Git标签方式
if(NOT USE_VERSION_FILE)
  # 获取完整的 git describe（包含标签和commit信息）
  execute_process(
    COMMAND git describe --tags --dirty --always
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE GIT_DESCRIBE_RESULT)

  # 尝试获取最近的标签
  execute_process(
    COMMAND git describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_TAG
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE GIT_TAG_RESULT)

  # 如果本地没有标签，尝试从远程获取标签
  if(GIT_TAG_RESULT OR NOT GIT_TAG)
    message(STATUS "No local tags found, attempting to fetch tags from remote...")
    execute_process(
      COMMAND git fetch --tags --quiet
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      ERROR_QUIET
      RESULT_VARIABLE FETCH_RESULT)
    
    if(NOT FETCH_RESULT)
      execute_process(
        COMMAND git describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE GIT_TAG_RESULT)
    endif()
  endif()

  # 获取commit数量（从最近标签到当前commit）
  set(COMMIT_COUNT 0)
  if(GIT_TAG)
    execute_process(
      COMMAND git rev-list --count ${GIT_TAG}..HEAD
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE COMMIT_COUNT
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT COMMIT_COUNT)
      set(COMMIT_COUNT 0)
    endif()
  else()
    # 如果没有标签，获取总commit数
    execute_process(
      COMMAND git rev-list --count HEAD
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE COMMIT_COUNT
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT COMMIT_COUNT)
      set(COMMIT_COUNT 0)
    endif()
  endif()

  # 提取 MAJOR, MINOR, PATCH
  set(VERSION_MATCH "")
  if(GIT_TAG)
    string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" 
                       VERSION_MATCH 
                       PROJECT_VERSION_MAJOR 
                       PROJECT_VERSION_MINOR 
                       PROJECT_VERSION_PATCH 
                       ${GIT_TAG})
  endif()

  if(VERSION_MATCH)
    # 从标签提取版本号成功
    # 如果有新的commit，增加PATCH版本号
    if(COMMIT_COUNT AND COMMIT_COUNT GREATER 0)
      math(EXPR PROJECT_VERSION_PATCH "${PROJECT_VERSION_PATCH} + ${COMMIT_COUNT}")
      message(STATUS "Version from git tag: ${GIT_TAG} + ${COMMIT_COUNT} commits -> ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
    else()
      message(STATUS "Version from git tag: ${GIT_TAG} -> ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
    endif()
  else()
    # 没有找到标签，使用默认版本或基于commit数生成
    if(COMMIT_COUNT GREATER 0)
      # 基于commit数量生成版本号：0.0.{commit_count}
      set(PROJECT_VERSION_MAJOR 0)
      set(PROJECT_VERSION_MINOR 0)
      set(PROJECT_VERSION_PATCH ${COMMIT_COUNT})
      message(STATUS "No git tag found, using commit-based version: 0.0.${PROJECT_VERSION_PATCH} (${COMMIT_COUNT} commits)")
    else()
      # 完全无法获取版本信息，使用默认值
      set(PROJECT_VERSION_MAJOR 0)
      set(PROJECT_VERSION_MINOR 0)
      set(PROJECT_VERSION_PATCH 0)
      message(WARNING "Cannot determine version from git, using default version 0.0.0")
      message(WARNING "Hint: Create a git tag (e.g., 'git tag v1.0.0') or ensure git repository is initialized")
    endif()
  endif()
endif()
# 确保版本变量不为空（防止宏定义为空字符串）
if(NOT PROJECT_VERSION_MAJOR)
  set(PROJECT_VERSION_MAJOR 0)
endif()
if(NOT PROJECT_VERSION_MINOR)
  set(PROJECT_VERSION_MINOR 0)
endif()
if(NOT PROJECT_VERSION_PATCH)
  set(PROJECT_VERSION_PATCH 0)
endif()

set(VERSION_STRING
    "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
)
string(TIMESTAMP BUILD_DATE "%Y%m%d")

# 添加版本定义，可以在代码中使用
# 注意：使用引号确保宏值正确传递
add_definitions(
  -DFIRMWARE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  -DFIRMWARE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
  -DFIRMWARE_VERSION_PATCH=${PROJECT_VERSION_PATCH}
  -DFIRMWARE_VERSION_STRING="${VERSION_STRING}"
  -DFIRMWARE_BUILD_DATE="${BUILD_DATE}")

# 验证版本号已正确设置
message(STATUS "Firmware version: ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

# 合并 bootloader 和应用程序固件
# 注意：必须在目标创建之后包含
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    include(${PROJECT_SOURCE_DIR}/cmake/combine_firmware.cmake)
endif()

# auto format User folder code
file(GLOB_RECURSE USER_SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/User/*.cpp"
    "${CMAKE_SOURCE_DIR}/User/*.h"
)

add_custom_target(format ALL
    COMMAND clang-format -i -style=file ${USER_SOURCE_FILES}
    COMMENT "Running clang-format on all files in User/"
)

# 在编译完成后输出版本信息
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
    COMMAND ${CMAKE_COMMAND} -E echo "Firmware Build Complete"
    COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
    COMMAND ${CMAKE_COMMAND} -E echo "Project Name: ${CMAKE_PROJECT_NAME}"
    COMMAND ${CMAKE_COMMAND} -E echo "Firmware Version: ${VERSION_STRING}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Major Version: ${PROJECT_VERSION_MAJOR}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Minor Version: ${PROJECT_VERSION_MINOR}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Patch Version: ${PROJECT_VERSION_PATCH}"
    COMMAND ${CMAKE_COMMAND} -E echo "Build Date: ${BUILD_DATE}"
    COMMAND ${CMAKE_COMMAND} -E echo "Build Type: ${CMAKE_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} -E echo "UWB Chip Type: ${UWB_CHIP_TYPE}"
    COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMENT "Displaying firmware version information"
)