#include "fvp.h"

int main(int argc, char *argv[])
{
    if (argc == 3 && strcmp(argv[1], "-split") == 0)
    {
        Parser::getInstance().exportParts(argv[2]);
    }
    else if (argc == 4 && strcmp(argv[1], "-d") == 0)
    {
        HcbDecoder decoder;
        decoder.decode(argv[2], argv[3]);
        Parser::getInstance().dumpStrings(argv[3]);
    }
    else if ((argc == 5 || argc == 7) && strcmp(argv[1], "-c") == 0)
    {
        QByteArray new_script;

        if(argc==5)
            new_script = Parser::getInstance().insertStrings( argv[argc - 3], argv[argc - 2]);
        else if(argc == 7 && strcmp(argv[2], "-ww") == 0)
            new_script = Parser::getInstance().insertStrings(argv[argc - 3], argv[argc - 2], atoi(argv[3]));
        else
            goto END;

        QTemporaryFile script;
        script.open();
        script.write(new_script.data());
        script.close();

        HcbRebuilder rebuilder;
        rebuilder.rebuild(script.fileName().toStdString().c_str(), argv[argc - 1]);

    }
    else if( (argc == 3 || argc == 4) && (strcmp(argv[1], "-encode") == 0 || strcmp(argv[1], "-decode") == 0) )
    {
        if(argc == 3)
            NvsgConverter::getInstance().autoProcess(argv[2], argv[1]);
        else if(argc == 4)
            NvsgConverter::getInstance().autoProcess(argv[2], argv[1], argv[3]);
        else
            goto END;
    }

    else
END:    usage();

    return 0;
}

void usage()
{
    printf("\nusage:\n"
           "   tool -d <input.hcb> <output.txt>\n"
           "   tool -split <input_strings.txt>\n"
           "   tool -c [-ww limit] <input_strings.txt> <input_script.txt> <output.hcb>\n"
           "   tool -decode <input (NVSG File or Folder)> [output]\n"
           "   tool -encode <input (PNG File or Folder)> [output]\n"
           );
}
