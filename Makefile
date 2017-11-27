CC = avr-g++ --std=c++11 -mmcu=atmega2560
CMD = $(CC) -O2 -c
APP = app_vgacalc1.elf

%.o: %.cpp
	$(CMD) $<

%.elf: %.o
	$(CC) -o $@ $^

all: app_analogweb1.elf app_vga1.elf app_vgacalc1.elf app_web1.elf app_sdls1.elf \
    app_sdod1.elf

app_analogweb1.elf: app_analogweb1.o arp.o dns.o dhcp.o uip_server.o uip_client.o \
    uip_ethernet.o uip.o uip_udp.o network.o misc.o uip_timer.o
app_web1.elf: app_web1.o arp.o dns.o dhcp.o uip_server.o uip_client.o uip_ethernet.o \
    uip.o uip_udp.o network.o misc.o zd2card.o zd.o zdfile.o zdfat.o fyle.o uip_timer.o
app_sdls1.elf: app_sdls1.o zd2card.o zd.o zdfile.o zdfat.o fyle.o misc.o
app_sdod1.elf: app_sdod1.o zd2card.o misc.o
app_vga1.elf: app_vga1.o vga.o
app_vgacalc1.elf: app_vgacalc1.o calc.o vga.o misc.o keyboard.o

app_sdls1.o: app_sdls1.cpp
app_sdod1.o: app_sdod1.cpp
app_vga1.o: app_vga1.cpp screenFont.h vga.h
app_vgacalc1.o: app_vgacalc1.cpp screenFont.h keyboard.h calc.h
arp.o: arp.cpp arp.h
calc.o: calc.cpp calc.h
dhcp.o: dhcp.cpp dhcp.h
dns.o: dns.cpp dns.h
keyboard.o: keyboard.cpp keyboard.h
misc.o: misc.cpp misc.h
network.o: network.cpp
uip.o: uip.cpp uip.h
uip_client.o: uip_client.cpp uip_client.h
uip_ethernet.o: uip_ethernet.cpp uip_ethernet.h
uip_server.o: uip_server.cpp uip_server.h
uip_timer.o: uip_timer.cpp
uip_udp.o: uip_udp.cpp
vga.o: vga.cpp vga.h mega.h
zd2card.o: zd2card.cpp zd2card.h
zd.o: zd.cpp zd.h
zdfile.o: zdfile.cpp
zdfat.o: zdfat.cpp

download:
	avrdude -c stk500 -p m2560 -P /dev/ttyUSB0 -U $(APP)

arduino:
	avrdude -c wiring -p m2560 -P /dev/ttyACM0 -U $(APP)

clean:
	rm -vf *.o *.elf

rebuild: clean all



