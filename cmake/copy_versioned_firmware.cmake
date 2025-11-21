# copy_versioned_firmware.cmake
# 在构建时复制固件文件到 bin 文件夹，并添加版本号到文件名

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

# 初始化版本变量
set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

# 获取 git tag
if(GIT_EXECUTABLE)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE GIT_TAG_RESULT)
    
    if(NOT GIT_TAG_RESULT AND GIT_TAG)
        # 移除 v 前缀
        string(REGEX REPLACE "^v" "" GIT_TAG_CLEAN "${GIT_TAG}")
        
        # 匹配版本号格式
        string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$" 
                           VERSION_MATCH 
                           "${GIT_TAG_CLEAN}")
        
        if(VERSION_MATCH)
            # 提取各个部分
            string(REGEX REPLACE "^([0-9]+)\\..*" "\\1" VERSION_MAJOR "${GIT_TAG_CLEAN}")
            string(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\..*" "\\1" VERSION_MINOR "${GIT_TAG_CLEAN}")
            string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)$" "\\1" VERSION_PATCH "${GIT_TAG_CLEAN}")
            
            # 获取 commit 数量
            execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-list --count ${GIT_TAG}..HEAD
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE COMMIT_COUNT
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE)
            
            if(COMMIT_COUNT AND COMMIT_COUNT GREATER 0)
                math(EXPR VERSION_PATCH "${VERSION_PATCH} + ${COMMIT_COUNT}")
            endif()
        endif()
    endif()
endif()

# 构建版本字符串，格式：v_0_10_2
set(VERSION_SUFFIX "v_${VERSION_MAJOR}_${VERSION_MINOR}_${VERSION_PATCH}")

# 使用传入的固件输出目录，如果没有则计算
# 注意：生成器表达式可能无法正确传递，所以优先使用计算值
if(NOT FIRMWARE_OUTPUT_DIR OR FIRMWARE_OUTPUT_DIR STREQUAL "")
    if(CMAKE_BUILD_TYPE)
        set(FIRMWARE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
    else()
        # 如果没有 CMAKE_BUILD_TYPE，尝试常见的目录
        if(EXISTS "${CMAKE_BINARY_DIR}/Debug")
            set(FIRMWARE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/Debug")
        elseif(EXISTS "${CMAKE_BINARY_DIR}/Release")
            set(FIRMWARE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/Release")
        else()
            set(FIRMWARE_OUTPUT_DIR "${CMAKE_BINARY_DIR}")
        endif()
    endif()
endif()

# 调试信息
message(STATUS "Firmware output directory: ${FIRMWARE_OUTPUT_DIR}")
message(STATUS "Version suffix: ${VERSION_SUFFIX}")

# 创建 bin 文件夹
set(BIN_DIR "${FIRMWARE_OUTPUT_DIR}/bin")
file(MAKE_DIRECTORY ${BIN_DIR})
message(STATUS "Created bin directory: ${BIN_DIR}")

# 复制并重命名主固件文件
set(SOURCE_BIN "${FIRMWARE_OUTPUT_DIR}/${PROJECT_NAME}.bin")
set(DEST_BIN "${BIN_DIR}/${PROJECT_NAME}_${VERSION_SUFFIX}.bin")

message(STATUS "Looking for source firmware: ${SOURCE_BIN}")

if(EXISTS ${SOURCE_BIN})
    message(STATUS "Found source firmware, copying...")
    file(COPY ${SOURCE_BIN} DESTINATION ${BIN_DIR})
    file(RENAME ${BIN_DIR}/${PROJECT_NAME}.bin ${DEST_BIN})
    message(STATUS "Successfully copied firmware to: ${DEST_BIN}")
else()
    message(WARNING "Source firmware file not found: ${SOURCE_BIN}")
    message(WARNING "Firmware output directory contents:")
    file(GLOB FILES_IN_DIR "${FIRMWARE_OUTPUT_DIR}/*")
    foreach(FILE ${FILES_IN_DIR})
        message(WARNING "  - ${FILE}")
    endforeach()
endif()

# 如果存在合并的固件文件，也复制它
# 合并固件在 CMAKE_BINARY_DIR 目录中（不是子目录）
set(SOURCE_COMBINED_BIN "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_combined.bin")
set(DEST_COMBINED_BIN "${BIN_DIR}/${PROJECT_NAME}_combined_${VERSION_SUFFIX}.bin")

message(STATUS "Looking for combined firmware: ${SOURCE_COMBINED_BIN}")

if(EXISTS ${SOURCE_COMBINED_BIN})
    message(STATUS "Found combined firmware, copying...")
    file(COPY ${SOURCE_COMBINED_BIN} DESTINATION ${BIN_DIR})
    file(RENAME ${BIN_DIR}/${PROJECT_NAME}_combined.bin ${DEST_COMBINED_BIN})
    message(STATUS "Successfully copied combined firmware to: ${DEST_COMBINED_BIN}")
else()
    message(STATUS "Combined firmware not found (this is OK if not using combined firmware)")
endif()

