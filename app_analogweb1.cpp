/*
Mega eth CS: D53
*/

#include "uip_ethernet.h"
#include "uip_server.h"
#include "uip_client.h"
#include <avr/interrupt.h>
#include "misc.h"

#define F_CPU 16000000UL
#include <util/delay.h>

EthernetServer server = EthernetServer(80);

uint32_t g_millis = 0;

ISR(TIMER0_OVF_vect)
{
    g_millis++;
}

inline char nibble(uint8_t n)
{
    return n <= 9 ? '0' + n : 'A' + n - 10;
}

int main()
{
    // 16,000,000/16,000 = 1000
    // 16,000 / 256 = 62

    Serial serial;
    serial.init();
    TCCR0B = CS02; // | CS00;
    TIMSK0 |= 1<<TOIE0;
    sei();

    uint8_t mac[6] = {0x00,0x01,0x02,0x03,0x04,0x05};
    serial.write("begin ethernet\r\n");
    IPAddrezz myIP(192,168,200,56);
    Ethernet.begin(mac, myIP);
    serial.write("My IP address: \r\n");
    uint32_t ip = Ethernet.localIP();
    //ip = millis();
    
    for (int8_t i = 7; i >= 0; i--)
        serial.write(nibble(ip >> (i << 2) & 0xf));
  
    serial.write("\r\n");
    server.begin();
    

    while (true)
    {
        EthernetClient client = server.available();

        if (client)
        {
            //Serial.println("new client");
            bool currentLineIsBlank = true;
        
            while (client.connected())
            {
                if (client.available())
                {
                    char c = client.read();
                    //Serial.write(c);

        if (c == '\n' && currentLineIsBlank)
        {
          // send a standard http response header
          client.print("HTTP/1.1 200 OK\r\n");
          client.print("Content-Type: text/html\r\n");
          client.print("Connection: close\r\n");
          client.print("Refresh: 5\r\n\r\n");  // refresh the page automatically every 5 sec
          
          client.print("<!DOCTYPE HTML>\r\n");
          client.print("<html>\r\n");
          // output the value of each analog input pin
          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
            int sensorReading = 100;
            client.print("analog input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.print("<br />\r\n");
          }
          client.print("</html>\r\n");
           break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
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





