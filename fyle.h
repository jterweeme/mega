/*

 SD - a slightly more friendly wrapper for sdfatlib

 This library aims to expose a subset of SD card functionality
 in the form of a higher level "wrapper" object.

 License: GNU General Public License V3
          (Because sdfatlib is licensed with this.)

 (C) Copyright 2010 SparkFun Electronics

 */

#ifndef _FYLE_H_
#define _FYLE_H_

#include "zdfat.h"
#include "zdfatutil.h"

#define FILE_READ O_READ
#define FILE_WRITE (O_READ | O_WRITE | O_CREAT)

class Fyle //: public Stream
{
private:
    char _name[13]; // our name
    SdFile *_file;  // underlying file pointer
public:
    Fyle(SdFile f, const char *name);     // wraps an underlying SdFile
    Fyle();      // 'empty' constructor
    ~Fyle();     // destructor
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *buf, size_t size);
    virtual int read();
    virtual int peek();
    virtual int available();
    virtual void flush();
    int read(void *buf, uint16_t nbyte);
    bool seek(uint32_t pos);
    uint32_t position();
    uint32_t size();
    void close();
    operator bool();
    char * name();
    bool isDirectory(void);
    Fyle openNextFile(uint8_t mode = O_RDONLY);
    void rewindDirectory(void);
    //using Print::write;
};

#endif



