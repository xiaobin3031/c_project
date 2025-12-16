# 所有 core 源文件
file(GLOB_RECURSE CORE_SRC "*.c")

# 编译 core 静态库
add_library(core STATIC ${CORE_SRC})

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# core 的 main 可能在 modules/core_test，所以不生成可执行文件
