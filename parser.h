#ifndef PARSER_H
#define PARSER_H

#include <QFile>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QTextCodec>
#include <QByteArray>
#include <QRegExp>
#include <QList>


class Parser
{
public:

    static Parser & getInstance()
    {
        static Parser parser;
        return parser;
    }

    ~Parser();

    QStringList *getScript();

    int dumpStrings(const char *in);
    QByteArray insertStrings(const QString strings, const QString script, int char_limit = default_charlimit);
    int exportParts(const QString filename);

private:

    void prepareScript();
    void loadScript(const QString filename);

    static const int line_limit = 3;
    static const int default_charlimit = 64;

    Parser() { script = NULL; }
    Parser(const Parser&){}
    void operator=(Parser const&){}
    QStringList *script;

    QStringList *importPart(const QString filename);
    int writeShiftJis(const QString filename, const QStringList part);
    QStringList *readShiftJis(const QString filename);

    QString parse_name(QString name_line);
    const QString wordWrap(const QString str, int line_limit, int char_limit);
};

#endif // PARSER_H
