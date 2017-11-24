// original code by Nick Gammon, Australia

#include "vga.h"
#include <avr/interrupt.h>
#include "screenFont.h"
#include "mega.h"

ISR(__vector_20)    // timer1_ovf
{
    VGA::g_vga->vsync();
}

ISR(__vector_15)    // timer2_ovf
{
    VGA::g_vga->hsync();
}

uint32_t g_vsync = 0;

void VGA::vsync()
{
    g_vsync++;
    vLine = 0;
    messageLine = 0;
    backPorchLinesToGo = verticalBackPorchLines;
}

VGA *VGA::g_vga;

void VGA::hsync()
{
    if (backPorchLinesToGo)
    {
        backPorchLinesToGo--;
        return;
    }  // end still doing back porch

    // if all lines done, do the front porch
    if (vLine >= verticalPixels)
        return;

    // pre-load pointer for speed
    const register uint8_t *linePtr = &screen_font[(vLine >> 1) & 0x07][0];
    register char *messagePtr = &(_message[messageLine][0]);
    register uint8_t i = _cols; // how many pixels to send
    *p_ucsr1b = 1<<txen1;  // transmit enable (starts transmitting white)

    while (i--)
        *p_udr1 = pgm_read_byte(linePtr + (*messagePtr++)); // blit pixel data to screen

    // wait till done
    while (!(*p_ucsr1a & 1<<txc1))
        ;

    *p_ucsr1b = 0;   // disable transmit; drop back to black
    vLine++;    // finished this line

    // every 16 pixels it is time to move to a new line in our text
    //  (because we double up the characters vertically)
    if ((vLine & 0xF) == 0)
        messageLine++;
}

void VGA::clear()
{
    for (uint8_t row = 0; row < _rows; row++)
        for (uint8_t col = 0; col < _cols; col++)
            _message[row][col] = ' ';

    gotoxy(0, 0);
}

void VGA::init()
{
    g_vga = this;
    sei();
  
    // Timer 1 - vertical sync pulses
    *p_ddrb |= 1<<ddb6;
    *p_ddrh = 1<<ddh6;
    *ptccr1a = 1<<wgm10 | 1<<wgm11 | 1<<com1b1;
    *ptccr1b = 1<<wgm12 | 1<<wgm13 | 1<<cs12 | 1<<cs10;
    *pocr1a = 259;  // 16666 / 64 uS = 260 (less one)
    *pocr1b = 0;    // 64 / 64 uS = 1 (less one)
    *p_tifr1 = 1<<tov1;   // clear overflow flag
    *p_timsk1 = 1<<toie1;  // interrupt on overflow on timer 1

    // Timer 2 - horizontal sync pulses
    *p_ddrd = 0xff;
    *p_tccr2a = 1<<wgm20 | 1<<wgm21 | 1<<com2b1;
    *p_tccr2b = 1<<wgm22 | 1<<cs21;
    *p_ocr2a = 63;   // 32 / 0.5 uS = 64 (less one)
    *p_ocr2b = 7;    // 4 / 0.5 uS = 8 (less one)
    *p_tifr2 = 1<<tov2;   // clear overflow flag
    *p_timsk2 = 1<<toie2;  // interrupt on overflow on timer 2
 
    *p_ubrr1 = 0;  // USART Baud Rate Register
    *p_ddrd |= 1<<ddd4;
    *p_ucsr1b = 0;
    *p_ucsr1c = 1<<umsel10 | 1<<umsel11 | 1<<ucpol1;  // Master SPI mode
}


