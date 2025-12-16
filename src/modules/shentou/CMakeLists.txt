# shentou 模块源
find_package(CURL REQUIRED)
find_package(ZLIB REQUIRED)
file(GLOB SHENTOU_SRC "*.c")

add_library(shentou STATIC ${SHENTOU_SRC})
target_include_directories(shentou PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(shentou PUBLIC CURL::libcurl ZLIB::ZLIB)

# 如果模块 main 需要生成可执行文件
add_executable(shentou_main main.c)
target_link_libraries(shentou_main shentou core pthread)
