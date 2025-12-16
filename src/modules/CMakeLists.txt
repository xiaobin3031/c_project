# modules 子目录
set(MODULES_DIRS shentou)

foreach(mod ${MODULES_DIRS})
    add_subdirectory(${mod})
endforeach()
