#include "fyle.h"
#include "zd.h"
#include "misc.h"
#include <avr/interrupt.h>
#include <stdio.h>

uint32_t g_millis = 0;

ISR(TIMER0_OVF_vect)
{
    g_millis++;
}

uint32_t millis()
{
    return g_millis;
}

void printDirectory(Fyle dir, int numTabs, Serial &serial)
{
    while (true)
    {
        Fyle entry = dir.openNextFile();

        if (!entry)
            break;

        for (uint8_t i = 0; i < numTabs; i++)
            serial.write("\t");

        serial.write(entry.name());
        
        if (entry.isDirectory())
        {
            serial.write("/");
            printDirectory(entry, numTabs + 1, serial);
        }
        else
        {
            serial.write("\t\t");
            char buf[50];
            snprintf(buf, 50, "%u\r\n", entry.size());
            serial.write(buf);
        }
        entry.close();
    }
}

int main()
{
    TCCR0B = CS02;
    TIMSK0 |= 1<<TOIE0;
    sei();
    Fyle myFile;
    Serial serial;
    serial.init();
    ZD zd;

    if (!zd.begin(9))
    {
        serial.write("initialization failed!\r\n");
        return 0;
    }

    myFile = zd.open("/");
    printDirectory(myFile, 0, serial);

    while (true)
        ;

    return 0;
}


