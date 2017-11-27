/*

 SD - a slightly more friendly wrapper for sdfatlib

 This library aims to expose a subset of SD card functionality
 in the form of a higher level "wrapper" object.

 License: GNU General Public License V3
          (Because sdfatlib is licensed with this.)

 (C) Copyright 2010 SparkFun Electronics

 */

#ifndef _ZD_H_
#define _ZD_H_

#include "zdfat.h"
#include "zdfatutil.h"

#define FILE_READ O_READ
#define FILE_WRITE (O_READ | O_WRITE | O_CREAT)
#include "fyle.h"

class ZD
{
private:
    Sd2Card card;
    SdVolume volume;
    SdFile root;
    SdFile getParentDir(const char *filepath, int *indx);
public:
    bool begin(uint8_t csPin = SD_CHIP_SELECT_PIN);
    Fyle open(const char *filename, uint8_t mode = FILE_READ);
    bool exists(char *filepath);
    bool mkdir(char *filepath);
    bool remove(char *filepath);
    bool rmdir(char *filepath);
private:
    int fileOpenMode;
    friend class Fyle;
    friend bool callback_openPath(SdFile&, char *, bool, void *); 
};

extern ZD g_zd;

#endif
