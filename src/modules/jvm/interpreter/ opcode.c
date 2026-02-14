#include "opcode.h"
#include "../utils/bytes.h"

char *opcode_to_string(u2 opcode) {
    switch(opcode) {
                // Constants
        case 0x00: return "nop";  // 00 
        case 0x01: return "aconst_null";  // 01 
        case 0x02: return "iconst_m1";  // 02 
        case 0x03: return "iconst_0";  // 03 
        case 0x04: return "iconst_1";  // 04 
        case 0x05: return "iconst_2";  // 05 
        case 0x06: return "iconst_3";  // 06 
        case 0x07: return "iconst_4";  // 07 
        case 0x08: return "iconst_5";  // 08 
        case 0x09: return "lconst_0";  // 09 
        case 0x0a: return "lconst_1";  // 10 
        case 0x0b: return "fconst_0";  // 11 
        case 0x0c: return "fconst_1";  // 12 
        case 0x0d: return "fconst_2";  // 13 
        case 0x0e: return "dconst_0";  // 14 
        case 0x0f: return "dconst_1";  // 15 
        case 0x10: return "bipush";  // 16 
        case 0x11: return "sipush";  // 17 
        case 0x12: return "ldc";  // 18 
        case 0x13: return "ldc_w";  // 19 
        case 0x14: return "ldc2_w";  // 20 

        // Loads
        case 0x15: return "iload";  // 21 
        case 0x16: return "lload";  // 22 
        case 0x17: return "fload";  // 23 
        case 0x18: return "dload";  // 24 
        case 0x19: return "aload";  // 25 
        case 0x1a: return "iload_0";  // 26 
        case 0x1b: return "iload_1";  // 27 
        case 0x1c: return "iload_2";  // 28 
        case 0x1d: return "iload_3";  // 29 
        case 0x1e: return "lload_0";  // 30 
        case 0x1f: return "lload_1";  // 31 
        case 0x20: return "lload_2";  // 32 
        case 0x21: return "lload_3";  // 33 
        case 0x22: return "fload_0";  // 34 
        case 0x23: return "fload_1";  // 35 
        case 0x24: return "fload_2";  // 36 
        case 0x25: return "fload_3";  // 37 
        case 0x26: return "dload_0";  // 38 
        case 0x27: return "dload_1";  // 39 
        case 0x28: return "dload_2";  // 40 
        case 0x29: return "dload_3";  // 41 
        case 0x2a: return "aload_0";  // 42 
        case 0x2b: return "aload_1";  // 43 
        case 0x2c: return "aload_2";  // 44 
        case 0x2d: return "aload_3";  // 45 
        case 0x2e: return "iaload";  // 46 
        case 0x2f: return "laload";  // 47 
        case 0x30: return "faload";  // 48 
        case 0x31: return "daload";  // 49 
        case 0x32: return "aaload";  // 50 
        case 0x33: return "baload";  // 51 
        case 0x34: return "caload";  // 52 
        case 0x35: return "saload";  // 53 

        // Stores
        case 0x36: return "istore";  // 54 
        case 0x37: return "lstore";  // 55 
        case 0x38: return "fstore";  // 56 
        case 0x39: return "dstore";  // 57 
        case 0x3a: return "astore";  // 58 
        case 0x3b: return "istore_0";  // 59 
        case 0x3c: return "istore_1";  // 60 
        case 0x3d: return "istore_2";  // 61 
        case 0x3e: return "istore_3";  // 62 
        case 0x3f: return "lstore_0";  // 63 
        case 0x40: return "lstore_1";  // 64 
        case 0x41: return "lstore_2";  // 65 
        case 0x42: return "lstore_3";  // 66 
        case 0x43: return "fstore_0";  // 67 
        case 0x44: return "fstore_1";  // 68 
        case 0x45: return "fstore_2";  // 69 
        case 0x46: return "fstore_3";  // 70 
        case 0x47: return "dstore_0";  // 71 
        case 0x48: return "dstore_1";  // 72 
        case 0x49: return "dstore_2";  // 73 
        case 0x4a: return "dstore_3";  // 74 
        case 0x4b: return "astore_0";  // 75 
        case 0x4c: return "astore_1";  // 76 
        case 0x4d: return "astore_2";  // 77 
        case 0x4e: return "astore_3";  // 78 
        case 0x4f: return "iastore";  // 79 
        case 0x50: return "lastore";  // 80 
        case 0x51: return "fastore";  // 81 
        case 0x52: return "dastore";  // 82 
        case 0x53: return "aastore";  // 83 
        case 0x54: return "bastore";  // 84 
        case 0x55: return "castore";  // 85 
        case 0x56: return "sastore";  // 86

        // Stack
        case 0x57: return "pop";  // 87 
        case 0x58: return "pop2";  // 88 
        case 0x59: return "dup";  // 89 
        case 0x5a: return "dup_x1";  // 90 
        case 0x5b: return "dup_x2";  // 91 
        case 0x5c: return "dup2";  // 92 
        case 0x5d: return "dup2_x1";  // 93 
        case 0x5e: return "dup2_x2";  // 94 
        case 0x5f: return "swap";  // 95 

        // Math
        case 0x60: return "iadd";  // 96 
        case 0x61: return "ladd";  // 97 
        case 0x62: return "fadd";  // 98 
        case 0x63: return "dadd";  // 99 
        case 0x64: return "isub";  // 100 
        case 0x65: return "lsub";  // 101 
        case 0x66: return "fsub";  // 102 
        case 0x67: return "dsub";  // 103 
        case 0x68: return "imul";  // 104 
        case 0x69: return "lmul";  // 105 
        case 0x6a: return "fmul";  // 106 
        case 0x6b: return "dmul";  // 107 
        case 0x6c: return "idiv";  // 108 
        case 0x6d: return "ldiv";  // 109 
        case 0x6e: return "fdiv";  // 110 
        case 0x6f: return "ddiv";  // 111 
        case 0x70: return "irem";  // 112 
        case 0x71: return "lrem";  // 113 
        case 0x72: return "frem";  // 114 
        case 0x73: return "drem";  // 115 
        case 0x74: return "ineg";  // 116 
        case 0x75: return "lneg";  // 117 
        case 0x76: return "fneg";  // 118 
        case 0x77: return "dneg";  // 119 
        case 0x78: return "ishl";  // 120 
        case 0x79: return "lshl";  // 121 
        case 0x7a: return "ishr";  // 122 
        case 0x7b: return "lshr";  // 123 
        case 0x7c: return "iushr";  // 124 
        case 0x7d: return "lushr";  // 125 
        case 0x7e: return "iand";  // 126 
        case 0x7f: return "land";  // 127 
        case 0x80: return "ior";  // 128 
        case 0x81: return "lor";  // 129 
        case 0x82: return "ixor";  // 130 
        case 0x83: return "lxor";  // 131 
        case 0x84: return "iinc";  // 132 

        // Conversions
        case 0x85: return "i2l";  // 133 
        case 0x86: return "i2f";  // 134 
        case 0x87: return "i2d";  // 135 
        case 0x88: return "l2i";  // 136 
        case 0x89: return "l2f";  // 137 
        case 0x8a: return "l2d";  // 138 
        case 0x8b: return "f2i";  // 139 
        case 0x8c: return "f2l";  // 140 
        case 0x8d: return "f2d";  // 141 
        case 0x8e: return "d2i";  // 142 
        case 0x8f: return "d2l";  // 143 
        case 0x90: return "d2f";  // 144 
        case 0x91: return "i2b";  // 145 
        case 0x92: return "i2c";  // 146 
        case 0x93: return "i2s";  // 147 

        // Comparisons
        case 0x94: return "lcmp";  // 148 
        case 0x95: return "fcmpl";  // 149 
        case 0x96: return "fcmpg";  // 150 
        case 0x97: return "dcmpl";  // 151 
        case 0x98: return "dcmpg";  // 152 
        case 0x99: return "ifeq";  // 153 
        case 0x9a: return "ifne";  // 154 
        case 0x9b: return "iflt";  // 155 
        case 0x9c: return "ifge";  // 156 
        case 0x9d: return "ifgt";  // 157 
        case 0x9e: return "ifle";  // 158 
        case 0x9f: return "if_icmpeq";  // 159 
        case 0xa0: return "if_icmpne";  // 160 
        case 0xa1: return "if_icmplt";  // 161 
        case 0xa2: return "if_icmpge";  // 162 
        case 0xa3: return "if_icmpgt";  // 163 
        case 0xa4: return "if_icmple";  // 164 
        case 0xa5: return "if_acmpeq";  // 165 
        case 0xa6: return "if_acmpne";  // 166 

        // References
        case 0xb2: return "getstatic";  // 178
        case 0xb3: return "putstatic";  // 179
        case 0xb4: return "getfield";  // 180
        case 0xb5: return "putfield";  // 181
        case 0xb6: return "invokevirtual";  // 182
        case 0xb7: return "invokespecial";  // 183
        case 0xb8: return "invokestatic";  // 184
        case 0xb9: return "invokeinterface";  // 185
        case 0xba: return "invokedynamic";  // 186
        case 0xbb: return "new";  // 187
        case 0xbc: return "newarray";  // 188
        case 0xbd: return "anewarray";  // 189
        case 0xbe: return "arraylength";  // 190
        case 0xbf: return "athrow";  // 191
        case 0xc0: return "checkcast";  // 192
        case 0xc1: return "instanceof";  // 193
        case 0xc2: return "monitorenter";  // 194
        case 0xc3: return "monitorexit";  // 195

        // Control
        case 0xa7: return "goto";  // 167 
        case 0xa8: return "jsr";  // 168 
        case 0xa9: return "ret";  // 169 
        case 0xaa: return "tableswitch";  // 170 
        case 0xab: return "lookupswitch";  // 171 
        case 0xac: return "ireturn";  // 172 
        case 0xad: return "lreturn";  // 173 
        case 0xae: return "freturn";  // 174 
        case 0xaf: return "dreturn";  // 175 
        case 0xb0: return "areturn";  // 176 
        case 0xb1: return "return";  // 177 
        
        // Extended
        case 0xc4: return "wide";  // 196 
        case 0xc5: return "multianewarray";  // 197 
        case 0xc6: return "ifnull";  // 198 
        case 0xc7: return "ifnonnull";  // 199 
        case 0xc8: return "goto_w";  // 200 
        case 0xc9: return "jsr_w";  // 201 

        // Reserved
        case 0xca: return "breakpoint";  // 202 
        case 0xfe: return "impdep1";  // 254 
        case 0xff: return "impdep2";  // 255 
        default: return "not match";
    };
}