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
add_subdirectory(ota)

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
    OTA
    # Add user defined libraries
)

# 设置cmake install指令，烧录程序
include(${PROJECT_SOURCE_DIR}/cmake/flash_by_install.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/create_bin.cmake)

# 获取版本信息
# 直接从Git标签读取版本信息（由Git hooks自动更新tag）
# 初始化版本变量为默认值
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)

# 查找 git 命令
find_program(GIT_EXECUTABLE git PATHS
    "C:/Program Files/Git/cmd"
    "C:/Program Files (x86)/Git/cmd"
    "C:/Program Files/Git/bin"
    "C:/Program Files (x86)/Git/bin"
    ENV PATH
    NO_DEFAULT_PATH)

if(NOT GIT_EXECUTABLE)
    find_program(GIT_EXECUTABLE git)
endif()

if(GIT_EXECUTABLE)
    message(STATUS "Found git: ${GIT_EXECUTABLE}")
else()
    message(WARNING "Git not found, version will be set to 0.0.0")
endif()

# 获取完整的 git describe（包含标签和commit信息）
if(GIT_EXECUTABLE)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --always
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE GIT_DESCRIBE_RESULT)
else()
  set(GIT_DESCRIBE "")
  set(GIT_DESCRIBE_RESULT 1)
endif()

# 尝试获取最近的标签
if(GIT_EXECUTABLE)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_TAG
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE GIT_TAG_RESULT)
else()
  set(GIT_TAG "")
  set(GIT_TAG_RESULT 1)
endif()

# 调试信息：显示 git 命令执行结果
message(STATUS "Git tag command result: ${GIT_TAG_RESULT}")
message(STATUS "Git tag value: '${GIT_TAG}'")

# 如果本地没有标签，尝试从远程获取标签
if(GIT_TAG_RESULT OR NOT GIT_TAG)
  message(STATUS "No local tags found, attempting to fetch tags from remote...")
    if(GIT_EXECUTABLE)
      execute_process(
        COMMAND ${GIT_EXECUTABLE} fetch --tags --quiet
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        ERROR_QUIET
        RESULT_VARIABLE FETCH_RESULT)
    else()
      set(FETCH_RESULT 1)
    endif()
  
  if(NOT FETCH_RESULT AND GIT_EXECUTABLE)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_TAG
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE GIT_TAG_RESULT)
  endif()
endif()

# 获取commit数量（从最近标签到当前commit）
set(COMMIT_COUNT 0)
if(GIT_TAG AND GIT_EXECUTABLE)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-list --count ${GIT_TAG}..HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE COMMIT_COUNT
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT COMMIT_COUNT)
    set(COMMIT_COUNT 0)
  endif()
elseif(GIT_EXECUTABLE)
  # 如果没有标签，获取总commit数
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE COMMIT_COUNT
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT COMMIT_COUNT)
    set(COMMIT_COUNT 0)
  endif()
endif()

# 提取 MAJOR, MINOR, PATCH
# 支持 v1.0.0 和 1.0.0 两种格式
set(VERSION_MATCH "")
if(GIT_TAG)
  message(STATUS "Attempting to parse git tag: '${GIT_TAG}'")
  
  # 先移除 v 前缀（如果存在）
  string(REGEX REPLACE "^v" "" GIT_TAG_CLEAN "${GIT_TAG}")
  message(STATUS "Cleaned git tag: '${GIT_TAG_CLEAN}'")
  
  # 匹配版本号格式 MAJOR.MINOR.PATCH
  string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$" 
                     VERSION_MATCH 
                     "${GIT_TAG_CLEAN}")
  
  if(VERSION_MATCH)
    # 提取各个部分
    string(REGEX REPLACE "^([0-9]+)\\..*" "\\1" PROJECT_VERSION_MAJOR "${GIT_TAG_CLEAN}")
    string(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\..*" "\\1" PROJECT_VERSION_MINOR "${GIT_TAG_CLEAN}")
    string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)$" "\\1" PROJECT_VERSION_PATCH "${GIT_TAG_CLEAN}")
    message(STATUS "Regex match successful: '${VERSION_MATCH}'")
  else()
    message(STATUS "Regex match failed for tag: '${GIT_TAG_CLEAN}'")
    set(VERSION_MATCH "")
  endif()
  
  message(STATUS "Parsed version: MAJOR='${PROJECT_VERSION_MAJOR}', MINOR='${PROJECT_VERSION_MINOR}', PATCH='${PROJECT_VERSION_PATCH}'")
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

# 创建版本头文件（在构建时动态生成，确保版本信息总是最新的）
set(VERSION_HEADER_DIR "${CMAKE_BINARY_DIR}/generated")
set(VERSION_HEADER_FILE "${VERSION_HEADER_DIR}/firmware_version.h")

# 确保目录存在
file(MAKE_DIRECTORY ${VERSION_HEADER_DIR})

# 配置版本头文件模板
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/version.h.in
    ${VERSION_HEADER_FILE}
    @ONLY
)

# 添加版本头文件目录到包含路径
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${VERSION_HEADER_DIR})

# 添加版本定义（为了向后兼容，保留 add_definitions）
# 注意：使用引号确保宏值正确传递
add_definitions(
  -DFIRMWARE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  -DFIRMWARE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
  -DFIRMWARE_VERSION_PATCH=${PROJECT_VERSION_PATCH}
  -DFIRMWARE_VERSION_STRING="${VERSION_STRING}"
  -DFIRMWARE_BUILD_DATE="${BUILD_DATE}")

# 创建自定义目标，在每次构建前更新版本信息
add_custom_target(update_version ALL
    COMMAND ${CMAKE_COMMAND} -DCMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}
                             -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
                             -DVERSION_HEADER_FILE=${VERSION_HEADER_FILE}
                             -P ${CMAKE_SOURCE_DIR}/cmake/update_version.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Updating firmware version from git tag"
    VERBATIM
)

# 让主目标依赖于版本更新目标，确保每次构建都更新版本
add_dependencies(${CMAKE_PROJECT_NAME} update_version)

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