// code from Nick Gammon

// D18: pixel output
// D9 HSync
// D12 VSync

#include <avr/sleep.h>
#include "vga.h"

int main()
{
    VGA vga;
    vga.init();
    
    for (int i = 0; i < 30; i++)
    {
        vga.gotoxy(0, i);
        vga.write("Hello world");
    }

    while (true)
        sleep_mode();

    return 0; 
}




