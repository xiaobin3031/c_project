#pragma once

typedef enum {
    // Constants
    OPCODE_nop = 0x00,  // 00 
    OPCODE_aconst_null = 0x01,  // 01 
    OPCODE_iconst_m1 = 0x02,  // 02 
    OPCODE_iconst_0 = 0x03,  // 03 
    OPCODE_iconst_1 = 0x04,  // 04 
    OPCODE_iconst_2 = 0x05,  // 05 
    OPCODE_iconst_3 = 0x06,  // 06 
    OPCODE_iconst_4 = 0x07,  // 07 
    OPCODE_iconst_5 = 0x08,  // 08 
    OPCODE_lconst_0 = 0x09,  // 09 
    OPCODE_lconst_1 = 0x0a,  // 10 
    OPCODE_fconst_0 = 0x0b,  // 11 
    OPCODE_fconst_1 = 0x0c,  // 12 
    OPCODE_fconst_2 = 0x0d,  // 13 
    OPCODE_dconst_0 = 0x0e,  // 14 
    OPCODE_dconst_1 = 0x0f,  // 15 
    OPCODE_bipush = 0x10,  // 16 
    OPCODE_sipush = 0x11,  // 17 
    OPCODE_ldc = 0x12,  // 18 
    OPCODE_ldc_w = 0x13,  // 19 
    OPCODE_ldc2_w = 0x14,  // 20 

    // Loads
    OPCODE_iload = 0x15,     // 21 
    OPCODE_lload = 0x16,     // 22 
    OPCODE_fload = 0x17,     // 23 
    OPCODE_dload = 0x18,     // 24 
    OPCODE_aload = 0x19,     // 25 
    OPCODE_iload_0 = 0x1a,     // 26 
    OPCODE_iload_1 = 0x1b,     // 27 
    OPCODE_iload_2 = 0x1c,     // 28 
    OPCODE_iload_3 = 0x1d,     // 29 
    OPCODE_lload_0 = 0x1e,     // 30 
    OPCODE_lload_1 = 0x1f,     // 31 
    OPCODE_lload_2 = 0x20,     // 32 
    OPCODE_lload_3 = 0x21,     // 33 
    OPCODE_fload_0 = 0x22,     // 34 
    OPCODE_fload_1 = 0x23,     // 35 
    OPCODE_fload_2 = 0x24,     // 36 
    OPCODE_fload_3 = 0x25,     // 37 
    OPCODE_dload_0 = 0x26,     // 38 
    OPCODE_dload_1 = 0x27,     // 39 
    OPCODE_dload_2 = 0x28,     // 40 
    OPCODE_dload_3 = 0x29,     // 41 
    OPCODE_aload_0 = 0x2a,     // 42 
    OPCODE_aload_1 = 0x2b,     // 43 
    OPCODE_aload_2 = 0x2c,     // 44 
    OPCODE_aload_3 = 0x2d,     // 45 
    OPCODE_iaload = 0x2e,     // 46 
    OPCODE_laload = 0x2f,     // 47 
    OPCODE_faload = 0x30,     // 48 
    OPCODE_daload = 0x31,     // 49 
    OPCODE_aaload = 0x32,     // 50 
    OPCODE_baload = 0x33,     // 51 
    OPCODE_caload = 0x34,     // 52 
    OPCODE_saload = 0x35,     // 53 

    // Stores
    OPCODE_istore = 0x36,       // 54 
    OPCODE_lstore = 0x37,       // 55 
    OPCODE_fstore = 0x38,       // 56 
    OPCODE_dstore = 0x39,       // 57 
    OPCODE_astore = 0x3a,       // 58 
    OPCODE_istore_0 = 0x3b,       // 59 
    OPCODE_istore_1 = 0x3c,       // 60 
    OPCODE_istore_2 = 0x3d,       // 61 
    OPCODE_istore_3 = 0x3e,       // 62 
    OPCODE_lstore_0 = 0x3f,       // 63 
    OPCODE_lstore_1 = 0x40,       // 64 
    OPCODE_lstore_2 = 0x41,       // 65 
    OPCODE_lstore_3 = 0x42,       // 66 
    OPCODE_fstore_0 = 0x43,       // 67 
    OPCODE_fstore_1 = 0x44,       // 68 
    OPCODE_fstore_2 = 0x45,       // 69 
    OPCODE_fstore_3 = 0x46,       // 70 
    OPCODE_dstore_0 = 0x47,       // 71 
    OPCODE_dstore_1 = 0x48,       // 72 
    OPCODE_dstore_2 = 0x49,       // 73 
    OPCODE_dstore_3 = 0x4a,       // 74 
    OPCODE_astore_0 = 0x4b,       // 75 
    OPCODE_astore_1 = 0x4c,       // 76 
    OPCODE_astore_2 = 0x4d,       // 77 
    OPCODE_astore_3 = 0x4e,       // 78 
    OPCODE_iastore = 0x4f,       // 79 
    OPCODE_lastore = 0x50,       // 80 
    OPCODE_fastore = 0x51,       // 81 
    OPCODE_dastore = 0x52,       // 82 
    OPCODE_aastore = 0x53,       // 83 
    OPCODE_bastore = 0x54,       // 84 
    OPCODE_castore = 0x55,       // 85 
    OPCODE_sastore = 0x56,       // 86

    // Stack
    OPCODE_pop = 0x57,      // 87 
    OPCODE_pop2 = 0x58,      // 88 
    OPCODE_dup = 0x59,      // 89 
    OPCODE_dup_x1 = 0x5a,      // 90 
    OPCODE_dup_x2 = 0x5b,      // 91 
    OPCODE_dup2 = 0x5c,      // 92 
    OPCODE_dup2_x1 = 0x5d,      // 93 
    OPCODE_dup2_x2 = 0x5e,      // 94 
    OPCODE_swap = 0x5f,      // 95 

    // Math
    OPCODE_iadd = 0x60,       // 96 
    OPCODE_ladd = 0x61,       // 97 
    OPCODE_fadd = 0x62,       // 98 
    OPCODE_dadd = 0x63,       // 99 
    OPCODE_isub = 0x64,       // 100 
    OPCODE_lsub = 0x65,       // 101 
    OPCODE_fsub = 0x66,       // 102 
    OPCODE_dsub = 0x67,       // 103 
    OPCODE_imul = 0x68,       // 104 
    OPCODE_lmul = 0x69,       // 105 
    OPCODE_fmul = 0x6a,       // 106 
    OPCODE_dmul = 0x6b,       // 107 
    OPCODE_idiv = 0x6c,       // 108 
    OPCODE_ldiv = 0x6d,       // 109 
    OPCODE_fdiv = 0x6e,       // 110 
    OPCODE_ddiv = 0x6f,       // 111 
    OPCODE_irem = 0x70,       // 112 
    OPCODE_lrem = 0x71,       // 113 
    OPCODE_frem = 0x72,       // 114 
    OPCODE_drem = 0x73,       // 115 
    OPCODE_ineg = 0x74,       // 116 
    OPCODE_lneg = 0x75,       // 117 
    OPCODE_fneg = 0x76,       // 118 
    OPCODE_dneg = 0x77,       // 119 
    OPCODE_ishl = 0x78,       // 120 
    OPCODE_lshl = 0x79,       // 121 
    OPCODE_ishr = 0x7a,       // 122 
    OPCODE_lshr = 0x7b,       // 123 
    OPCODE_iushr = 0x7c,       // 124 
    OPCODE_lushr = 0x7d,       // 125 
    OPCODE_iand = 0x7e,       // 126 
    OPCODE_land = 0x7f,       // 127 
    OPCODE_ior = 0x80,       // 128 
    OPCODE_lor = 0x81,       // 129 
    OPCODE_ixor = 0x82,       // 130 
    OPCODE_lxor = 0x83,       // 131 
    OPCODE_iinc = 0x84,       // 132 

    // Conversions
    OPCODE_i2l = 0x85,      // 133 
    OPCODE_i2f = 0x86,      // 134 
    OPCODE_i2d = 0x87,      // 135 
    OPCODE_l2i = 0x88,      // 136 
    OPCODE_l2f = 0x89,      // 137 
    OPCODE_l2d = 0x8a,      // 138 
    OPCODE_f2i = 0x8b,      // 139 
    OPCODE_f2l = 0x8c,      // 140 
    OPCODE_f2d = 0x8d,      // 141 
    OPCODE_d2i = 0x8e,      // 142 
    OPCODE_d2l = 0x8f,      // 143 
    OPCODE_d2f = 0x90,      // 144 
    OPCODE_i2b = 0x91,      // 145 
    OPCODE_i2c = 0x92,      // 146 
    OPCODE_i2s = 0x93,      // 147 

    // Comparisons
    OPCODE_lcmp = 0x94,       // 148 
    OPCODE_fcmpl = 0x95,       // 149 
    OPCODE_fcmpg = 0x96,       // 150 
    OPCODE_dcmpl = 0x97,       // 151 
    OPCODE_dcmpg = 0x98,       // 152 
    OPCODE_ifeq = 0x99,       // 153 
    OPCODE_ifne = 0x9a,       // 154 
    OPCODE_iflt = 0x9b,       // 155 
    OPCODE_ifge = 0x9c,       // 156 
    OPCODE_ifgt = 0x9d,       // 157 
    OPCODE_ifle = 0x9e,       // 158 
    OPCODE_if_icmpeq = 0x9f,       // 159 
    OPCODE_if_icmpne = 0xa0,       // 160 
    OPCODE_if_icmplt = 0xa1,       // 161 
    OPCODE_if_icmpge = 0xa2,       // 162 
    OPCODE_if_icmpgt = 0xa3,       // 163 
    OPCODE_if_icmple = 0xa4,       // 164 
    OPCODE_if_acmpeq = 0xa5,       // 165 
    OPCODE_if_acmpne = 0xa6,       // 166 

    // References
    OPCODE_getstatic = 0xb2,       // 178
    OPCODE_putstatic = 0xb3,       // 179
    OPCODE_getfield = 0xb4,       // 180
    OPCODE_putfield = 0xb5,       // 181
    OPCODE_invokevirtual = 0xb6,       // 182
    OPCODE_invokespecial = 0xb7,       // 183
    OPCODE_invokestatic = 0xb8,       // 184
    OPCODE_invokeinterface = 0xb9,       // 185
    OPCODE_invokedynamic = 0xba,       // 186
    OPCODE_new = 0xbb,       // 187
    OPCODE_newarray = 0xbc,       // 188
    OPCODE_anewarray = 0xbd,       // 189
    OPCODE_arraylength = 0xbe,       // 190
    OPCODE_athrow = 0xbf,       // 191
    OPCODE_checkcast = 0xc0,       // 192
    OPCODE_instanceof = 0xc1,       // 193
    OPCODE_monitorenter = 0xc2,       // 194
    OPCODE_monitorexit = 0xc3,       // 195

    // Control
    OPCODE_goto = 0xa7,       // 167 
    OPCODE_jsr = 0xa8,       // 168 
    OPCODE_ret = 0xa9,       // 169 
    OPCODE_tableswitch = 0xaa,       // 170 
    OPCODE_lookupswitch = 0xab,       // 171 
    OPCODE_ireturn = 0xac,       // 172 
    OPCODE_lreturn = 0xad,       // 173 
    OPCODE_freturn = 0xae,       // 174 
    OPCODE_dreturn = 0xaf,       // 175 
    OPCODE_areturn = 0xb0,       // 176 
    OPCODE_return = 0xb1,       // 177 
    
    // Extended
    OPCODE_wide = 0xc4,      // 196 
    OPCODE_multianewarray = 0xc5,      // 197 
    OPCODE_ifnull = 0xc6,      // 198 
    OPCODE_ifnonnull = 0xc7,      // 199 
    OPCODE_goto_w = 0xc8,      // 200 
    OPCODE_jsr_w = 0xc9,      // 201 

    // Reserved
    OPCODE_breakpoint = 0xca,     // 202 
    OPCODE_impdep1 = 0xfe,     // 254 
    OPCODE_impdep2 = 0xff,     // 255 
} opcode_e;