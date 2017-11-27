/*
Mega eth CS: D53
*/

#include "uip_ethernet.h"
#include "uip_server.h"
#include "uip_client.h"
#include <avr/interrupt.h>
#include "misc.h"
#include "fyle.h"
#include "zd.h"
#include <stdio.h>

#define F_CPU 16000000UL
#include <util/delay.h>

uint32_t g_millis = 0;

ISR(TIMER0_OVF_vect)
{
    g_millis++;
}

uint32_t millis()
{
    return g_millis;
}

inline char nibble(uint8_t n)
{
    return n <= 9 ? '0' + n : 'A' + n - 10;
}

static uint8_t g_count = 0;

int main()
{
    // 16,000,000/16,000 = 1000
    // 16,000 / 256 = 62

    UIPServer server = UIPServer(80);
    Fyle myFile;
    Serial serial;
    serial.init();
    TCCR0B = CS02; // | CS00;
    TIMSK0 |= 1<<TOIE0;
    sei();

    uint8_t mac[6] = {0x00,0x01,0x02,0x03,0x04,0x05};
    serial.write("begin ethernet\r\n");
    IPAddrezz myIP(192,168,200,56);
    UIPEthernet.begin(mac, myIP);
    serial.write("My IP address: \r\n");
    uint32_t ip = UIPEthernet.localIP();
    
    for (int8_t i = 7; i >= 0; i--)
        serial.write(nibble(ip >> (i << 2) & 0xf));
  
    serial.write("\r\n");

    ZD zd;
    
    if (!zd.begin(9))
    {
        serial.write("SD initialization failed!\r\n");
    }

    server.begin();
    

    while (true)
    {
        UIPClient client = server.available();

        if (client)
        {
            //Serial.println("new client");
            bool currentLineIsBlank = true;
        
            while (client.connected())
            {
                if (client.available())
                {
                    char c = client.read();
                    serial.write(c);

                    if (c == '\n' && currentLineIsBlank)
                    {
                        myFile = zd.open("index.htm");
                        client.write("HTTP/1.1 200 OK\r\n");
                        client.write("Content-Type: text/html\r\n");
                        client.write("Connection: close\r\n");
                        client.write("Refresh: 5\r\n\r\n");
                        
                        if (myFile)
                            while (myFile.available())
                                client.write(myFile.read());

                        myFile.close();
                        break;
                    }

                    if (c == '\n')
                        currentLineIsBlank = true;
                    else if (c != '\r')
                        currentLineIsBlank = false;
                }
            }
            _delay_ms(1);
            client.stop();
            //Serial.println("client disonnected");
        }
    }

    return 0;
}





