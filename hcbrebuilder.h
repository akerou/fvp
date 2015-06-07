#ifndef HCBREBUILDER_H
#define HCBREBUILDER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "hcb_global.h"
extern _OP_DEF opdef[];

#define MAX_IDXCOUNT 8192
#define HASH_MODULO 257
#define HASH_BUCKETSIZE 128

class HcbRebuilder
{
public:
    HcbRebuilder() {}
    int rebuild(const char *txname, const char *hcbname);
private:

    typedef enum { IDX_JMP, IDX_FUNC, IDX_IMPORT, IDX_PTR } IDX_TYPE;

    typedef struct
    {
        char *name;
        int offset;
    } JMPOFFSET;

    typedef struct
    {
        unsigned char *hcbbuf, *hcbp;

        JMPOFFSET *labels[4];
        unsigned int numlabels[4];

        unsigned int *hashlabels;
        unsigned int *hashidnum;
        unsigned short *hashcounts;

        unsigned int numFunc;
        unsigned char *curFuncp;

        unsigned char *importp;

    } BUILD;

    int remapImportList(BUILD *b);
    int readFinalItems(BUILD *b, char *txp, char *txend);
    int scanPropertyLine(const char *propname, char *txp, char *txend, int *nextline);
    int parseLine(BUILD *b, char *txp, char *txend);
    int genOpcodeData(BUILD *b, char *txp, char *txend);
    int getOpcodeId(char *txp, const char *endp);
    int resolveJumpOffsets(BUILD *b, int mode, unsigned char *p, unsigned char *endp);
    int manageLabel(BUILD *b, char *val, int mode, IDX_TYPE type);
    unsigned int hashstring(unsigned char *s);
    int bytesUntilNextLine(const char *p, const char *endp);
    int bytesUntilCRLF(const char *p, const char *endp);
    int bytesWhitespace(const char *p, const char *endp);
};

#endif // HCBREBUILDER_H
