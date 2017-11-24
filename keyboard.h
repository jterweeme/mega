#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_
#include <avr/pgmspace.h>
#include "misc.h"

static const uint8_t PS2_KEYMAP_SIZE = 136;

struct PS2Keymap_t
{
    uint8_t noshift[PS2_KEYMAP_SIZE];
    uint8_t shift[PS2_KEYMAP_SIZE];
    uint8_t uses_altgr;
    uint8_t altgr[PS2_KEYMAP_SIZE];
};

class PS2Keyboard
{
private:
    MyBitset _bitset;
    CircBuf _buf;
public:
    PS2Keyboard();
    void isr();
    void ps2interrupt();
    uint8_t get_scan_code();
};

class USKeyboard : public PS2Keyboard
{
private:
    static const uint8_t BREAK = 1, MODIFIER = 2, SHIFT_L = 4,
        SHIFT_R = 8, ALTGR = 0x10, CTRL = 0x20, PS2_TAB = 9,
        PS2_ENTER = 13, PS2_LINEFEED = 10, PS2_BACKSPACE = 127,
        PS2_ESC = 27, F1 = 0, F2 = 0, F3 = 0, F4 = 0, F5 = 0,
        F6 = 0, F7 = 0, F8 = 0, F9 = 0, F10 = 0, F11 = 0, F12 = 0,
        PS2_SCROLL = 0, INSERT = 0, HOME = 0, END = 0, DELETE = 127,
        PAGEUP = 25, PAGEDOWN = 26, UPARROW = 11, LEFTARROW = 8,
        DOWNARROW = 12, RIGHTARROW = 21;

    static const PROGMEM PS2Keymap_t PS2Keymap_US;
    const PS2Keymap_t *keymap = &PS2Keymap_US;
public:
    static uint8_t CharBuffer;
    static uint8_t UTF8next;
    bool available();
    int read();
    char get_iso8859_code();

    static const uint8_t N1 = 0x16, N2 = 0x1e, N3 = 0x26, N4 = 0x25, N5 = 0x2e,
        N6 = 0x36, N7 = 0x3d, N8 = 0x3e, N9 = 0x46, N0 = 0x45, NUM1 = 0x69,
        NUM2 = 0x72, NUM3 = 0x7a, PLUS = 0x79, MINUS = 0x7b, SLASH = 0x4a,
        ENTER = 0x5a, ESC = 0x76, NUMLOCK = 0x77, STAR = 0x7c;   
};

#endif


