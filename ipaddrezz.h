#ifndef _IPADDREZZ_H_
#define _IPADDREZZ_H_

#define htons(x) ( ((x)<<8) | (((x)>>8)&0xFF) )
#define ntohs(x) htons(x)

#define htonl(x) ( ((x)<<24 & 0xFF000000UL) | \
                   ((x)<< 8 & 0x00FF0000UL) | \
                   ((x)>> 8 & 0x0000FF00UL) | \
                   ((x)>>24 & 0x000000FFUL) )
#define ntohl(x) htonl(x)

#define Ethernet UIPEthernet
#define EthernetServer UIPServer
#define EthernetUDP UIPUDP

class IPAddrezz
{
private:
    union {
        uint8_t a8[4];  // IPv4 address
        uint32_t a32;
    } _address;

    uint8_t* raw_address() { return _address.a8; }
public:
    IPAddrezz() { memset(_address.a8, 0, sizeof(_address)); }
    IPAddrezz(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet);
    IPAddrezz(uint32_t address) { _address.a32 = address; }
    IPAddrezz(const uint8_t *address) { memcpy(_address.a8, address, sizeof(_address)); }
    operator uint32_t() { return _address.a32; }
    bool operator==(const IPAddrezz& addr) { return _address.a32 == addr._address.a32; };
    bool operator==(const uint8_t* addr);
    uint8_t operator[](int index) const { return _address.a8[index]; };
    uint8_t& operator[](int index) { return _address.a8[index]; };
    IPAddrezz& operator=(const uint8_t *address);
    IPAddrezz& operator=(uint32_t address);
    friend class UDeeP;
    friend class Klient;
    friend class DNSClient;
};

#endif



