#include "network.h"
#include "util.h"
#include <avr/io.h>
#include <string.h>
#include "board.h"

#define F_CPU 16000000UL
#include <util/delay.h>

#define CSACTIVE (PORTB &= ~(1<<0))
#define CSPASSIVE (PORTB |= 1<<0)
#define waitspi() while(!(SPSR&(1<<SPIF)))

uint16_t Enc28J60Network::nextPacketPtr;
uint8_t Enc28J60Network::bank=0xff;

struct memblock Enc28J60Network::receivePkt;

void Enc28J60Network::init(uint8_t* macaddr)
{
  MemoryPool::init(); // 1 byte in between RX_STOP_INIT and pool to allow prepending of controlbyte
  // initialize I/O
  // ss as output:
#if 0
  pinMode(ENC28J60_CONTROL_CS, OUTPUT);
  CSPASSIVE; // ss=0
#else
    *p_ddr_ss |= 1<<pss;
    //DDRB |= 1<<PB0;
    //PORTB |= 1<<PB0;
    *p_port_ss |= 1<<pss;
#endif

#if 0
  pinMode(SPI_MOSI, OUTPUT);
  pinMode(SPI_SCK, OUTPUT);
  pinMode(SPI_MISO, INPUT);
  pinMode(SPI_SS, OUTPUT);

  digitalWrite(SPI_MOSI, LOW);
  digitalWrite(SPI_SCK, LOW);
#else
    DDRB |= 1<<2 | 1<<1 | 1<<PB0;
    DDRB &= ~(1<<3);
    PORTB &= ~(1<<2);   //mosi
    PORTB &= ~(1<<1);   //sck
#endif
    SPCR = (1<<SPE)|(1<<MSTR);
    SPSR |= (1<<SPI2X);
    writeOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
    _delay_ms(50);
    nextPacketPtr = RXSTART_INIT;
    writeRegPair(ERXSTL, RXSTART_INIT);
    writeRegPair(ERXRDPTL, RXSTART_INIT);
    writeRegPair(ERXNDL, RXSTOP_INIT);
    writeReg(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN|ERXFCON_BCEN);
    writeRegPair(EPMM0, 0x303f);
    writeRegPair(EPMCSL, 0xf7f9);
    writeRegPair(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
    writeOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
    writeRegPair(MAIPGL, 0x0C12);
    writeReg(MABBIPG, 0x12);
    writeRegPair(MAMXFLL, MAX_FRAMELEN);
    writeReg(MAADR5, macaddr[0]);
    writeReg(MAADR4, macaddr[1]);
    writeReg(MAADR3, macaddr[2]);
    writeReg(MAADR2, macaddr[3]);
    writeReg(MAADR1, macaddr[4]);
    writeReg(MAADR0, macaddr[5]);
    phyWrite(PHCON2, PHCON2_HDLDIS);
    setBank(ECON1);
    writeOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
    writeOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
    phyWrite(PHLCON,0x476);
}

memhandle Enc28J60Network::receivePacket()
{
  uint8_t rxstat;
  uint16_t len;
  // check if a packet has been received and buffered
  //if( !(readReg(EIR) & EIR_PKTIF) ){
  // The above does not work. See Rev. B4 Silicon Errata point 6.
  if (readReg(EPKTCNT) != 0)
    {
      uint16_t readPtr = nextPacketPtr+6 > RXSTOP_INIT ? nextPacketPtr+6-RXSTOP_INIT+RXSTART_INIT : nextPacketPtr+6;
      // Set the read pointer to the start of the received packet
      writeRegPair(ERDPTL, nextPacketPtr);
      // read the next packet pointer
      nextPacketPtr = readOp(ENC28J60_READ_BUF_MEM, 0);
      nextPacketPtr |= readOp(ENC28J60_READ_BUF_MEM, 0) << 8;
      // read the packet length (see datasheet page 43)
      len = readOp(ENC28J60_READ_BUF_MEM, 0);
      len |= readOp(ENC28J60_READ_BUF_MEM, 0) << 8;
      len -= 4; //remove the CRC count
      // read the receive status (see datasheet page 43)
      rxstat = readOp(ENC28J60_READ_BUF_MEM, 0);
      //rxstat |= readOp(ENC28J60_READ_BUF_MEM, 0) << 8;
      writeOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
      // check CRC and symbol errors (see datasheet page 44, table 7-3):
      // The ERXFCON.CRCEN is set by default. Normally we should not
      // need to check this.
      if ((rxstat & 0x80) != 0)
        {
          receivePkt.begin = readPtr;
          receivePkt.size = len;
          return UIP_RECEIVEBUFFERHANDLE;
        }
      // Move the RX read pointer to the start of the next received packet
      // This frees the memory we just read out
      setERXRDPT();
    }
  return (NOBLOCK);
}

void Enc28J60Network::setERXRDPT()
{
    writeRegPair(ERXRDPTL, nextPacketPtr == RXSTART_INIT ? RXSTOP_INIT : nextPacketPtr-1);
}

memaddress Enc28J60Network::blockSize(memhandle handle)
{
    return handle == NOBLOCK ? 0 : handle == UIP_RECEIVEBUFFERHANDLE ?
        receivePkt.size : blocks[handle].size;
}

void
Enc28J60Network::sendPacket(memhandle handle)
{
  memblock *packet = &blocks[handle];
  uint16_t start = packet->begin-1;
  uint16_t end = start + packet->size;

  // backup data at control-byte position
  uint8_t data = readByte(start);
  // write control-byte (if not 0 anyway)
  if (data)
    writeByte(start, 0);

  // TX start
  writeRegPair(ETXSTL, start);
  // Set the TXND pointer to correspond to the packet size given
  writeRegPair(ETXNDL, end);
  // send the contents of the transmit buffer onto the network
  writeOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
  // Reset the transmit logic problem. See Rev. B4 Silicon Errata point 12.
  if( (readReg(EIR) & EIR_TXERIF) )
    {
      writeOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRTS);
    }

  //restore data on control-byte position
  if (data)
    writeByte(start, data);
}

uint16_t
Enc28J60Network::setReadPtr(memhandle handle, memaddress position, uint16_t len)
{
  memblock *packet = handle == UIP_RECEIVEBUFFERHANDLE ? &receivePkt : &blocks[handle];
  memaddress start = handle == UIP_RECEIVEBUFFERHANDLE && packet->begin + position > RXSTOP_INIT ? packet->begin + position-RXSTOP_INIT+RXSTART_INIT : packet->begin + position;

  writeRegPair(ERDPTL, start);
  
  if (len > packet->size - position)
    len = packet->size - position;
  return len;
}

uint16_t
Enc28J60Network::readPacket(memhandle handle, memaddress position, uint8_t* buffer, uint16_t len)
{
  len = setReadPtr(handle, position, len);
  readBuffer(len, buffer);
  return len;
}

uint16_t
Enc28J60Network::writePacket(memhandle handle, memaddress position, uint8_t* buffer, uint16_t len)
{
  memblock *packet = &blocks[handle];
  uint16_t start = packet->begin + position;

  writeRegPair(EWRPTL, start);

  if (len > packet->size - position)
    len = packet->size - position;
  writeBuffer(len, buffer);
  return len;
}

uint8_t Enc28J60Network::readByte(uint16_t addr)
{
  writeRegPair(ERDPTL, addr);

  CSACTIVE;
  SPDR = ENC28J60_READ_BUF_MEM;
  waitspi();
  // read data
  SPDR = 0x00;
  waitspi();
  CSPASSIVE;
  return (SPDR);
}

void Enc28J60Network::writeByte(uint16_t addr, uint8_t data)
{
  writeRegPair(EWRPTL, addr);

  CSACTIVE;
  // issue write command
  SPDR = ENC28J60_WRITE_BUF_MEM;
  waitspi();
  // write data
  SPDR = data;
  waitspi();
  CSPASSIVE;
}

void
Enc28J60Network::copyPacket(memhandle dest_pkt, memaddress dest_pos, memhandle src_pkt, memaddress src_pos, uint16_t len)
{
  memblock *dest = &blocks[dest_pkt];
  memblock *src = src_pkt == UIP_RECEIVEBUFFERHANDLE ? &receivePkt : &blocks[src_pkt];
  memaddress start = src_pkt == UIP_RECEIVEBUFFERHANDLE && src->begin + src_pos > RXSTOP_INIT ? src->begin + src_pos-RXSTOP_INIT+RXSTART_INIT : src->begin + src_pos;
  enc28J60_mempool_block_move_callback(dest->begin+dest_pos,start,len);
  // Move the RX read pointer to the start of the next received packet
  // This frees the memory we just read out
  setERXRDPT();
}

void
enc28J60_mempool_block_move_callback(memaddress dest, memaddress src, memaddress len)
{
//void
//Enc28J60Network::memblock_mv_cb(uint16_t dest, uint16_t src, uint16_t len)
//{
  //as ENC28J60 DMA is unable to copy single bytes:
  if (len == 1)
    {
      Enc28J60Network::writeByte(dest,Enc28J60Network::readByte(src));
    }
  else
    {
      // calculate address of last byte
      len += src - 1;

      /*  1. Appropriately program the EDMAST, EDMAND
       and EDMADST register pairs. The EDMAST
       registers should point to the first byte to copy
       from, the EDMAND registers should point to the
       last byte to copy and the EDMADST registers
       should point to the first byte in the destination
       range. The destination range will always be
       linear, never wrapping at any values except from
       8191 to 0 (the 8-Kbyte memory boundary).
       Extreme care should be taken when
       programming the start and end pointers to
       prevent a never ending DMA operation which
       would overwrite the entire 8-Kbyte buffer.
       */
      Enc28J60Network::writeRegPair(EDMASTL, src);
      Enc28J60Network::writeRegPair(EDMADSTL, dest);

      if ((src <= RXSTOP_INIT)&& (len > RXSTOP_INIT))len -= (RXSTOP_INIT-RXSTART_INIT);
      Enc28J60Network::writeRegPair(EDMANDL, len);

      /*
       2. If an interrupt at the end of the copy process is
       desired, set EIE.DMAIE and EIE.INTIE and
       clear EIR.DMAIF.

       3. Verify that ECON1.CSUMEN is clear. */
      Enc28J60Network::writeOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_CSUMEN);

      /* 4. Start the DMA copy by setting ECON1.DMAST. */
      Enc28J60Network::writeOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_DMAST);

      // wait until runnig DMA is completed
      while (Enc28J60Network::readOp(ENC28J60_READ_CTRL_REG, ECON1) & ECON1_DMAST);
    }
}

void
Enc28J60Network::freePacket()
{
    setERXRDPT();
}

uint8_t
Enc28J60Network::readOp(uint8_t op, uint8_t address)
{
  CSACTIVE;
  // issue read command
  SPDR = op | (address & ADDR_MASK);
  waitspi();
  // read data
  SPDR = 0x00;
  waitspi();
  // do dummy read if needed (for mac and mii, see datasheet page 29)
  if(address & 0x80)
  {
    SPDR = 0x00;
    waitspi();
  }
  // release CS
  CSPASSIVE;
  return(SPDR);
}

void
Enc28J60Network::writeOp(uint8_t op, uint8_t address, uint8_t data)
{
  CSACTIVE;
  // issue write command
  SPDR = op | (address & ADDR_MASK);
  waitspi();
  // write data
  SPDR = data;
  waitspi();
  CSPASSIVE;
}

void
Enc28J60Network::readBuffer(uint16_t len, uint8_t* data)
{
  CSACTIVE;
  // issue read command
  SPDR = ENC28J60_READ_BUF_MEM;
  waitspi();
  while(len)
  {
    len--;
    // read data
    SPDR = 0x00;
    waitspi();
    *data = SPDR;
    data++;
  }
  //*data='\0';
  CSPASSIVE;
}

void
Enc28J60Network::writeBuffer(uint16_t len, uint8_t* data)
{
  CSACTIVE;
  // issue write command
  SPDR = ENC28J60_WRITE_BUF_MEM;
  waitspi();
  while(len)
  {
    len--;
    // write data
    SPDR = *data;
    data++;
    waitspi();
  }
  CSPASSIVE;
}

void
Enc28J60Network::setBank(uint8_t address)
{
  // set the bank (if needed)
  if((address & BANK_MASK) != bank)
  {
    // set the bank
    writeOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
    writeOp(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK)>>5);
    bank = (address & BANK_MASK);
  }
}

uint8_t
Enc28J60Network::readReg(uint8_t address)
{
  // set the bank
  setBank(address);
  // do the read
  return readOp(ENC28J60_READ_CTRL_REG, address);
}

void
Enc28J60Network::writeReg(uint8_t address, uint8_t data)
{
  // set the bank
  setBank(address);
  // do the write
  writeOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

void
Enc28J60Network::writeRegPair(uint8_t address, uint16_t data)
{
  // set the bank
  setBank(address);
  // do the write
  writeOp(ENC28J60_WRITE_CTRL_REG, address, (data&0xFF));
  writeOp(ENC28J60_WRITE_CTRL_REG, address+1, (data) >> 8);
}

void
Enc28J60Network::phyWrite(uint8_t address, uint16_t data)
{
  // set the PHY register address
  writeReg(MIREGADR, address);
  // write the PHY data
  writeRegPair(MIWRL, data);
  // wait until the PHY write completes
  while(readReg(MISTAT) & MISTAT_BUSY){
    _delay_us(15);
  }
}

uint16_t
Enc28J60Network::phyRead(uint8_t address)
{
  writeReg(MIREGADR,address);
  writeReg(MICMD, MICMD_MIIRD);
  // wait until the PHY read completes
  while(readReg(MISTAT) & MISTAT_BUSY){
    _delay_us(15);
  }  //and MIRDH
  writeReg(MICMD, 0);
  return (readReg(MIRDL) | readReg(MIRDH) << 8);
}

void
Enc28J60Network::clkout(uint8_t clk)
{
  //setup clkout: 2 is 12.5MHz:
  writeReg(ECOCON, clk & 0x7);
}

// read the revision of the chip:
uint8_t
Enc28J60Network::getrev(void)
{
  return(readReg(EREVID));
}

uint16_t
Enc28J60Network::chksum(uint16_t sum, memhandle handle, memaddress pos, uint16_t len)
{
  uint16_t t;
  len = setReadPtr(handle, pos, len)-1;
  CSACTIVE;
  // issue read command
  SPDR = ENC28J60_READ_BUF_MEM;
  waitspi();
  uint16_t i;
  for (i = 0; i < len; i+=2)
  {
    // read data
    SPDR = 0x00;
    waitspi();
    t = SPDR << 8;
    SPDR = 0x00;
    waitspi();
    t += SPDR;
    sum += t;
    if(sum < t) {
      sum++;            /* carry */
    }
  }
  if(i == len) {
    SPDR = 0x00;
    waitspi();
    t = (SPDR << 8) + 0;
    sum += t;
    if(sum < t) {
      sum++;            /* carry */
    }
  }
  CSPASSIVE;

  /* Return sum in host byte order. */
  return sum;
}

void
Enc28J60Network::powerOff()
{
  writeOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_RXEN);
  _delay_ms(50);
  writeOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_VRPS);
  _delay_ms(50);
  writeOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PWRSV);
}

void Enc28J60Network::powerOn()
{
  writeOp(ENC28J60_BIT_FIELD_CLR, ECON2, ECON2_PWRSV);
  _delay_ms(50);
  writeOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
  _delay_ms(50);
}

bool Enc28J60Network::linkStatus()
{
  return (phyRead(PHSTAT2) & 0x0400) > 0;
}

Enc28J60Network Enc28J60;

static const uint8_t POOLOFFSET = 1;

struct memblock MemoryPool::blocks[MEMPOOL_NUM_MEMBLOCKS+1];

void MemoryPool::init()
{
    memset(&blocks[0], 0, sizeof(blocks));
    blocks[POOLSTART].begin = MEMPOOL_STARTADDRESS;
    blocks[POOLSTART].size = 0; 
    blocks[POOLSTART].nextblock = NOBLOCK;
}


memhandle MemoryPool::allocBlock(memaddress size)
{
    memblock* best = NULL;
    memhandle cur = POOLSTART;
    memblock* block = &blocks[POOLSTART];
    memaddress bestsize = MEMPOOL_SIZE + 1;

    do
    {
      memhandle next = block->nextblock;
      memaddress freesize = (next == NOBLOCK ? blocks[POOLSTART].begin + MEMPOOL_SIZE : blocks[next].begin) - block->begin - block->size;
      if (freesize == size)
        {
          best = &blocks[cur];
          goto found;
        }
      if (freesize > size && freesize < bestsize)
        {
          bestsize = freesize;
          best = &blocks[cur];
        }
      if (next == NOBLOCK)
        {
          if (best)
            goto found;
          else
            goto collect;
        }
      block = &blocks[next];
      cur = next;
    }
  while (true);
  collect:
    {
      cur = POOLSTART;
      block = &blocks[POOLSTART];
      memhandle next;
      while ((next = block->nextblock) != NOBLOCK)
        {
          memaddress dest = block->begin + block->size;
          memblock* nextblock = &blocks[next];
          memaddress* src = &nextblock->begin;
          if (dest != *src)
            {
#ifdef MEMPOOL_MEMBLOCK_MV
              MEMPOOL_MEMBLOCK_MV(dest,*src,nextblock->size);
#endif
              *src = dest;
            }
          block = nextblock;
        }
      if (blocks[POOLSTART].begin + MEMPOOL_SIZE - block->begin - block->size >= size)
        best = block;
      else
        goto notfound;
    }
found:
    {
      block = &blocks[POOLOFFSET];
      for (cur = POOLOFFSET; cur < MEMPOOL_NUM_MEMBLOCKS + POOLOFFSET; cur++)
        {
          if (block->size)
            {
              block++;
              continue;
            }
          memaddress address = best->begin + best->size;
#ifdef MEMBLOCK_ALLOC
          MEMBLOCK_ALLOC(address,size);
#endif
          block->begin = address;
          block->size = size;
          block->nextblock = best->nextblock;
          best->nextblock = cur;
          return cur;
        }
    }

notfound:
    return NOBLOCK;
}

void MemoryPool::freeBlock(memhandle handle)
{
    if (handle == NOBLOCK)
        return;

    memblock *b = &blocks[POOLSTART];

    do
    {
      memhandle next = b->nextblock;
      if (next == handle)
        {
          memblock *f = &blocks[next];
#ifdef MEMBLOCK_FREE
          MEMBLOCK_FREE(f->begin,f->size);
#endif
          b->nextblock = f->nextblock;
          f->size = 0;
          f->nextblock = NOBLOCK;
          return;
        }
      if (next == NOBLOCK)
        return;
      b = &blocks[next];
    }
  while (true);
}

void MemoryPool::resizeBlock(memhandle handle, memaddress position)
{
    memblock * block = &blocks[handle];
    block->begin += position;
    block->size -= position;
}

void MemoryPool::resizeBlock(memhandle handle, memaddress position, memaddress size)
{
    memblock *block = &blocks[handle];
    block->begin += position;
    block->size = size;
}

memaddress MemoryPool::blockSize(memhandle handle)
{
    return blocks[handle].size;
}

