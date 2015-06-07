#include "nvsgconverter.h"
#include <QDir>

NvsgConverter::NvsgConverter()
{
    readPosInfo();
}

void NvsgConverter::autoProcess(const char *input, const char *process_mode, const char *output)
{
    QString in = input;
    QString out = output;
    QString mode = process_mode;

    QDir inFolder(in);

    if(inFolder.exists())
        massProcess(inFolder, out, mode);

    else
    {
        if(mode == e_mode)
        {
            if(out == NULL)
                out = in.section(".", -1, 0);
            encodeNvsg(in, out);
        }
        else if(mode == d_mode)
        {
            if(out == NULL)
                out = in + ".png";
            decodeNvsg(in, out);
            writePosInfo();
        }

    }
}

void NvsgConverter::massProcess(QDir in_dir, const QString out, const QString mode)
{
    QStringList filenames = in_dir.entryList(QDir::Files);
    QDir out_dir;

    if(out == NULL)
        out_dir.setPath(in_dir.dirName() + "_converted");
    else
        out_dir.setPath(out);

    if(!out_dir.exists())
        QDir().mkdir(out_dir.dirName());

    if(mode == e_mode)
    {
        foreach(QString filename, filenames)
            encodeNvsg(in_dir.absolutePath() + "/" + filename, out_dir.absolutePath() + "/" + filename.section(".", 0, 0));
    }
    else if(mode == d_mode)
    {
        foreach(QString filename, filenames)
            decodeNvsg(in_dir.absolutePath() + "/" + filename, out_dir.absolutePath() + "/" + filename + ".png");
        writePosInfo();
    }

}

int NvsgConverter::decodeNvsg(const QString infile, const QString outfile)
{
    QByteArray data = readFile(infile);

    char *header = new char[44];
    memcpy(header, data.data(), 44);
    data = data.mid(44);

    if(strncmp(header, HZC1_MAGIC, 4) != 0 || strncmp(header + 12, NVSG_MAGIC, 4))
        return -1;

    quint32 uncompressed_size;
    memcpy(&uncompressed_size, header + 4, 4);

    quint16 format, width, height, x, y;
    memcpy(&format, header + 18, 2);
    memcpy(&width, header + 20, 2);
    memcpy(&height, header + 22, 2);
    memcpy(&x, header + 24, 2);
    memcpy(&y, header + 26, 2);

    QByteArray size;
    QDataStream ds(&size, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << uncompressed_size;
    data.prepend(size);

    data = qUncompress(data);

    if(x != 0 || y != 0)
        if(!checkMap(infile))
            appendPosInfo(infile, x, y);

    imgFromString_save(outfile, data, width, height, format);

    delete header;
    return 0;
}

int NvsgConverter::encodeNvsg(const QString infile, const QString outfile)
{
    Image *img = imgToString(infile);
    Pos pos = getPos(infile.section(".", 0, 0));
    quint32 nvsg_header = 32;
    quint16 format = 1;
    quint16 unk = 256;
    quint32 img_count = 0;

    char *header = new char[44];
    memset(header, 0x00, 44);
    mempcpy(header, HZC1_MAGIC, 4);
    mempcpy(header + 4, (char*) &(img->bytes), 4);
    memcpy(header + 8, (char*) &nvsg_header, 4);
    mempcpy(header + 12, NVSG_MAGIC, 4);
    mempcpy(header + 16, (char*) &unk, 2);
    memcpy(header + 18, (char*) &format, 2);
    memcpy(header + 20, (char*) &(img->width), 2);
    memcpy(header + 22, (char*) &(img->height), 2);
    memcpy(header + 24, (char*) &pos.x, 2);
    memcpy(header + 26, (char*) &pos.y, 2);
    memcpy(header + 32, (char*) &img_count, 4);

    QFile outf(outfile);
    outf.open(QIODevice::WriteOnly);
    outf.write(QByteArray::fromRawData(header, 44).append(qCompress(QByteArray::fromRawData(img->data, img->bytes), 9).remove(0, 4)));
    outf.close();

    delete header;
    delete img;

    return 0;
}

int NvsgConverter::imgFromString_save(const QString filename, QByteArray data, int width, int height, int mode)
{
    unsigned char *img = new unsigned char[data.length()];
    const char *tmp = data.data();

    if(mode==0)
    {
        for(int h=0; h<height; h++)
            for(int w=0; w<width*3; w+=3)
            {
                img[h*width*3 + w]      = tmp[h*width*3 + w + 2];
                img[h*width*3 + w + 1]  = tmp[h*width*3 + w + 1];
                img[h*width*3 + w + 2]  = tmp[h*width*3 + w];
            }

        lodepng_encode24_file(filename.toStdString().c_str(), img, width, height);

    }
    else if(mode==1)
    {
        for(int h=0; h<height; h++)
            for(int w=0; w<width*4; w+=4)
            {
                img[h*width*4 + w]      = tmp[h*width*4 + w + 2];
                img[h*width*4 + w + 1]  = tmp[h*width*4 + w + 1];
                img[h*width*4 + w + 2]  = tmp[h*width*4 + w];
                img[h*width*4 + w + 3]  = tmp[h*width*4 + w + 3];
            }

        lodepng_encode32_file(filename.toStdString().c_str(), img, width, height);
    }

    delete img;
    return 0;
}

NvsgConverter::Image *NvsgConverter::imgToString(const QString filename)
{
    unsigned char *tmp;

    FILE *fp;
    unsigned char *imgbuf;
    int fsize;

    fp = fopen(QTextCodec::codecForName("Shift-JIS")->fromUnicode(filename).data(), "rb");
    if (fp == NULL)
    {
        printf("error opening %s\n", filename.toStdString().c_str());
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    imgbuf = new unsigned char[fsize];

    fread(imgbuf, 1, fsize, fp);
    fclose(fp);

    unsigned width, height;
    LodePNGState *state = new LodePNGState();

    lodepng_decode(&tmp, &width, &height, state, imgbuf, fsize);
    delete imgbuf;

    Image *img = new Image();
    img->height = height;
    img->width = width;

    if(state->info_png.color.colortype == 2)
    {
        img->bytes = width*height*3;
        char *data = new char[width*height*3];

        for(int h=0; h<img->height; h++)
            for(int w=0; w<img->width*3; w+=3)
            {
                data[h*width*3 + w]      = tmp[h*width*3 + w + 2];
                data[h*width*3 + w + 1]  = tmp[h*width*3 + w + 1];
                data[h*width*3 + w + 2]  = tmp[h*width*3 + w];
            }

        img->data = data;
    }

    else if(state->info_png.color.colortype == 6)
    {
        img->bytes = width*height*4;
        char *data = new char[width*height*4];

        for(int h=0; h<img->height; h++)
            for(int w=0; w<img->width*4; w+=4)
            {
                data[h*width*4 + w]      = tmp[h*width*4 + w + 2];
                data[h*width*4 + w + 1]  = tmp[h*width*4 + w + 1];
                data[h*width*4 + w + 2]  = tmp[h*width*4 + w];
                data[h*width*4 + w + 3]  = tmp[h*width*4 + w + 3];
            }
        img->data = data;
    }

    delete state;
    free(tmp);

    return img;
}

int NvsgConverter::readPosInfo()
{
    QMap<QString, Pos> positions;

    QFile f(pos_file);
    if(f.exists()) {
        f.open(QIODevice::ReadOnly);
        QByteArray pos_data = qUncompress(f.readAll());
        f.close();

        int index = 0;
        while(index < pos_data.length() - 6)
        {
            quint16 str_len;
            Pos pos;
            memcpy(&str_len, pos_data.data() + index, 2);
            char *filename = new char[str_len];

            memcpy(filename, pos_data.data() + index + 2, str_len);
            memcpy(&pos.x, pos_data.data() + index + 2 + str_len, 2);
            memcpy(&pos.y, pos_data.data() + index + 2 + str_len + 2, 2);
            positions.insert(filename, pos);
            index += str_len + 6;
        }

    }

    this->positions = positions;

    return 0;
}

int NvsgConverter::appendPosInfo(QString filename, quint16 x_pos, quint16 y_pos)
{
    if(filename.contains("/"))
        filename = filename.section("/", -1);

    quint16 str_len = filename.length();
    char *entry = new char[str_len + 6];
    memcpy(entry, (char*) &str_len, 2);
    memcpy(entry + 2, (char*) filename.toStdString().c_str(), str_len);
    memcpy(entry + 2 + str_len, (char*) &x_pos, 2);
    memcpy(entry + 2 + str_len + 2, (char*) &y_pos, 2);

    pos_buf.append(QByteArray::fromRawData(entry, str_len + 6));
    return 0;
}

bool NvsgConverter::checkMap(const QString filename)
{
    QString key;
    if(filename.contains("/"))
        key = filename.section("/", -1);
    else
        key = filename;

    return positions.contains(key);
}

NvsgConverter::Pos NvsgConverter::getPos(const QString filename)
{
    QString key;
    if(filename.contains("/"))
        key = filename.section("/", -1);
    else
        key = filename;
    Pos pos;
    if(positions.contains(key))
        pos = positions.value(key);

    return pos;
}

int NvsgConverter::writePosInfo()
{
    if(pos_buf.length() > 6)
    {
        QByteArray writeData = qCompress(pos_buf, 9);
        QFile f(pos_file);
        f.open(QIODevice::WriteOnly | QIODevice::Append);
        f.write(writeData.data(), writeData.length());
        f.close();
    }

    return 0;
}

QByteArray NvsgConverter::readFile(const QString infile)
{
    QFile inputfile(infile);
    inputfile.open(QIODevice::ReadOnly);
    QByteArray data = inputfile.readAll();
    inputfile.close();

    return data;
}

