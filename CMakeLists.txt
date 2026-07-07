cmake_minimum_required(VERSION 3.12)

project(memo)

if(MSVC)
    # 关闭 /showIncludes
    string(REPLACE "/showIncludes" "" CMAKE_DEPFILE_FLAGS_C "${CMAKE_DEPFILE_FLAGS_C}")
    string(REPLACE "/showIncludes" "" CMAKE_DEPFILE_FLAGS_CXX "${CMAKE_DEPFILE_FLAGS_CXX}")
endif()
set(CMAKE_CXX_STANDARD 17)

#启用cmake查找路径调试
#set(CMAKE_FIND_DEBUG_MODE TRUE)

if (WIN32)
    #编译使用utf8文件
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
        # 为Release添加调试信息
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
    endif ()
endif()


add_compile_definitions(ROOT_PROJECT_PATH="${CMAKE_SOURCE_DIR}")
add_compile_definitions(TEST=false)

############################# projects defines #############################

add_subdirectory(src)


