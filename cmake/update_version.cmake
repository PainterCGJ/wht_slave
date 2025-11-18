# update_version.cmake
# 在构建时动态更新版本信息

# 初始化版本变量
set(FIRMWARE_VERSION_MAJOR 0)
set(FIRMWARE_VERSION_MINOR 0)
set(FIRMWARE_VERSION_PATCH 0)
set(FIRMWARE_VERSION_STRING "0.0.0")

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
            string(REGEX REPLACE "^([0-9]+)\\..*" "\\1" FIRMWARE_VERSION_MAJOR "${GIT_TAG_CLEAN}")
            string(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\..*" "\\1" FIRMWARE_VERSION_MINOR "${GIT_TAG_CLEAN}")
            string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)$" "\\1" FIRMWARE_VERSION_PATCH "${GIT_TAG_CLEAN}")
            
            # 获取 commit 数量
            execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-list --count ${GIT_TAG}..HEAD
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE COMMIT_COUNT
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE)
            
            if(COMMIT_COUNT AND COMMIT_COUNT GREATER 0)
                math(EXPR FIRMWARE_VERSION_PATCH "${FIRMWARE_VERSION_PATCH} + ${COMMIT_COUNT}")
            endif()
            
            set(FIRMWARE_VERSION_STRING "${FIRMWARE_VERSION_MAJOR}.${FIRMWARE_VERSION_MINOR}.${FIRMWARE_VERSION_PATCH}")
        endif()
    endif()
endif()

# 获取构建日期
string(TIMESTAMP BUILD_DATE "%Y%m%d")

# 配置版本头文件
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/version.h.in
    ${VERSION_HEADER_FILE}
    @ONLY
)

message(STATUS "Version updated: ${FIRMWARE_VERSION_STRING} (from git tag)")

