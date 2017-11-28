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

class Buffer
{
    char _buf[512] = {0};
    uint16_t _pos = 0;
public:
    void push(char c) { _buf[_pos++] = c; }
    char *get() { return _buf; }
    void reset();
    bool end() const;
};

bool Buffer::end() const
{
    return _buf[_pos - 4] == 0x0d && _buf[_pos - 3] == 0x0a && _buf[_pos - 2] == 0x0d &&
        _buf[_pos - 1] == 0x0a ? true : false;
}

void Buffer::reset()
{
    _pos = 0;

    for (uint16_t i = 0; i < 512; i++)
        _buf[i] = 0;
}

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
    Buffer buffer;

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
                    buffer.push(c);

                    if (buffer.end())
                    {
                        if (strncmp("GET ", buffer.get(), 4) == 0)
                        {
                            char fn[100] = {0};
                            char ext[10] = {0};
                            
                            for (uint16_t i = 5; i < 512; i++)
                            {
                                if (buffer.get()[i] != ' ')
                                    fn[i - 5] = buffer.get()[i];
                                else
                                    break;
                            }
                            
                            if (fn[0] == 0)
                                strncpy(fn, "index.htm", 100);

                            size_t dot = 0;

                            for (uint8_t i = 0; i < strlen(fn); i++)
                                if (fn[i] == '.')
                                    dot = i;

                            for (char *p = fn + dot + 1, *d = ext; *p != 0; p++)
                                *d++ = *p;
                            
                            serial.write(ext);
                            serial.write("\r\n");
                            serial.write(fn);
                            serial.write("\r\n");
                            serial.write(buffer.get());
                            myFile = zd.open(fn);
                            client.write("HTTP/1.1 200 OK\r\n");

                            if (strncmp(ext, "svg", 3) == 0)
                                client.write("Content-Type: image/svg+xml\r\n");
                            else
                                client.write("Content-Type: text/html\r\n");

                            client.write("Connection: close\r\n\r\n");  // let op de dubbel nl
                        
                            if (myFile)
                                while (myFile.available())
                                    client.write(myFile.read());

                            myFile.close();                               
                        }
                        else if (strncmp("PUT ", buffer.get(), 4) == 0)
                        {
                            serial.write(buffer.get());
                        }

                        buffer.reset();
                        break;
                    }
                }
            }
            _delay_ms(1);
            client.stop();
            //Serial.println("client disonnected");
        }
    }

    return 0;
}





