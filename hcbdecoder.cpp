#include "hcbdecoder.h"

const char *HcbDecoder::funcAlias(int funcId, int funcCount, unsigned char *buf, unsigned char *p)
{
    int i;

#if 1
    if (funcCount == 3382)
    {
        //IroSekai stuff
        if (funcId >= 26 && funcId <= 971)
        {
            const char *retstr = getBgName(buf, p);
            if (retstr != NULL)
            {
                sprintf(g_temp, "SHOWIMG_%s", retstr);
                return g_temp;
            }
        }

        switch (funcId)
        {
            case 2756: return "SET_TEXTMODE"; break;
            case 2785: return "SHOWNAME"; break;
            case 2787: return "TEXTWRITE"; break;
            case 2801: return "POLL_INPUT\n"; break;
            case 2860: return "CAMERA_MOVE"; break;
            case 2883: return "PLAY_SFX"; break;

            case 3355: return "SCENE_PROLOGUE"; break;

            default: break;
        }
    }
#endif

    if (funcCount == 6160)
    {
        //AstralAir
        switch (funcId)
        {
            case 6137: return "SCENE_PROLOGUE"; break;
        }
    }

    if (funcCount == 5546)
    {
        //Hoshimemo FD
        switch (funcId)
        {
            case 5525: return "SCENE_PROLOGUE"; break;
        }
    }

    //auto-detected SPEAK_ names
    for (i = 0; i < speak_count; i++)
    {
        if (speakfuncs[i] == funcId)
        {
            sprintf(g_temp, "SPEAK_%d_", funcId);
            return g_temp;
        }
    }

    return NULL;
}

void HcbDecoder::findSpeakFuncs(unsigned char *hcbbuf, unsigned int *func_index, unsigned int func_count)
{
    unsigned int i;

    if (func_count > MAX_SPEAKFUNC) { func_count = MAX_SPEAKFUNC; }

    speak_count = 0;

    for (i = 0; i < func_count; i++)
    {
        int ret = isSpeakFunction(hcbbuf, func_index[i]);
        if (ret == 1) { speakfuncs[speak_count++] = i; }
    }
}

int HcbDecoder::decode_op(const unsigned char *p)
{
    unsigned int opcode;

    opcode = *p;
    if (*p > HCB_LAST_OPCODE) { printf("\nunknown opcode %02X!\n", opcode); return -1; }

    //decode
    fprintf(outfile, "%s", opdef[opcode].name);
    p++; //move past opcode

    switch (opdef[opcode].params)
    {
        case OPARG_NULL: return 1;
        case OPARG_X32: fprintf(outfile, " 0x%X", *(unsigned int*)p); return 5;
        case OPARG_I32: fprintf(outfile, " %d", *(int*)p); return 5;
        case OPARG_I16: fprintf(outfile, " %d", *(short*)p); return 3;
        case OPARG_I8I8: fprintf(outfile, " %d %d", *(char*)p, *(char*)(p + 1)); return 3;
        case OPARG_I8: fprintf(outfile, " %d", *(char*)p); return 2;
        case OPARG_STRING: fprintf(outfile, " %s", (char*)(p + 1)); return *p + 2;

        default: printf("internal error: unknown param type\n"); break;
    }

    return -2; //error
}

int HcbDecoder::indexFunctions(const unsigned char *buf, const unsigned char *p, const unsigned char *endcodep, unsigned int *idx, unsigned int *counted, int mode, const char *codemap)
{
    /*
        mode 0: scan for function start addresses
        mode 1: scan for jump offsets within a function
    */

    unsigned int indexcount = 0;

    *counted = 0;

    while (p < endcodep)
    {
        unsigned int oplen;

        while (codemap[p - buf] != 1) { p++; }

        if (*p > HCB_LAST_OPCODE) { printf("\nunknown opcode %02X (offset=%X)\n", *p, p - buf + 4); return -1; }
        if (indexcount >= MAX_IDXCOUNT) { printf("internal error: exceeded indexing capacity (mode %d)\n", mode); return -1; }

        if (mode == 0 && *p == 0x01)
        {
            idx[indexcount++] = p - buf;
        }
        else if (mode == 1 && (*p == 0x06 || *p == 0x07))
        {
            unsigned int c, jmpoffset;

            jmpoffset = *(unsigned int*)(p + 1);

            //check if offset was already in the index.
            for (c = 0; c < indexcount; c++)
            {
                if (idx[c] == jmpoffset) { break; }
            }

            //add the offset to the index if it is new
            if (c >= indexcount) { idx[indexcount++] = jmpoffset; }
        }

        //MemoriaEN -- stop at string section
        if (mode == 1 && (p[0]==0x04 && p[1]==0x04 && p[2]==0x0E)) { endcodep = p + 2; }

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

    idx[indexcount] = endcodep - buf; //only useful in mode 0
    *counted = indexcount;

    return 0;
}

int HcbDecoder::getFuncIdFromOffset(unsigned int offset, unsigned int *func_index, int numfunc, int exact)
{
    int i;

    if (exact == 1)
    {
        //search for exact offset of a function
        for (i = 0; i < numfunc; i++)
        {
            if (func_index[i] == offset) { return i; }
        }
    }
    else
    {
        //search for function which contains the offset
        for (i = 0; i < (numfunc + 1); i++)
        {
            if (func_index[i] == offset) { return i; }
            else if (func_index[i] > offset) { return i - 1; }
        }
    }

    return -1;
}

void HcbDecoder::getSysImports(unsigned char *buf, char ***sys_imports)
{
    unsigned int num, i;
    unsigned char *p;

    p = buf + *(int*)buf;
    p += 4 + 6;
    p += 1 + *p;

    num = *(unsigned short*)p;
    p += 2;

    *sys_imports = (char**)malloc(num * sizeof(char*));

    for (i = 0; i < num; i++)
    {
        (*sys_imports)[i] = (char*)(p + 2);
        p += 2 + p[1];
    }
}


#define JMP_LABELSPEC "_F%u_x%X_"

int HcbDecoder::decode(const char *hcbname, const char *outname)
{
    FILE *infile;
    unsigned char *buf, *p, *endcodep;
    unsigned int entryoffset;
    int fsize, curfunc;
    char **sys_imports;

    unsigned int *func_index, func_count;
    unsigned int *label_index, label_count;

    unsigned char *ophistory[OPHIST];
    unsigned int histcount;

    char *codemap;

    infile = fopen(hcbname, "rb");
    if (infile == NULL)
    {
        printf("error opening %s\n", hcbname);
        return 1;
    }

    outfile = fopen(outname, "wb");
    if (outfile == NULL)
    {
        printf("error opening %s\n", outname);
        return 1;
    }

    fseek(infile, 0, SEEK_END);
    fsize = ftell(infile);
    fseek(infile, 0, SEEK_SET);

    buf = (unsigned char*)malloc(fsize + 4);
    fread(buf, 1, fsize, infile);

    p = buf + 4;
    endcodep = buf + *(int*)buf;

    entryoffset = *(unsigned int*)endcodep;

    //flow analysis
    codemap = (char*)malloc(fsize);
    //analyzer_start(buf, codemap);
    memset(codemap, 1, fsize);

    //makes some indexes
    getSysImports(buf, &sys_imports);

    func_index = (unsigned int*)malloc(sizeof(*func_index) * MAX_IDXCOUNT);
    fsize = indexFunctions(buf, p, endcodep, func_index, &func_count, 0, codemap);
    if (fsize != 0) { return -1; }

    label_index = (unsigned int*)malloc(sizeof(*label_index) * MAX_IDXCOUNT);
    label_count = 0;

    findSpeakFuncs(buf, func_index, func_count);

    //parse script code
    curfunc = -1;
    histcount = 0;


    //VM TESTING -------
    //p = buf + 0x0C6128;
    //curfunc = 9999;

#if 0
    getSpeakerName(buf, p);
    goto leave;
#endif
    //------

    while (p < endcodep)
    {
        unsigned int curoffset;
        int oplen;

        if (p[0]==0x04 && p[1]==0x04 && p[2]==0x0E)
        {
            //MemoriaEN -- stop at string section
            endcodep = p + 2;
        }

        while (codemap[p - buf] != 1)
        {
            //printf("# unk%02X\n", *p);
            p++;
        }

        curoffset = p - buf;

        ophistory[histcount++] = p;
        histcount %= OPHIST;

#if 1
        //skip nop
        if (*p == 0x00) { p++; continue; }
#endif

#if 1
        //see if a new function is starting
        if (*p == 0x01)
        {
            const char *alias;

            curfunc++;

            if (curoffset != func_index[curfunc]) { printf("#WARN: inconsistent index!\n"); break; }
            if (curoffset == entryoffset) { fprintf(outfile, "\n\n#ENTRYPOINT"); }

            //index any jump offsets
            oplen = indexFunctions(buf, p, buf + func_index[curfunc + 1], label_index, &label_count, 1, codemap);
            if (oplen != 0) { break; }

            //printf("\n# OFFSET = 0x%X", p - buf);
            fprintf(outfile, "\n# =================================================\n\n");

            alias = funcAlias(curfunc, func_count, buf, p);
            if (alias != NULL)
            {
                char temp[64];
                int c;

                strcpy(temp, alias);
                c = strlen(temp) - 1;
                while (c > 0 && temp[c] == '\n') { temp[c--] = 0; }
                fprintf(outfile, "%s:\n", temp);
            }
            else { fprintf(outfile, "function_%u_:\n", curfunc); }
        }
        else
        {
            //make a label if current offset is a jump destination
            unsigned int c;

            for (c = 0; c < label_count; c++)
            {
                if (curoffset == label_index[c])
                {
                    fprintf(outfile, JMP_LABELSPEC ":\n", curfunc, curoffset - func_index[curfunc]);
                    break;
                }
            }
        }
#endif
        //printf("%06X  ", curoffset);
        fprintf(outfile, "\t");

        //handle opcodes
        oplen = -1;
#if 1
        if (*p == 0x02)
        {
            //function call
            int funcId;
            const char *alias;

            funcId = getFuncIdFromOffset(*(unsigned int*)(p + 1), func_index, func_count, 1);
            if (funcId < 0) { printf("internal error: index search failure\n"); break; }

            alias = funcAlias(funcId, func_count, buf, buf + *(unsigned int*)(p + 1));
            if (alias != NULL)
            {
                fprintf(outfile, "call %s", alias);
#if 1
                if (memcmp(alias, "SPEAK_", 6) == 0 && curfunc > 3000)
                {
                    const char *spkr;
                    int argIdx;

                    argIdx = *(int*)(p + 1);
                    argIdx = buf[argIdx + 1];
                    argIdx = histcount + (OPHIST - 1) - argIdx;

                    spkr = getSpeakerName(buf, ophistory[argIdx % OPHIST]);
                    if (spkr != NULL) fprintf(outfile, " #%s", spkr);
                    else { printf(" # wtf noname\n"); }
                }
#endif
            }
            else { fprintf(outfile, "call function_%u_", funcId); }

            oplen = 5;
        }
        else if (*p == 0x03)
        {
            //named syscall
            fprintf(outfile, "%s %s", opdef[*p].name, sys_imports[p[1]]);
            oplen = 3;
        }
        else if (*p == 0x06 || *p == 0x07)
        {
            //jump
            int funcId;
            unsigned int jmpoffset;

            jmpoffset = *(unsigned int*)(p + 1);
            funcId = getFuncIdFromOffset(jmpoffset, func_index, func_count, 0);

            if (funcId < 0) { printf("internal error: index search failure\n"); break; }

            if (buf[jmpoffset] == 0x0E && buf[jmpoffset + 2 + buf[jmpoffset + 1]] == 0x06)
            {
                //MemoriaEN -- substitute string trampoline with actual string op
                decode_op(buf + jmpoffset);
            }
            else
            {
                fprintf(outfile, "%s " JMP_LABELSPEC, opdef[*p].name, funcId, jmpoffset - func_index[funcId]);

                if ((funcId != curfunc) || (jmpoffset >= func_index[funcId + 1]))
                {
                    printf("\t#WARN: jumped out of function scope?\n");
                }
            }

            oplen = 5;
        }
        else if (*p == 0x0A /*|| *p == 0x0B || *p == 0x0C*/)
        {
            unsigned int len, val;

            switch (*p)
            {
                case 0x0A: len = 4 + 1; val = *(int*)(p + 1); break;
                case 0x0B: len = 2 + 1; val = *(short*)(p + 1); break;
                case 0x0C: len = 1 + 1; val = *(char*)(p + 1); break;
            }

            fprintf(outfile, "pushint ");

            if (p[len] == 0x03)
            {
                const char *alias, *impname;

                impname = sys_imports[*(short*)(p + len + 1)];
                if (strcmp(impname, "ThreadStart") == 0)
                {
                    int funcId = getFuncIdFromOffset(val, func_index, func_count, 1);
                    if (funcId < 0) { printf("error: ThreadStart address %X not seen as a function\n", val); /*return -1;*/ }

                    alias = funcAlias(funcId, func_count, buf, p);
                    if (alias != NULL) { fprintf(outfile, "LABEL:%s", alias); }
                    else { fprintf(outfile, "LABEL:function_%d_", funcId); }
                }
                else { goto writenum; }
            }
            else
            {
writenum:
                fprintf(outfile, "%d", val);
            }

            oplen = len;
        }
#endif
        else
        {
            //this. most of the time.
            oplen = decode_op(p);
        }

        //quick hack for vm
        if (*p == 0x19) { histcount = (histcount + (OPHIST - 1)) % OPHIST; }

        //annotations for some opcodes
#if 1
        if ((*p == 0x08 || *p == 0x00) && oplen == 1)
        {
            //catch repeated op08
            while (p[oplen] == *p)
            {
                ophistory[histcount++] = p + oplen;
                histcount %= OPHIST;

                oplen++;
            }
            if (oplen > 1) { fprintf(outfile, " rep(%d)", oplen); }
        }
#endif
        fprintf(outfile, "\n");

        if (oplen > 0) { p += oplen; }
        else { printf("quitting due to error\n"); curfunc = -1; break; }
    }

    p = buf + *(int*)buf; //just in case

    if (curfunc > 0)
    {
        //after parsing, output a few more bits
        curfunc = getFuncIdFromOffset(*(unsigned int*)p, func_index, func_count, 1);
        fprintf(outfile, "\nENTRYPOINT = function_%d_\n", curfunc);
        p += 4;

        fprintf(outfile, "BIN =");
        for (fsize = 0; fsize < 6; fsize++) { fprintf(outfile, " %02X", *p++); }

        fprintf(outfile, "\nTITLE = %s\n", p + 1);
        p += 1 + *p;

        curfunc = *(unsigned short*)p;
        fprintf(outfile, "NUM_IMPORTS = %u\n", curfunc);
        p += 2;

        //engine imports
        for (fsize = 0; fsize < curfunc; fsize++)
        {
            fprintf(outfile, "%d | %s [%d]\n", fsize, p + 2, *p);
            p += 2 + p[1];
        }
    }
//leave:
    free(sys_imports);
    free(func_index);
    free(label_index);
    free(codemap);

    fclose(outfile);

    return 0;
}

void HcbDecoder::nothing (const char *x, ...) { return; }

void HcbDecoder::quickscan(VM_CONTEXT *c)
{
    int i;
    unsigned char *origp = c->p;

    //skip to "pushstack" op
    if (c->qscan_state > 0)
    {
        for (i = 0; i < 60; i++)
        {
            if (*c->p == 0x10 || *c->p == 0x04) { break; }

            switch (opdef[*c->p].params)
            {
                case OPARG_NULL: c->p++; break;
                case OPARG_I8: c->p += 2; break;
                case OPARG_I8I8:
                case OPARG_I16: c->p += 3; break;
                case OPARG_I32:
                case OPARG_X32: c->p += 5; break;
                default: break;
            }
        }

        if (i >= 60 || *c->p == 0x04) { c->p = origp; return; }

        c->qscan_state--;
    }
}

void HcbDecoder::vm_execute(VM_CONTEXT *c)
{
    int n, run;

    run = 350;

    while (run > 0)
    {
        run--;

        if (c->sp >= VMSTACK_SIZE)
        {
            fprintf(stderr, "VM error: ran out of stack\n");
            c->status = -1;
            return;
        }
        /*if (c->p < c->hcbbuf)
        {
            fprintf(stderr, "VM error: inst ptr not set\n");
            c->status = -1;
            return;
        }*/
        if (*c->p > HCB_LAST_OPCODE)
        {
            fprintf(stderr, "VM error: opcode %02X is invalid\n", *c->p);
            c->status = -1;
            return;
        }

        //debug_printf("vm: %6X | sp=%2d | %02X  %s\n", c->p - c->hcbbuf, c->sp, *c->p, opdef[*c->p].name);

        switch (*c->p)
        {
        case 0x00: //nop
            c->p++;
            break;

        case 0x01: //initstack
            c->p += 3;
            quickscan(c); //hack
            break;

        case 0x02: //call
            if (c->hcbbuf[*(int*)(c->p + 1) + 2] != 0 || c->framenum < 0)
            {
                //restore stack and ignore call
                //debug_printf("VM: call: ignoring...\n");
                c->sp -= c->hcbbuf[*(int*)(c->p + 1) + 2];
                c->p += 5;
            }
            else
            {
                //set up call
                c->frame[c->framenum].orig_sp = c->sp;
                c->frame[c->framenum].retp = c->p + 5;

                c->stack[c->sp].type = T_RET;
                c->stack[c->sp].data = c->framenum;
                c->sp++;
                c->framenum++;

                c->p = c->hcbbuf + *(int*)(c->p + 1);
            }
            break;

        case 0x04: //ret
            if (c->sp < 1)
            {
                printf("VM error: cannot return / sp = 0\n");
                c->status = -1;
                run = 0;
            }

            if (c->stack[c->sp - 1].type != T_RET)
            {
                printf("VM error: cannot return / bad sp\n");
                c->status = -1;
                run = 0;
            }

            n = c->stack[c->sp - 1].data; //get frame number to restore

            c->sp = c->frame[n].orig_sp;
            c->p = c->frame[n].retp;
            c->framenum--;
            break;

        case 0x06: //jmp
            c->p = c->hcbbuf + *(int*)(c->p + 1);
            break;

        case 0x07: //jmpcond
            if (c->stack[c->sp - 1].type == T_FALSE) //wtf really?
            {
                //debug_printf("VM: jmpcond... yes\n");
                c->p = c->hcbbuf + *(int*)(c->p + 1);
            }
            else { c->p += 5; }

            c->sp--;
            break;

        case 0x08: //pushtrue
            c->stack[c->sp].type = T_TRUE;
            c->stack[c->sp].data = 0;
            c->sp++;
            c->p++;
            break;

        case 0x09: //pushfalse
            c->stack[c->sp].type = T_FALSE;
            c->stack[c->sp].data = 0;
            c->sp++;
            c->p++;
            break;

        case 0x0A: //pushint32
            c->stack[c->sp].type = T_INTEGER;
            c->stack[c->sp].data = *(int*)(c->p + 1);
            c->sp++;
            c->p += 5;
            break;

        case 0x0B: //pushint16
            c->stack[c->sp].type = T_INTEGER;
            c->stack[c->sp].data = *(short*)(c->p + 1);
            c->sp++;
            c->p += 3;
            break;

        case 0x0C: //pushint8
            c->stack[c->sp].type = T_INTEGER;
            c->stack[c->sp].data = *(char*)(c->p + 1);
            c->sp++;
            c->p += 2;
            break;

        case 0x0E: //pushstring
            c->stack[c->sp].type = T_STRING;
            c->stack[c->sp].data = (c->p - c->hcbbuf) + 2;
            c->sp++;

            c->status++;
            if (c->status >= 1) { run = 0; }

            c->p += 2 + c->p[1];
            break;

        case 0x0F: //pushglobal
            //stub
            c->stack[c->sp].type = T_INTEGER;
            c->stack[c->sp].data = 1337; //totally fake.
            c->sp++;

            c->p += 3;
            break;

        case 0x10: //pushstack
            n = c->sp + (char)c->p[1];

            if (n < 0)
            {
                //debug_printf("VM: pushstack: negative index!\n");
                c->status = -1;
                run = 0;
                break;
            }

            c->stack[c->sp].type = c->stack[n].type;
            c->stack[c->sp].data = c->stack[n].data;
            c->sp++;
            c->p += 2;
            break;

        case 0x15: //popglobal
            //stub
            c->sp--;
            c->p += 3;
            break;

        case 0x19: //neg
            c->stack[c->sp - 1].data *= -1;
            c->p++;
            break;
/*
        case 0x1A: //add
            c->sp--;

            if (c->stack[c->sp].type != T_INTEGER || c->stack[c->sp + 1].type != T_INTEGER)
            {
                fprintf(stderr, "VM: add: non-integer operands\n");
                run = 0;
                break;
            }

            c->stack[c->sp].data += c->stack[c->sp + 1].data;
            c->p++;
            break;

        case 0x021: //logor
            c->sp--;

            if (c->stack[c->sp].type > T_FALSE || c->stack[c->sp + 1].type > T_FALSE)
            {
                fprintf(stderr, "VM: logor: non-bool operands\n");
                run = 0;
                break;
            }

            n = (c->stack[c->sp].type ^ 1) || (c->stack[c->sp + 1].type ^ 1);
            n ^= 1;
            c->stack[c->sp].type = (n == 0) ? T_TRUE : T_FALSE;

            c->p++;
            break;
*/
        case 0x22: //eq
            c->sp--;

            if (((c->stack[c->sp - 1].type == T_INTEGER && c->stack[c->sp].type == T_INTEGER) ||
                 (c->stack[c->sp - 1].type <= T_FALSE && c->stack[c->sp].type <= T_FALSE)) == 0)
            {
                //mismatched types
                //debug_printf("VM: eq: mismatched operand types.\n");
                c->stack[c->sp - 1].type = T_FALSE;
            }
            else
            {
                n = (c->stack[c->sp - 1].data == c->stack[c->sp].data);
                c->stack[c->sp - 1].type = (n == 1) ? T_TRUE : T_FALSE;
            }

            c->stack[c->sp - 1].data = 0;

            c->p++;
            break;

        default:
            printf("VM error: unimplemented opcode %02X (pos=%X)\n", *c->p, *c->p  - *c->hcbbuf);
            c->status = -1;
            run = 0;
            break;
        }
    }
}

void HcbDecoder::vm_init(VM_CONTEXT *context, unsigned char *hcbbuf, unsigned char *p)
{
    context->hcbbuf = hcbbuf;
    context->p = p;

    context->sp = 0;
    context->framenum = 0;

    context->status = 0;
    context->qscan_state = 2;
}

const char *HcbDecoder::getSpeakerName(unsigned char *hcbbuf, unsigned char *p)
{
    VM_CONTEXT context;

    vm_init(&context, hcbbuf, p);
    vm_execute(&context);

    if (context.sp < 1 || context.stack[context.sp - 1].data < 0 || context.status < 0) { return NULL; }

    return (char*)hcbbuf + context.stack[context.sp - 1].data;
}

const char *HcbDecoder::getBgName(unsigned char *hcbbuf, unsigned char *p)
{
    VM_CONTEXT context;

    vm_init(&context, hcbbuf, p);

    context.qscan_state = 0; //don't use quickscan
    context.framenum = -1; //don't allow call
    context.sp = 0;

    vm_execute(&context);

    if (context.stack[context.sp - 1].data < 0 || context.status < 0 || context.sp < 1) { return NULL; }

    return (char*)hcbbuf + context.stack[context.sp - 1].data;
}

int HcbDecoder::isSpeakFunction(unsigned char *hcbbuf, unsigned int offset)
{
    VM_CONTEXT context;
    int status;

    unsigned int *calloffset;
    unsigned char bytecode[] = { 8,8,8,8,8, 2, 0,0,0,0 };
    unsigned char *dest = hcbbuf + offset;

    // protagonist=5, everything else=3
    if (((dest[1] == 3 || dest[1] == 5) && dest[2] == 0) != 1) { return 0; }

    calloffset = (unsigned int*)(bytecode + 6);
    *calloffset = offset;

    vm_init(&context, hcbbuf, bytecode);
    vm_execute(&context);

    status = ((context.stack[context.sp - 1].data < 0 || context.status < 0 || context.sp < 1) == 0);
    return status;
}

