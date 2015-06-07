#ifndef HCBDECODER_H
#define HCBDECODER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hcb_global.h>

#define MAX_IDXCOUNT 8192
#define VMSTACK_SIZE 16
#define MAX_SPEAKFUNC 50
#define OPHIST 6

extern _OP_DEF opdef[];

class HcbDecoder
{
public:
    HcbDecoder() {}
    int decode(const char *hcbname, const char *outname);

private:

    typedef enum { T_TRUE, T_FALSE, T_INTEGER, T_FLOAT, T_STRING, T_RET } VM_TYPE;

    typedef struct
    {
        unsigned char opcode;
        const char *name;
        _OPARG params;
    } _OP_DEF;

    typedef struct
    {
        unsigned char *hcbbuf;
        unsigned char *p; //instruction pointer

        int sp; //stack position
        int framenum;

        int status;
        int qscan_state;

        struct
        {
            VM_TYPE type;
            int data;
        } stack[VMSTACK_SIZE];

        struct
        {
            int orig_sp;
            unsigned char *retp;
        } frame[3];

    } VM_CONTEXT;

    int getFuncIdFromOffset(unsigned int offset, unsigned int *func_index, int numfunc, int exact);
    int indexFunctions(const unsigned char *buf, const unsigned char *p, const unsigned char *endcodep, unsigned int *idx, unsigned int *counted, int mode, const char *codemap);
    int decode_op(const unsigned char *p);
    const char *funcAlias(int funcId, int funcCount, unsigned char *buf, unsigned char *p);
    void getSysImports(unsigned char *buf, char ***sys_imports);

    FILE *outfile;
    int txt_start;
    char g_temp[32];
    int speakfuncs[MAX_SPEAKFUNC], speak_count;
    const char *getSpeakerName(unsigned char *hcbbuf, unsigned char *p);
    void vm_execute(VM_CONTEXT *c);
    void nothing(const char *x...);
    void quickscan(VM_CONTEXT *c);
    int isSpeakFunction(unsigned char *hcbbuf, unsigned int offset);
    const char *getBgName(unsigned char *hcbbuf, unsigned char *p);
    void findSpeakFuncs(unsigned char *hcbbuf, unsigned int *func_index, unsigned int func_count);
    void vm_init(VM_CONTEXT *context, unsigned char *hcbbuf, unsigned char *p);
};

#endif // HCBDECODER_H
