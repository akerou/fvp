#include "hcbrebuilder.h"

int HcbRebuilder::bytesWhitespace(const char *p, const char *endp)
{
    const char *origp = p;

    while ((*p == ' ' || *p == '\t') && (p < endp)) { p++; }

    return p - origp;
}

int HcbRebuilder::bytesUntilCRLF(const char *p, const char *endp)
{
    const char *origp = p;

    while ((*p != '\r' && *p != '\n' && *p != 0) && (p < endp)) { p++; }

    return p - origp;
}

int HcbRebuilder::bytesUntilNextLine(const char *p, const char *endp)
{
    const char *origp = p;

    p += bytesUntilCRLF(p, endp);
    if (*p == '\r') { p++; }
    if (*p == '\n') { p++; }

    return p - origp;
}

unsigned int HcbRebuilder::hashstring(unsigned char *s)
{
    unsigned int hash = 0;

    while (*s)
    {
        hash = hash * 3 + *s * 37;
        s++;
    }

    return hash;
}

int HcbRebuilder::manageLabel(BUILD *b, char *val, int mode, IDX_TYPE type)
{
    /*
        mode 0: find label by name
        mode 1: find label by offset (cast "val" to int)
        mode 2: add new label (fail if label already exists)
        mode 3: add new label (return existing id when it exists)
    */
    unsigned int i, hash, bucket;
    JMPOFFSET *idx = b->labels[type];

    hash = 0;
    bucket = 0;

    if (mode == 0)
    {
        for (i = 0; i < b->numlabels[type]; i++) { if (strcmp(val, idx[i].name) == 0) { return i; } }
    }
    else if (mode == 1)
    {
        for (i = 0; i < b->numlabels[type]; i++) { if (idx[i].offset == (int)val) { return i; } }
    }
    else if (mode == 2 || mode == 3)
    {
        //check if label already exists
        if (type == IDX_FUNC)
        {
            //use string hash to quickly find function name
            hash = hashstring((unsigned char*)val);
            bucket = hash % HASH_MODULO;

            for (i = 0; i < b->hashcounts[bucket]; i++)
            {
                unsigned int num = b->hashidnum[bucket * HASH_BUCKETSIZE + i];
                if (strcmp(val, idx[num].name) == 0) { return (mode == 3) ? num : -1; }
            }
        }

        for (i = 0; i < b->numlabels[type]; i++) { if (strcmp(val, idx[i].name) == 0) { return (mode == 3) ? i : -1; } }

        if (b->numlabels[type] >= MAX_IDXCOUNT || (type == IDX_IMPORT && b->numlabels[IDX_IMPORT] >= 256))
        {
            printf("internal error: exceeded indexing capacity (mode %d)\n", mode);
            return -2;
        }

        //add new label
        idx[i].name = val;
        idx[i].offset = 0; //will be initialized when actual label is found

        if (type == IDX_FUNC)
        {
            if (b->hashcounts[bucket] >= HASH_BUCKETSIZE)
            {
                printf("internal error: exceeded hash indexing capacity\n");
                return -2;
            }

            b->hashlabels[bucket * HASH_BUCKETSIZE + b->hashcounts[bucket]] = hash;
            b->hashidnum[bucket * HASH_BUCKETSIZE + b->hashcounts[bucket]] = b->numlabels[type];
            b->hashcounts[bucket]++;
        }

        b->numlabels[type]++;

        //if (strcmp(val, "function_2787_") == 0 || strcmp(val, "function_2801_") == 0) printf("%s : %u\n", val, i);

        return i;
    }

    return -1; //failed
}

int HcbRebuilder::resolveJumpOffsets(BUILD *b, int mode, unsigned char *p, unsigned char *endp)
{
    /*
        mode 0: only fix jmp
        mode 1: only fix call and syscall
    */
    JMPOFFSET *idx = b->labels[mode];
    JMPOFFSET *imp = b->labels[IDX_IMPORT];

    while (p < endp)
    {
        unsigned int oplen;

        if ((mode == 0 && (*p == 0x06 || *p == 0x07)) ||
            (mode == 1 && (*p == 0x02)) )
        {
            int *n = (int*)(p + 1);

            if (idx[*n].offset == 0)
            {
                printf("error: label %s was not defined\n", idx[*n].name);
                return -1;
            }

            *n = idx[*n].offset;
        }
        else if (mode == 1 && *p == 0x03)
        {
            unsigned short *n = (unsigned short*)(p + 1);
            *n = imp[256 + *n].offset;
        }

        switch (opdef[*p].params)
        {
            case OPARG_NULL: oplen = 1; break;
            case OPARG_X32: oplen = 5; break;
            case OPARG_I32: oplen = 5; break;
            case OPARG_I16: oplen = 3; break;
            case OPARG_I8I8: oplen = 3; break;
            case OPARG_I8: oplen = 2; break;
            case OPARG_STRING: oplen = 2 + p[1]; break;
            default: printf("internal error: unknown param type\n"); return -2;
        }

        p += oplen;
    }

    if (mode == 1)
    {
        //resolve label pointers contained as an integer
        unsigned int i;
        int *n;

        idx = b->labels[IDX_FUNC];

        for (i = 0; i < b->numlabels[IDX_PTR]; i++)
        {
            n = (int*)(b->hcbbuf + b->labels[IDX_PTR][i].offset);
            if (*n == 0) { printf("error: label %s was not defined\n", idx[i].name); return -1; }

            *n = idx[*n].offset;
        }
    }

    return 0;
}

int HcbRebuilder::getOpcodeId(char *txp, const char *endp)
{
    int i, oplen, linelen;

    //scan for length of op name
    linelen = endp - txp;

    for (oplen = 0; oplen < linelen; oplen++)
    {
        if (txp[oplen] == ' ' || txp[oplen] == '\r' || txp[oplen] == '\n') { break; }
    }

    //locate opcode from table
    for (i = 0; i <= HCB_LAST_OPCODE; i++)
    {
        if (oplen == (int)strlen(opdef[i].name) && memcmp(txp, opdef[i].name, oplen) == 0) { return i; }
    }

    txp[oplen] = 0;
    printf("error: unrecognized opcode name: %s\n", txp);

    return -1; //failed
}

int HcbRebuilder::genOpcodeData(BUILD *b, char *txp, char *txend)
{
    unsigned char *origp;
    int linelen, done;

    done = 0;
    origp = b->hcbp;
    linelen = bytesUntilCRLF(txp, txend);
    txp[linelen] = 0;

    b->hcbp++; //move past opcode

    //handle opcodes that involve labels
    if (*origp == 0x01)
    {
        int labelId, flabelId;

        //make sure label exists at current offset
        labelId = manageLabel(b, (char*)(origp - b->hcbbuf), 1, IDX_JMP);
        if (labelId < 0) { printf("error: function started without a label\n"); return -1; }

        //add new function label and reset jmp labels
        flabelId = manageLabel(b, b->labels[IDX_JMP][labelId].name, 3, IDX_FUNC);
        if (flabelId < 0) { printf("internal error: could not update function labels\n"); return -1; }

        if (b->labels[IDX_FUNC][flabelId].offset != 0)
        {
            printf("error: label %s already defined elsewhere\n", b->labels[IDX_FUNC][flabelId].name);
            return - 1;
        }

        b->labels[IDX_FUNC][flabelId].offset = origp - b->hcbbuf;
        b->numlabels[IDX_JMP] = 0;
    }
    else if (*origp == 0x02 || *origp == 0x06 || *origp == 0x07)
    {
        //add label if it doesn't exist
        int labelId = manageLabel(b, txp, 3, (*origp == 0x02) ? IDX_FUNC : IDX_JMP);
        if (labelId < 0) { printf("error: could not add label\n"); return -1; }

        //store label id within op param
        *(int*)(b->hcbp) = labelId;
        b->hcbp += 4;
        done = 1;
    }
    else if (*origp == 0x03)
    {
        int labelId = manageLabel(b, txp, 3, IDX_IMPORT);
        if (labelId < 0) { printf("error: could not add label\n"); return -1; }

        *(short*)(b->hcbp) = (short)labelId;
        b->hcbp += 2;
        done = 1;
    }
    else if (*origp == 0x08)
    {
        //handle rep(x) in 0x08
        if (memcmp(txp, "rep(", 4) == 0)
        {
            int ret;

            done = 1;
            txp += 4;

            while (txp[done] != ')') { done++; }
            txp[done] = 0;

            ret = strtol(txp, NULL, 10) - 1;
            for (done = 0; done < ret; done++) { *b->hcbp++ = 0x08; }
        }

        done = 1;
    }

    //everything else
    if (done == 0)
    {
        int val, c;

        val = strtol((char*)txp, NULL, 10);

        if (*origp == 0x0A)
        {
            //pushint: check if function label is being requested
            if (memcmp(txp, "LABEL:", 6) == 0)
            {
                int labelId = manageLabel(b, txp + 6, 3, IDX_FUNC);
                if (labelId < 0) { printf("error: could not add label\n"); return -1; }

                b->labels[IDX_PTR][b->numlabels[IDX_PTR]].name = txp + 6;
                b->labels[IDX_PTR][b->numlabels[IDX_PTR]].offset = b->hcbp - b->hcbbuf;
                b->numlabels[IDX_PTR]++;

                //store label id within op param
                *(int*)(b->hcbp) = labelId;
                b->hcbp += 4;

                goto done;
            }

            //pushint: see if a smaller integer type can be used
            if (val >= CHAR_MIN && val <= CHAR_MAX) { *origp = 0x0C; }
            else if (val >= SHRT_MIN && val <= SHRT_MAX) { *origp = 0x0B; }
        }

        switch (opdef[*origp].params)
        {
        case OPARG_STRING:
            *b->hcbp++ = (char)(linelen + 1);
            memcpy((char*)b->hcbp, (char*)txp, linelen);
            b->hcbp += linelen;
            *b->hcbp++ = 0;
            break;

        case OPARG_I8I8:
            c = 0;
            while (txp[c] != ' ') { c++; }
            txp[c] = 0;

            val = strtol((char*)txp, NULL, 10);
            *(char*)b->hcbp++ = (char)val;

            txp += c + 1;
            txp += bytesWhitespace(txp, txend);
            val = strtol((char*)txp, NULL, 10);
            //yes, the lack of a "break" is intentional here

        case OPARG_I8: *(char*)b->hcbp = (char)val; b->hcbp++; break;
        case OPARG_I16: *(short*)b->hcbp = (short)val; b->hcbp += 2; break;
        case OPARG_I32: *(int*)b->hcbp = (int)val; b->hcbp += 4; break;

        case OPARG_X32:
            *(unsigned int*)b->hcbp = (unsigned int)strtoul((char*)txp, NULL, 16);
            b->hcbp += 4;
            break;

        case OPARG_NULL: break;

        default:
            printf("internal error: unknown param type\n");
            return 0;
        }
    }

done:
    return b->hcbp - origp;
}

int HcbRebuilder::parseLine(BUILD *b, char *txp, char *txend)
{
    int linelen, nextline, opcode, c;

    nextline = bytesUntilNextLine(txp, txend);

    //skip any whitespace
    txp += bytesWhitespace(txp, txp + nextline);
    linelen = bytesUntilCRLF(txp, txp + nextline);

    //detect comment
    for (c = 0; c < linelen; c++)
    {
        if (txp[c] == '#')
        {
            linelen = c;

            c--;
            while (txp[c] == ' ' || txp[c] == '\t') { txp[c--] = 0; }
            break;
        }
    }

    if (linelen > 0)
    {
        //check if line is label or command
        if (txp[linelen - 1] == ':' && memcmp(txp, "pushstring ", 11) != 0)
        {
            //set up a LABEL
            int labelId;

            txp[linelen - 1] = 0;

            //try to add or update jmp label
            labelId = manageLabel(b, txp, 3, IDX_JMP);
            if (labelId < 0) { printf("error: could not add label\n"); return -1; }

            b->labels[IDX_JMP][labelId].offset = b->hcbp - b->hcbbuf;
        }
        else
        {
            //set up an OPCODE
            char *txend = txp + linelen;

            //get main opcode
            opcode = getOpcodeId(txp, txend);
            if (opcode < 0) { return -1; }

            *b->hcbp = (char)opcode;

            txp += strlen(opdef[opcode].name);
            if (opdef[opcode].params == OPARG_STRING) { txp++; }
            else { txp += bytesWhitespace(txp, txend); }

            //check if a new function is starting
            if (opcode == 0x01)
            {
                b->numFunc++;

                //resolve jmp offsets for the previous function
                if (b->numFunc > 1) { resolveJumpOffsets(b, 0, b->curFuncp, b->hcbp); }

                b->curFuncp = b->hcbp;
            }

            //get op parameter
            opcode = genOpcodeData(b, txp, txend);
            if (opcode < 1) { return -2; }
        }
    }

    return nextline;
}

int HcbRebuilder::scanPropertyLine(const char *propname, char *txp, char *txend, int *nextline)
{
    char *c = txp;
    int linelen;

    if (memcmp(propname, txp, strlen(propname)) != 0)
    {
        printf("error: could not find %s propery\n", propname);
        return -1;
    }

    linelen = bytesUntilCRLF(txp, txend);
    *nextline = bytesUntilNextLine(txp, txend);
    txp[linelen] = 0;

    while (*c != '=') { c++; }
    c++;
    c += bytesWhitespace(c, txp + linelen);

    return c - txp;
}

int HcbRebuilder::readFinalItems(BUILD *b, char *txp, char *txend)
{
    unsigned int i, total;
    int nextline, ret, *n;
    char *c;

    //entrypoint offset
    ret = scanPropertyLine("ENTRYPOINT", txp, txend, &nextline);
    if (ret < 1) { return -1; }

    c = txp + ret;
    ret = manageLabel(b, c, 0, IDX_FUNC);
    if (ret < 0) { printf("error: could not find label %s\n", txp); return -1; }

    n = (int*)(b->hcbp);
    *n = b->labels[IDX_FUNC][ret].offset;
    b->hcbp += 4;

    txp += nextline;

    //those 6 bytes
    ret = scanPropertyLine("BIN", txp, txend, &nextline);
    c = txp + ret;
    for (i = 0; i < 6; i++)
    {
        unsigned char v;

        c[2] = 0;
        v = (unsigned char)strtoul(c, NULL, 16);
        *b->hcbp++ = v;
        c += 3;
    }

    txp += nextline;

    //title string
    ret = scanPropertyLine("TITLE", txp, txend, &nextline);
    c = txp + ret;

    ret = strlen(c) + 1;
    *b->hcbp++ = (unsigned char)ret;
    memcpy(b->hcbp, c, ret);
    b->hcbp += ret;

    txp += nextline;

    //imports
    ret = scanPropertyLine("NUM_IMPORTS", txp, txend, &nextline);
    c = txp + ret;

    total = strtoul(c, NULL, 10);
    txp += nextline;

    *(unsigned short*)b->hcbp = (unsigned short)total;
    b->hcbp += 2;

    b->importp = b->hcbp;

    for (i = 0; i < total; i++)
    {
        int endBracket;
        nextline = bytesUntilNextLine(txp, txend);

        c = txp;
        while (*c != '|') { c++; }
        c += 2;

        ret = 0;
        while (c[ret] != ' ') { ret++; }

        //arg count
        c[ret] = 0;

        endBracket = ret;
        while (c[endBracket] != ']') { endBracket++; }
        c[endBracket] = 0;

        *b->hcbp++ = (unsigned char)strtoul(c + ret + 2, NULL, 10);

        //name
        ret = strlen(c) + 1;
        *b->hcbp++ = (unsigned char)ret;
        memcpy(b->hcbp, c, ret);
        b->hcbp += ret;

        txp += nextline;
    }

    //two more zeros for whatever reason
    *b->hcbp++ = 0;
    *b->hcbp++ = 0;

    return 0;
}

int HcbRebuilder::remapImportList(BUILD *b)
{
    unsigned int i, j;
    unsigned int impcount = *(unsigned short*)(b->importp - 2);

    for (i = 0; i < b->numlabels[IDX_IMPORT]; i++)
    {
        unsigned char *p = b->importp;

        for (j = 0; j < impcount; j++)
        {
            if (strcmp(b->labels[IDX_IMPORT][i].name, (char*)p + 2) == 0)
            {
                b->labels[IDX_IMPORT][i + 256].offset = j;
                break;
            }

            p += p[1] + 2;
        }

        if (j >= impcount)
        {
            printf("error: syscall %s not found in import list!\n", b->labels[IDX_IMPORT][i].name);
            return -1;
        }
    }

    return 0;
}

int HcbRebuilder::rebuild(const char *txname, const char *hcbname)
{
    FILE *fp;
    char *txbuf, *txp, *txend;
    unsigned char *hcbbuf;
    int fsize_tx, ret, curLine;
    BUILD build;

    //open and read text script
    fp = fopen(txname, "rb");
    if (fp == NULL)
    {
        printf("error opening %s\n", txname);
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    fsize_tx = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    txbuf = (char*)malloc(fsize_tx + 2);

    fread(txbuf, 1, fsize_tx, fp);
    fclose(fp);

    txp = txbuf;
    txend = txbuf + fsize_tx;
    *txend = 0;

    //allocate buffers
    hcbbuf = (unsigned char*)malloc(16 * 1024 * 1024);
    build.hcbbuf = hcbbuf;
    build.hcbp = hcbbuf + 4;
    build.curFuncp = build.hcbp;

    build.labels[IDX_JMP] = (JMPOFFSET*)malloc(sizeof(JMPOFFSET) * MAX_IDXCOUNT);
    build.labels[IDX_FUNC] = (JMPOFFSET*)malloc(sizeof(JMPOFFSET) * MAX_IDXCOUNT);
    build.labels[IDX_IMPORT] = (JMPOFFSET*)malloc(sizeof(JMPOFFSET) * 512);
    build.labels[IDX_PTR] = (JMPOFFSET*)malloc(sizeof(JMPOFFSET) * MAX_IDXCOUNT);
    build.numlabels[IDX_JMP] = 0;
    build.numlabels[IDX_FUNC] = 0;
    build.numlabels[IDX_IMPORT] = 0;
    build.numlabels[IDX_PTR] = 0;

    build.hashlabels = (unsigned int*)malloc(sizeof(int) * HASH_MODULO * HASH_BUCKETSIZE);
    build.hashidnum = (unsigned int*)malloc(sizeof(int) * HASH_MODULO * HASH_BUCKETSIZE);
    build.hashcounts = (unsigned short*)malloc(sizeof(*build.hashcounts) * HASH_MODULO);
    memset(build.hashcounts, 0, sizeof(*build.hashcounts) * HASH_MODULO);

    build.numFunc = 0;
    curLine = 1;

    //start parsing code
    while (txp < txend)
    {
        //stop when the property section is reached
        if (memcmp(txp, "ENTRYPOINT", 10) == 0) { break; }

        //main section
        ret = parseLine(&build, txp, txend);
        if (ret < 1)
        {
            printf("quitting due to error / line %d\n", curLine);
            goto leave;
        }

        txp += ret;
        curLine++;
    }

    //write length of code section at the beginning of the file
    *(int*)(build.hcbbuf) = build.hcbp - build.hcbbuf;

    //resolve jmp offsets for the last function
    resolveJumpOffsets(&build, 0, build.curFuncp, build.hcbp);
    build.curFuncp = build.hcbp;

    //parse property values and import table
    ret = readFinalItems(&build, txp, txend);
    if (ret != 0) { goto leave; }

    //resolve offsets for call/syscall
    ret = remapImportList(&build);
    if (ret != 0) { goto leave; }

    resolveJumpOffsets(&build, 1, build.hcbbuf + 4, build.curFuncp);

    //ready write hcb file
#if 1
    fp = fopen(hcbname, "wb");
    if (fp != NULL)
    {
        fwrite(hcbbuf, 1, build.hcbp - hcbbuf, fp);
        fclose(fp);
    }
    else
    {
        printf("error creating %s\n", hcbname);
    }
#endif
leave:
    free(build.labels[IDX_JMP]);
    free(build.labels[IDX_FUNC]);
    free(build.labels[IDX_IMPORT]);
    free(build.labels[IDX_PTR]);

    free(build.hashlabels);
    free(build.hashidnum);
    free(build.hashcounts);

    free(txbuf);
    free(hcbbuf);

    return 0;
}
