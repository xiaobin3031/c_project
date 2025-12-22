# shentou 模块源
file(GLOB_RECURSE JVM_SRC "*.c")

add_library(jvm STATIC ${JVM_SRC})
target_include_directories(jvm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_options(jvm PRIVATE -rdynamic)

# 如果模块 main 需要生成可执行文件
add_executable(jvm_main main.c)
target_link_libraries(jvm_main jvm core pthread)