#ifndef HCB_TOOL_H_INCLUDED
#define HCB_TOOL_H_INCLUDED

typedef enum { OPARG_NULL, OPARG_X32, OPARG_I32, OPARG_I16, OPARG_I8, OPARG_I8I8, OPARG_STRING } _OPARG;

#define HCB_LAST_OPCODE 0x27

typedef struct
{
    unsigned char opcode;
    const char *name;
    _OPARG params;
} _OP_DEF;

#endif // HCB_TOOL_H_INCLUDED
