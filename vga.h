#ifndef _VGA_H_
#define _VGA_H_
#include <stdint.h>

extern uint32_t g_vsync;

class VGA
{
private:
    static constexpr int _cols = 20;  // 160 pixels wide
    static constexpr int verticalPixels = 480;  // 480 pixels high
    static constexpr uint8_t _fontHeight = 8, _fontWidth = 8, _rows = 30;
    static constexpr int horizontalPixels = _rows * _fontWidth;
    static constexpr uint8_t verticalBackPorchLines = 35;  // includes sync pulse?
    static constexpr uint16_t verticalFrontPorchLines = 525 - verticalBackPorchLines;
    volatile int vLine;
    volatile int messageLine;
    volatile uint8_t backPorchLinesToGo;
    char _message[_rows][_cols] = {{0,0}};
    uint8_t _posX = 0;
    uint8_t _posY = 0;
public:
    static VGA *g_vga;
    void init();
    void clear();

    inline void write(char c)
    { if (_posX >= 20) { _posX = 0; _posY++; } _message[_posY][_posX++] = c; }

    inline void write(const char *s) { while (*s) write(*s++); }
    inline void gotoxy(uint8_t x, uint8_t y) { _posX = x; _posY = y; }
    inline void vsync();
    inline void hsync();
};

#endif


