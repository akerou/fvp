#ifndef NVSGCONVERTER_H
#define NVSGCONVERTER_H

#include <QFile>
#include <QMap>
#include <QString>
#include <QByteArray>
#include <QDataStream>

#include <lodepng.h>
#include <QDir>
#include <QTextCodec>


class NvsgConverter
{

public:

    static NvsgConverter & getInstance()
    {
        static NvsgConverter converter;
        return converter;
    }

    void autoProcess(const char *input, const char* process_mode, const char *output = NULL);

private:

    typedef struct
    {
        char *data;
        quint32 bytes;
        quint16 height;
        quint16 width;
        quint16 mode;
    } Image;

    typedef struct
    {
        quint16 x = 0;
        quint16 y = 0;
    } Pos;

    int decodeNvsg(const QString infile, const QString outfile);
    int encodeNvsg(const QString infile, const QString outfile);

    QMap<QString, Pos> positions;
    const QString d_mode = "-decode";
    const QString e_mode = "-encode";

    const char *HZC1_MAGIC = "hzc1";
    const char *NVSG_MAGIC = "NVSG";
    const char *pos_file = "pos.dat";

    void massProcess(const QDir in_dir, const QString out, const QString mode);

    NvsgConverter();
    NvsgConverter(const NvsgConverter&){}
    void operator=(NvsgConverter const&){}

    int imgFromString_save(const QString filename, QByteArray data, int width, int height, int mode);
    Image* imgToString(const QString filename);

    int readPosInfo();
    int appendPosInfo(QString filename, quint16 x_pos, quint16 y_pos);
    QByteArray pos_buf;
    int writePosInfo();

    bool checkMap(const QString filename);
    Pos getPos(const QString filename);

    QByteArray readFile(const QString infile);


};
#endif // NVSGCONVERTER_H
