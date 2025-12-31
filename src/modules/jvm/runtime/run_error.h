#pragma once

enum run_error_e{
    RUNTIME_ERROR_None = 0,
    RUNTIME_ERROR_NegativeArraySizeException = 1,
    RUNTIME_ERROR_NullPointerException,
    RUNTIME_ERROR_ArrayIndexOutOfBoundsException,


    RUNTIME_ERROR_RuntimeException = 99,
};