#ifndef HCB_OPCODES_H
#define HCB_OPCODES_H

_OP_DEF opdef[] = {
    { 0x00, "nop", OPARG_NULL },
    { 0x01, "initstack", OPARG_I8I8 },
    { 0x02, "call", OPARG_X32 },
    { 0x03, "syscall", OPARG_I16 },
    { 0x04, "ret", OPARG_NULL },
    { 0x05, "ret2", OPARG_NULL },
    { 0x06, "jmp", OPARG_X32 },
    { 0x07, "jmpcond", OPARG_X32 },
    { 0x08, "pushtrue", OPARG_NULL },
    { 0x09, "pushfalse", OPARG_NULL },
    { 0x0A, "pushint", OPARG_I32 },
    { 0x0B, "pushint", OPARG_I16 },
    { 0x0C, "pushint", OPARG_I8 },
    { 0x0D, "pushfloat", OPARG_X32 }, /* unused in script */
    { 0x0E, "pushstring", OPARG_STRING },
    { 0x0F, "pushglobal", OPARG_I16 },
    { 0x10, "pushstack", OPARG_I8 },
    { 0x11, "unk_11", OPARG_I16 },
    { 0x12, "unk_12", OPARG_I8 },
    { 0x13, "pushtop", OPARG_NULL }, /* unused in script */
    { 0x14, "pushtemp", OPARG_NULL },
    { 0x15, "popglobal", OPARG_I16 },
    { 0x16, "copystack", OPARG_I8 },
    { 0x17, "unk_17", OPARG_I16 },
    { 0x18, "unk_18", OPARG_I8 },
    { 0x19, "neg", OPARG_NULL },
    { 0x1A, "add", OPARG_NULL },
    { 0x1B, "sub", OPARG_NULL },
    { 0x1C, "mul", OPARG_NULL },
    { 0x1D, "div", OPARG_NULL },
    { 0x1E, "mod", OPARG_NULL },
    { 0x1F, "test", OPARG_NULL },
    { 0x20, "logand", OPARG_NULL },
    { 0x21, "logor", OPARG_NULL }, //same as 0x20 but opposite
    { 0x22, "eq", OPARG_NULL },
    { 0x23, "neq", OPARG_NULL }, //same as 0x22 but opposite
    { 0x24, "gt", OPARG_NULL },
    { 0x25, "le", OPARG_NULL },
    { 0x26, "lt", OPARG_NULL }, //same as 0x25 but opposite
    { 0x27, "ge", OPARG_NULL }  //same as 0x24 but opposite
};
#endif // HCB_OPCODES_H

