# get_build_version.cmake
# 在构建时获取最新版本信息并输出

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
set(BUILD_VERSION_MAJOR 0)
set(BUILD_VERSION_MINOR 0)
set(BUILD_VERSION_PATCH 0)
set(BUILD_VERSION_STRING "0.0.0")

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
            string(REGEX REPLACE "^([0-9]+)\\..*" "\\1" BUILD_VERSION_MAJOR "${GIT_TAG_CLEAN}")
            string(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\..*" "\\1" BUILD_VERSION_MINOR "${GIT_TAG_CLEAN}")
            string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)$" "\\1" BUILD_VERSION_PATCH "${GIT_TAG_CLEAN}")
            
            # 获取 commit 数量
            execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-list --count ${GIT_TAG}..HEAD
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE COMMIT_COUNT
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE)
            
            if(COMMIT_COUNT AND COMMIT_COUNT GREATER 0)
                math(EXPR BUILD_VERSION_PATCH "${BUILD_VERSION_PATCH} + ${COMMIT_COUNT}")
            endif()
            
            set(BUILD_VERSION_STRING "${BUILD_VERSION_MAJOR}.${BUILD_VERSION_MINOR}.${BUILD_VERSION_PATCH}")
        endif()
    endif()
endif()

# 获取构建日期
string(TIMESTAMP BUILD_DATE "%Y%m%d")

# 输出版本信息
message(STATUS "")
message(STATUS "==========================================")
message(STATUS "Firmware Build Complete")
message(STATUS "==========================================")
message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "Firmware Version: ${BUILD_VERSION_STRING}")
message(STATUS "  Major Version: ${BUILD_VERSION_MAJOR}")
message(STATUS "  Minor Version: ${BUILD_VERSION_MINOR}")
message(STATUS "  Patch Version: ${BUILD_VERSION_PATCH}")
message(STATUS "Build Date: ${BUILD_DATE}")
message(STATUS "Build Type: ${BUILD_TYPE}")
message(STATUS "UWB Chip Type: ${UWB_CHIP_TYPE}")
message(STATUS "==========================================")
message(STATUS "")

