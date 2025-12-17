#pragma once

typedef enum {
    // Constants
    OPCODE_NOP = 0x00,  // 00 
    OPCODE_ACONST_NULL = 0x01,  // 01 
    OPCODE_ICONST_M1 = 0x02,  // 02 
    OPCODE_ICONST_0 = 0x03,  // 03 
    OPCODE_ICONST_1 = 0x04,  // 04 
    OPCODE_ICONST_2 = 0x05,  // 05 
    OPCODE_ICONST_3 = 0x06,  // 06 
    OPCODE_ICONST_4 = 0x07,  // 07 
    OPCODE_ICONST_5 = 0x08,  // 08 
    OPCODE_LCONST_0 = 0x09,  // 09 
    OPCODE_LCONST_1 = 0x0a,  // 10 
    OPCODE_FCONST_0 = 0x0b,  // 11 
    OPCODE_FCONST_1 = 0x0c,  // 12 
    OPCODE_FCONST_2 = 0x0d,  // 13 
    OPCODE_DCONST_0 = 0x0e,  // 14 
    OPCODE_DCONST_1 = 0x0f,  // 15 
    OPCODE_BIPUSH = 0x10,  // 16 
    OPCODE_SIPUSH = 0x11,  // 17 
    OPCODE_LDC = 0x12,  // 18 
    OPCODE_LDC_W = 0x13,  // 19 
    OPCODE_LDC2_W = 0x14,  // 20 

    // Loads
} opcode_e;