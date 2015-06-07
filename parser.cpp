#include "parser.h"


QStringList *Parser::getScript()
 {
    return script;
}

void Parser::loadScript(const QString filename)
{
    if(script != NULL)
        delete script;

    script = readShiftJis(filename);
}

void Parser::prepareScript()
{
    int index;
    QString tag;
    QStringList *part;
    QStringList *tmp = new QStringList();

    while((index = script->indexOf(QRegExp("<part.+"))) != -1)
    {
        tag = script->at(index);
        script->removeAt(index);
        part = importPart(tag.section("filename=\"", 1).section("\"", 0, 0));

        if(part!=NULL)
        {
            tmp->append(*part);
            delete part;
        }

    }

    delete script;
    script = tmp;
}

QStringList *Parser::importPart(const QString filename)
{
    QFile f(filename);
    //QRegExp rx("(\n|\r)*");
    QStringList *part = new QStringList;

    if(f.exists())
    {
        f.open(QIODevice::ReadOnly | QIODevice::Text);
        QStringList tmp = QTextCodec::codecForName("Shift-JIS")->toUnicode(f.readAll().data()).split("\n");
        f.close();

        foreach(QString line, tmp) {
            if(!line.indexOf("#") == 0 && !line.contains("【"))
                part->append(line);
        }

        return part;
    }

    delete part;
    return NULL;
}

int Parser::writeShiftJis(const QString filename, const QStringList part)
{
    QFile f(filename);
    f.open(QIODevice::WriteOnly | QIODevice::Text);
    f.write(QTextCodec::codecForName("Shift-JIS")->fromUnicode(part.join("\n")));
    f.close();

    return 0;
}

QStringList *Parser::readShiftJis(const QString filename)
{
    QStringList *file = new QStringList();

    QFile f(filename);
    f.open(QIODevice::ReadOnly | QIODevice::Text);
    QStringList tmp = QTextCodec::codecForName("Shift-JIS")->toUnicode(f.readAll().data()).split("\n");
    f.close();

    file->append(tmp);
    return file;

}

int Parser::exportParts(const QString filename)
{
    loadScript(filename);

    QString tag;
    int startIndex, endIndex;
    while((startIndex = script->indexOf(QRegExp("<part.+"))) != -1)
    {
        tag = script->at(startIndex);
        script->removeAt(startIndex);

        if((endIndex = script->indexOf("</part>")) == -1)
            return -1;
        script->removeAt(endIndex);

        writeShiftJis(tag.section("filename=\"", 1).section("\"", 0, 0), script->mid(startIndex, endIndex));
    }

    return 0;
}

int Parser::dumpStrings(const char *in)
{
    QString in_file = in;
    QStringList *script_lines = readShiftJis(in_file);
    QStringList output;
    output.append("");

    int start_index = script_lines->indexOf("SCENE_PROLOGUE:");
    if(start_index == -1) start_index = 0;

    QString str_token = "pushstring ";
    for(int i = start_index +1; i < script_lines->length(); i++)
    {
        QString line = script_lines->at(i);

        if(line.contains("#") && line.contains("SPEAK"))
            output.append(parse_name(line.section("#", 1)));

        if(line.contains(str_token) && line.length() > str_token.length() + 1)
            output.append(line.section(str_token, 1) + "\n");
    }

    writeShiftJis(in_file.section(".", 0, 0) + "_strings.txt", output);
    delete script_lines;
    return 0;
}

QByteArray Parser::insertStrings(const QString strings, const QString script, int char_limit)
{
    loadScript(strings);
    prepareScript();
    QStringList *tl_lines = getScript();
    QStringList *script_lines = readShiftJis(script);

    QStringList output;
    QString line, tmp;

    int start_index = script_lines->indexOf("SCENE_PROLOGUE:");
    if(start_index == -1) start_index = 0;
    output.append(script_lines->mid(0, start_index + 1));

    for(int i = start_index + 1; i < script_lines->length(); i++)
    {
        line = script_lines->at(i);

        if(line.contains("pushstring") && tl_lines->length() > 0)
        {
            tmp = line.section("pushstring ", 1);
            int index = -1;
            if(tmp.length() > 0)
                index = tl_lines->indexOf(tmp);
            if(index != -1 && tl_lines->at(index+1).length() > 0)
            {
                if(tl_lines->at(index+1).indexOf("@") == 0)
                    output.append("\tpushstring " + tl_lines->at(index+1).section("@", 1));
                else
                    output.append("\tpushstring " + wordWrap(tl_lines->at(index+1), line_limit, char_limit));

                tl_lines->removeAt(index);
                tl_lines->removeAt(index);
                tl_lines->removeAt(index);

                continue;
            }
            else
                output.append(line);
        }

        else
            output.append(line);
    }

    delete script_lines;
    return QTextCodec::codecForName("Shift-JIS")->fromUnicode(output.join("\n"));
}

const QString Parser::wordWrap(const QString str, int line_limit, int char_limit)
{
    QString line = str;
    int rest_size = str.length();
    int index = 0;

    while(--line_limit > 0 && rest_size > char_limit)
    {
        index = line.lastIndexOf(" ", index + char_limit);
        line.insert(index+1, "~");
        rest_size = line.length() - (index + 2);
    }

    return line;
}


QString Parser::parse_name(QString name_line)
{
    QString name = "【" + name_line.remove("　").remove(" ") + "】";
    return name;
}

Parser::~Parser()
{
    if(script != NULL)
        delete script;
}
