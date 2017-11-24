#include "misc.h"

void MyBitset::addBit(uint8_t val)
{
    if (_bitcount - 1 <= 7)
        _incoming |= val << _bitcount - 1;

    _bitcount++;
}

void CircBuf::push(uint8_t v)
{
    uint8_t i = head + 1;

    if (i >= BUFFER_SIZE)
        i = 0;

    if (i != tail)
    {
        _buf[i] = v;
        head = i;
    }
}

uint8_t CircBuf::get(uint8_t pos)
{
    if (empty())
        return 0;

    tail = pos;
    return _buf[pos];
}

uint8_t CircBuf::get()
{
    uint8_t i = tail;

    if (++i >= BUFFER_SIZE)
        i = 0;

    return get(i);
}

void Serial::init() const
{
    UBRR0 = 103;    // 9600baud @16MHz
    UCSR0B = 1<<TXEN0;
}

void Serial::write(char c) const
{
    while (!(UCSR0A & (1<<UDRE0)))
        ;

    UDR0 = c;
}

uint8_t Serial::readByte() const
{
    while (!(UCSR0A & 1<<RXC0))
        ;

    return UDR0;
}

void Timer0::noToneA() const
{
    TCCR0A = 0;
    PORTD &= ~(1<<6);
}

void Timer0::noToneB() const
{
    TCCR0A = 0;
    PORTD &= ~(1<<5);
}

void Timer0::toneA(uint32_t freq) const
{
    TCCR0A = 1<<WGM01 | 1<<COM0A0;
    TCCR0B = 1<<CS02;
    DDRD |= 1<<6;
    uint8_t ocr;
    uint8_t prescalarbits;
    toneCalc(16000000, freq, ocr, prescalarbits);
    TCCR0B = (TCCR0B & 0b11111000) | prescalarbits;
    OCR0A = ocr;
}

void Timer0::toneB(uint32_t freq) const
{
    TCCR0A = 1<<WGM01 | 1<<COM0A0;
    TCCR0B = 1<<CS02;
    DDRD |= 1<<5;
    uint8_t ocr;
    uint8_t prescalarbits;
    toneCalc(16000000, freq, ocr, prescalarbits);
    TCCR0B = (TCCR0B & 0b11111000) | prescalarbits;
    OCR0A = ocr;
}

void Timer0::toneCalc(uint32_t f_cpu, uint32_t freq, uint8_t &ocr, uint8_t &cs) const
{
    if (freq > 31250)
    {
        ocr = f_cpu / (freq << 1) - 1;
        cs = 1<<CS20;            // clk/1 
    }
    else if (freq > 3907)
    {
        ocr = f_cpu / (freq << 4) - 1;
        cs = 1<<CS21;                // clk/8
    }
    else if (freq > 977)
    {
        ocr = f_cpu / (freq << 6) - 1;
        cs = 1<<CS21 | 1<<CS20;      // clk/32
    }
    else if (freq > 489)
    {
        ocr = f_cpu / (freq << 7) - 1;
        cs = 1<<CS22;                // clk/64
    }
    else if (freq > 245)
    {
        ocr = f_cpu / (freq << 8) - 1;
        cs = 1<<CS22 | 1<<CS20;      // clk/128
    }
    else if (freq > 123)
    {
        ocr = f_cpu / (freq << 9) - 1;
        cs = 1<<CS22 | 1<<CS21;      // clk/256
    }
    else
    {
        ocr = f_cpu / (freq << 11) - 1;
        cs = 1<<CS22 | 1<<CS21 | 1<<CS20;      // clk/1024
    }
}

void Timer1::noToneA() const
{
    TCCR1A = 0;
    PORTB &= ~(1<<1);
}

void Timer1::toneA(uint32_t frequency) const
{
    uint8_t prescalarbits = 1<<CS10;
    TCCR1A = 1<<COM1A0;
    TCCR1B = 1<<WGM12 | 1<<CS10;
    DDRB |= 1<<1;
    uint32_t ocr = 16000000 / frequency / 2 - 1;
    prescalarbits = 1<<CS10;    // clk/1

    if (ocr > 0xffff)
    {
        ocr = 16000000 / frequency / 2 / 64 - 1;
        prescalarbits = 1<<CS10 | 1<<CS11;  // clk/64
    }

    TCCR1B = (TCCR1B & 0b11111000) | prescalarbits;
    OCR1A = ocr;
}

void Timer2::noToneA() const
{
    TCCR2A = 0;
    PORTB &= ~(1<<3);
}

void Timer2::toneA(uint32_t freq) const
{
    TCCR2A = 1<<WGM21 | 1<<COM2A0;
    TCCR2B = 1<<CS22;
    DDRB |= 1<<3;
    uint8_t ocr;
    uint8_t prescalarbits;
    toneCalc(16000000, freq, ocr, prescalarbits);
    TCCR2B = (TCCR2B & 0b11111000) | prescalarbits;
    OCR2A = ocr;
}

void Timer2::toneCalc(uint32_t f_cpu, uint32_t freq, uint8_t &ocr, uint8_t &cs) const
{
    if (freq > 31250)
    {
        ocr = f_cpu / (freq << 1) - 1;
        cs = 1<<CS20;            // clk/1 
    }
    else if (freq > 3907)
    {
        ocr = f_cpu / (freq << 4) - 1;
        cs = 1<<CS21;                // clk/8
    }
    else if (freq > 977)
    {
        ocr = f_cpu / (freq << 6) - 1;
        cs = 1<<CS21 | 1<<CS20;      // clk/32
    }
    else if (freq > 489)
    {
        ocr = f_cpu / (freq << 7) - 1;
        cs = 1<<CS22;                // clk/64
    }
    else if (freq > 245)
    {
        ocr = f_cpu / (freq << 8) - 1;
        cs = 1<<CS22 | 1<<CS20;      // clk/128
    }
    else if (freq > 123)
    {
        ocr = f_cpu / (freq << 9) - 1;
        cs = 1<<CS22 | 1<<CS21;      // clk/256
    }
    else
    {
        ocr = f_cpu / (freq << 11) - 1;
        cs = 1<<CS22 | 1<<CS21 | 1<<CS20;      // clk/1024
    }
}




