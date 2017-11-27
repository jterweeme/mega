/*

 SD - a slightly more friendly wrapper for sdfatlib

 This library aims to expose a subset of SD card functionality
 in the form of a higher level "wrapper" object.

 License: GNU General Public License V3
          (Because sdfatlib is licensed with this.)

 (C) Copyright 2010 SparkFun Electronics

 */

#include "fyle.h"
#include <stdlib.h>
#include <string.h>
/* for debugging file open/close leaks
   uint8_t nfilecount=0;
*/

Fyle::Fyle(SdFile f, const char *n) {
  // oh man you are kidding me, new() doesnt exist? Ok we do it by hand!
  _file = (SdFile *)malloc(sizeof(SdFile)); 
  if (_file) {
    memcpy(_file, &f, sizeof(SdFile));
    
    strncpy(_name, n, 12);
    _name[12] = 0;
    
    /* for debugging file open/close leaks
       nfilecount++;
       Serial.print("Created \"");
       Serial.print(n);
       Serial.print("\": ");
       Serial.println(nfilecount, DEC);
    */
  }
}

Fyle::Fyle() {
  _file = 0;
  _name[0] = 0;
  //Serial.print("Created empty file object");
}

Fyle::~Fyle(void) {
  //  Serial.print("Deleted file object");
}

// returns a pointer to the file name
char *Fyle::name(void) {
    return _name;
}

// a directory is a special type of file
bool Fyle::isDirectory(void) {
  return (_file && _file->isDir());
}


size_t Fyle::write(uint8_t val) {
  return write(&val, 1);
}

size_t Fyle::write(const uint8_t *buf, size_t size) {
  size_t t;
  if (!_file) {
    //setWriteError();
    return 0;
  }
  //_file->clearWriteError();
  t = _file->write(buf, size);
#if 0
  if (_file->getWriteError()) {
    setWriteError();
    return 0;
  }
#endif
  return t;
}

int Fyle::peek() {
  if (! _file) 
    return 0;

  int c = _file->read();
  if (c != -1) _file->seekCur(-1);
  return c;
}

int Fyle::read() {
  if (_file) 
    return _file->read();
  return -1;
}

// buffered read for more efficient, high speed reading
int Fyle::read(void *buf, uint16_t nbyte) {
  if (_file) 
    return _file->read(buf, nbyte);
  return 0;
}

int Fyle::available() {
  if (! _file) return 0;

  uint32_t n = size() - position();

  return n > 0X7FFF ? 0X7FFF : n;
}

void Fyle::flush() {
  if (_file)
    _file->sync();
}

bool Fyle::seek(uint32_t pos) {
  if (! _file) return false;

  return _file->seekSet(pos);
}

uint32_t Fyle::position() {
  if (! _file) return -1;
  return _file->curPosition();
}

uint32_t Fyle::size() {
  if (! _file) return 0;
  return _file->fileSize();
}

void Fyle::close() {
  if (_file) {
    _file->close();
    free(_file); 
    _file = 0;

    /* for debugging file open/close leaks
    nfilecount--;
    Serial.print("Deleted ");
    Serial.println(nfilecount, DEC);
    */
  }
}

Fyle::operator bool() {
  if (_file) 
    return  _file->isOpen();
  return false;
}



