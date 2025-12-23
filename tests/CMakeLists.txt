# tests/CMakeLists.txt

if(DEFINED MODULE)
    message(STATUS "Building tests only for module: ${MODULE}")
    set(TEST_MODULES core ${MODULE})
else()
    set(TEST_MODULES core shentou)
endif()

foreach(tm ${TEST_MODULES})
    file(GLOB TM_TESTS "${tm}/*.c")
    if(TM_TESTS)
        add_executable(test_${tm} ${TM_TESTS} "./test_runner.c")
        target_include_directories(test_${tm} PRIVATE ../src ./)
        target_link_libraries(test_${tm} pthread ${tm} core)
        add_test(NAME ${tm}_tests COMMAND test_${tm})
    endif()
endforeach()
