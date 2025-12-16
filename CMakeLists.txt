cmake_minimum_required(VERSION 3.10)
project(shentou LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

# 开启测试
enable_testing()

# 打印信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# 添加 core
add_subdirectory(src/core)

# --------------------------
# modules
# 可以通过 -DMODULE=xxx 指定只编译某个模块
# 如果没指定，则编译所有模块
# --------------------------
set(ALL_MODULES shentou)
if(DEFINED MODULE)
    message(STATUS "Building only module: ${MODULE}")
    if(NOT ${MODULE} IN_LIST ALL_MODULES)
        message(FATAL_ERROR "Module '${MODULE}' not recognized!")
    endif()
    add_subdirectory(src/modules/${MODULE})
else()
    message(STATUS "Building all modules")
    foreach(mod ${ALL_MODULES})
        add_subdirectory(src/modules/${mod})
    endforeach()
endif()

# 添加 tests
add_subdirectory(tests)
