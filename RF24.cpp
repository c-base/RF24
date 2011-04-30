/*
 Copyright (C) 2011 James Coliz, Jr. <maniacbug@ymail.com>
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include <WProgram.h>
#include <SPI.h>
#include "RF24.h"
#include "nRF24L01.h"

#define SERIAL_DEBUG
#ifdef SERIAL_DEBUG
#define IF_SERIAL_DEBUG(x) (x)
#else
#define IF_SERIAL_DEBUG(x)
#endif

/******************************************************************/

void RF24::csn(int mode) 
{
  SPI.setDataMode(SPI_MODE0);
  digitalWrite(csn_pin,mode);
}

/******************************************************************/

void RF24::ce(int mode)
{
  digitalWrite(ce_pin,mode);
}

/******************************************************************/

uint8_t RF24::read_register(uint8_t reg, uint8_t* buf, uint8_t len) 
{
  uint8_t status;

  csn(LOW);
  status = SPI.transfer( R_REGISTER | ( REGISTER_MASK & reg ) );
  while ( len-- )
    *buf++ = SPI.transfer(0xff);

  csn(HIGH);

  return status;
}

/******************************************************************/

uint8_t RF24::read_register(uint8_t reg) 
{
  csn(LOW);
  SPI.transfer( R_REGISTER | ( REGISTER_MASK & reg ) );
  uint8_t result = SPI.transfer(0xff);

  csn(HIGH);
  return result;
}

/******************************************************************/

uint8_t RF24::write_register(uint8_t reg, const uint8_t* buf, uint8_t len)
{
  uint8_t status;

  csn(LOW);
  status = SPI.transfer( W_REGISTER | ( REGISTER_MASK & reg ) );
  while ( len-- )
    SPI.transfer(*buf++);

  csn(HIGH);

  return status;
}

/******************************************************************/

uint8_t RF24::write_register(uint8_t reg, uint8_t value)
{
  uint8_t status;

  IF_SERIAL_DEBUG(printf_P(PSTR("write_register(%02x,%02x)\n\r"),reg,value));

  csn(LOW);
  status = SPI.transfer( W_REGISTER | ( REGISTER_MASK & reg ) );
  SPI.transfer(value);
  csn(HIGH);

  return status;
}

/******************************************************************/

uint8_t RF24::write_payload(const void* buf, uint8_t len)
{
  uint8_t status;

  const uint8_t* current = (const uint8_t*)buf;

  csn(LOW);
  status = SPI.transfer( W_TX_PAYLOAD );
  uint8_t data_len = min(len,payload_size);
  uint8_t blank_len = payload_size - data_len;
  while ( data_len-- )
    SPI.transfer(*current++);
  while ( blank_len-- )
    SPI.transfer(0);

  csn(HIGH);

  return status;
}

/******************************************************************/

uint8_t RF24::read_payload(void* buf, uint8_t len) 
{
  uint8_t status;
  uint8_t* current = (uint8_t*)buf;

  csn(LOW);
  status = SPI.transfer( R_RX_PAYLOAD );
  uint8_t data_len = min(len,payload_size);
  uint8_t blank_len = payload_size - data_len;
  while ( data_len-- )
    *current++ = SPI.transfer(0xff);
  while ( blank_len-- )
    SPI.transfer(0xff);
  csn(HIGH);

  return status;
}

/******************************************************************/

uint8_t RF24::flush_rx(void)
{
  uint8_t status;

  csn(LOW);
  status = SPI.transfer( FLUSH_RX );  
  csn(HIGH);

  return status;
}

/******************************************************************/

uint8_t RF24::flush_tx(void)
{
  uint8_t status;

  csn(LOW);
  status = SPI.transfer( FLUSH_TX );  
  csn(HIGH);

  return status;
}

/******************************************************************/

uint8_t RF24::get_status(void) 
{
  uint8_t status;

  csn(LOW);
  status = SPI.transfer( NOP );
  csn(HIGH);

  return status;
}

/******************************************************************/

void RF24::print_status(uint8_t status)
{
  printf_P(PSTR("STATUS=%02x: RX_DR=%x TX_DS=%x MAX_RT=%x RX_P_NO=%x TX_FULL=%x\n\r"),
  status,
  (status & _BV(RX_DR))?1:0,
  (status & _BV(TX_DS))?1:0,
  (status & _BV(MAX_RT))?1:0,
  ((status >> RX_P_NO) & B111),
  (status & _BV(TX_FULL))?1:0
    );
}

/******************************************************************/

void RF24::print_observe_tx(uint8_t value) 
{
  printf_P(PSTR("OBSERVE_TX=%02x: POLS_CNT=%x ARC_CNT=%x\n\r"),
  value,
  (value >> PLOS_CNT) & B1111,
  (value >> ARC_CNT) & B1111
    );
}

/******************************************************************/

RF24::RF24(uint8_t _cepin, uint8_t _cspin): 
  ce_pin(_cepin), csn_pin(_cspin), payload_size(32), ack_packet_available(false)
{
}

/******************************************************************/

void RF24::setChannel(uint8_t channel)
{
  write_register(RF_CH,min(channel,127));  
}

/******************************************************************/

void RF24::setPayloadSize(uint8_t size)
{
  payload_size = min(size,32);
}

/******************************************************************/

uint8_t RF24::getPayloadSize(void) 
{
  return payload_size;
}

/******************************************************************/

void RF24::printDetails(void) 
{
  uint8_t buffer[5];
  uint8_t status = read_register(RX_ADDR_P0,buffer,5);
  print_status(status);
  printf_P(PSTR("RX_ADDR_P0 = 0x"));
  uint8_t *bufptr = buffer + 5;
  while( bufptr-- > buffer )
    printf_P(PSTR("%02x"),*bufptr);
  printf_P(PSTR("\n\r"));

  status = read_register(RX_ADDR_P1,buffer,5);
  printf_P(PSTR("RX_ADDR_P1 = 0x"));
  bufptr = buffer + 5;
  while( bufptr-- > buffer )
    printf_P(PSTR("%02x"),*bufptr);
  printf_P(PSTR("\n\r"));

  status = read_register(RX_ADDR_P2,buffer,1);
  printf_P(PSTR("RX_ADDR_P2 = 0x%02x"),*buffer);
  printf_P(PSTR("\n\r"));

  status = read_register(RX_ADDR_P3,buffer,1);
  printf_P(PSTR("RX_ADDR_P3 = 0x%02x"),*buffer);
  printf_P(PSTR("\n\r"));

  status = read_register(TX_ADDR,buffer,5);
  printf_P(PSTR("TX_ADDR = 0x"));
  bufptr = buffer + 5;
  while( bufptr-- > buffer )
    printf_P(PSTR("%02x"),*bufptr);
  printf_P(PSTR("\n\r"));
  
  status = read_register(RX_PW_P0,buffer,1);
  printf_P(PSTR("RX_PW_P0 = 0x%02x\n\r"),*buffer);

  status = read_register(RX_PW_P1,buffer,1);
  printf_P(PSTR("RX_PW_P1 = 0x%02x\n\r"),*buffer);

  read_register(EN_AA,buffer,1);
  printf_P(PSTR("EN_AA = %02x\n\r"),*buffer);

  read_register(EN_RXADDR,buffer,1);
  printf_P(PSTR("EN_RXADDR = %02x\n\r"),*buffer);

  read_register(RF_CH,buffer,1);
  printf_P(PSTR("RF_CH = %02x\n\r"),*buffer);  
}

/******************************************************************/

void RF24::begin(void)
{
  pinMode(ce_pin,OUTPUT);
  pinMode(csn_pin,OUTPUT);

  ce(LOW);
  csn(HIGH);

  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV8);

  // Set generous timeouts, to make testing a little easier
  write_register(SETUP_RETR,(B1111 << ARD) | (B1111 << ARC));

  // Reset current status
  write_register(STATUS,_BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );

  // Flush buffers
  flush_rx();
  flush_tx();    

  // Set up default configuration.  Callers can always change it later.
  setChannel(1);
}

/******************************************************************/

void RF24::startListening(void)
{
  write_register(CONFIG, _BV(EN_CRC) | _BV(PWR_UP) | _BV(PRIM_RX));
  write_register(STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );

  // Flush buffers
  flush_rx();

  // Go!  
  ce(HIGH);

  // wait for the radio to come up (130us actually only needed)
  delay(1);
}

/******************************************************************/

void RF24::stopListening(void)
{
  ce(LOW);
}

/******************************************************************/

boolean RF24::write( const void* buf, uint8_t len )
{
  boolean result = false;

  // Transmitter power-up
  write_register(CONFIG, _BV(EN_CRC) | _BV(PWR_UP));
  delay(2);

  // Send the payload
  write_payload( buf, len );

  // Allons!
  ce(HIGH);

  // IN the end, the send should be blocking.  It comes back in 60ms worst case, or much faster
  // if I tighted up the retry logic.  (Default settings will be 750us.
  // Monitor the send
  uint8_t observe_tx;
  uint8_t status;
  uint8_t retries = 255;
  do
  {
    status = read_register(OBSERVE_TX,&observe_tx,1);
    IF_SERIAL_DEBUG(Serial.print(status,HEX));
    IF_SERIAL_DEBUG(Serial.print(observe_tx,HEX));
    if ( ! retries-- )
    {
      IF_SERIAL_DEBUG(printf("ABORTED: too many retries\n\r"));
      break;
    }
  }
  while( ! ( status & ( _BV(TX_DS) | _BV(MAX_RT) ) ) );

  if ( status & _BV(TX_DS) )
    result = true;
  
  IF_SERIAL_DEBUG(Serial.print(result?"...OK.":"...Failed"));
  
  ack_packet_available = ( status & _BV(RX_DR) );
  uint8_t pl_len = 0; 
  if ( ack_packet_available )
  {
    write_register(STATUS,_BV(RX_DR) );
    csn(LOW);
    SPI.transfer( R_RX_PL_WID );
    pl_len = SPI.transfer(0xff);
    csn(HIGH);
    IF_SERIAL_DEBUG(Serial.print("[AckPacket]/"));
    IF_SERIAL_DEBUG(Serial.println(pl_len,DEC));
  }

  // Yay, we are done.
  ce(LOW);

  // Power down
  write_register(CONFIG, _BV(EN_CRC) );

  // Reset current status
  write_register(STATUS,_BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );

  // Flush buffers
  flush_tx();  

  return result;
}
/******************************************************************/

boolean RF24::available(void) 
{
  return available(NULL);
}

/******************************************************************/

boolean RF24::available(uint8_t* pipe_num) 
{
  uint8_t status = get_status();
  IF_SERIAL_DEBUG(print_status(status));
  boolean result = ( status & _BV(RX_DR) );

  if (result)
  {
    // If the caller wants the pipe number, include that
    if ( pipe_num )
      *pipe_num = ( status >> RX_P_NO ) & B111;

    // Clear the status bit

    // ??? Should this REALLY be cleared now?  Or wait until we
    // actually READ the payload?

    write_register(STATUS,_BV(RX_DR) );

    // Handle ack payload receipt
    if ( status & _BV(TX_DS) )
    {
      write_register(STATUS,_BV(TX_DS));
    }
  }

  return result;
}

/******************************************************************/

boolean RF24::read( void* buf, uint8_t len ) 
{
  // was this the last of the data available?
  boolean result = false;

  // Fetch the payload
  read_payload( buf, len );

  uint8_t fifo_status;
  read_register(FIFO_STATUS,&fifo_status,1);
  if ( fifo_status & _BV(RX_EMPTY) )
    result = true;

  return result;
}

/******************************************************************/

void RF24::openWritingPipe(uint64_t value)
{
  // Note that AVR 8-bit uC's store this LSB first, and the NRF24L01
  // expects it LSB first too, so we're good.  
  
  write_register(RX_ADDR_P0, reinterpret_cast<uint8_t*>(&value), 5);
  write_register(TX_ADDR, reinterpret_cast<uint8_t*>(&value), 5);  
  write_register(RX_PW_P0,min(payload_size,32));
}

/******************************************************************/

void RF24::openReadingPipe(uint8_t child, uint64_t value)
{
  const uint8_t child_pipe[] = { 
    RX_ADDR_P1, RX_ADDR_P2, RX_ADDR_P3, RX_ADDR_P4, RX_ADDR_P5   };
  const uint8_t child_payload_size[] = { 
    RX_PW_P1, RX_PW_P2, RX_PW_P3, RX_PW_P4, RX_PW_P5   };
  const uint8_t child_pipe_enable[] = { 
    ERX_P1, ERX_P2, ERX_P3, ERX_P4, ERX_P5 };

  if (--child < 5)
  {
    // For pipes 2-5, only write the LSB
    if ( !child )
      write_register(child_pipe[child], reinterpret_cast<uint8_t*>(&value), 5);    
    else  
      write_register(child_pipe[child], reinterpret_cast<uint8_t*>(&value), 1);    
    
    write_register(child_payload_size[child],payload_size);  

    // Note this is kind of an inefficient way to set up these enable bits, bit I thought it made
    // the calling code more simple
    uint8_t en_rx;
    read_register(EN_RXADDR,&en_rx,1);
    en_rx |= _BV(child_pipe_enable[child]);
    write_register(EN_RXADDR,en_rx);
  }
}

/******************************************************************/
  
void RF24::toggle_features(void)
{
  csn(LOW);
  SPI.transfer( ACTIVATE );
  SPI.transfer( 0x73 );
  csn(HIGH);
}

/******************************************************************/

void RF24::enableAckPayload(void)
{
  //
  // enable ack payload and dynamic payload features
  //

  write_register(FEATURE,read_register(FEATURE) | _BV(EN_ACK_PAY) | _BV(EN_DPL) );

  // If it didn't work, the features are not enabled
  if ( ! read_register(FEATURE) )
  {
    // So enable them and try again
    toggle_features();
    write_register(FEATURE,read_register(FEATURE) | _BV(EN_ACK_PAY) | _BV(EN_DPL) );
  }

  IF_SERIAL_DEBUG(printf("FEATURE=%i\n\r",read_register(FEATURE)));

  //
  // Enable dynamic payload on pipe 0
  //

  write_register(DYNPD,read_register(DYNPD) | _BV(DPL_P1) | _BV(DPL_P0));
}

/******************************************************************/

void RF24::writeAckPayload(uint8_t pipe, const void* buf, uint8_t len)
{
  const uint8_t* current = (const uint8_t*)buf;

  csn(LOW);
  SPI.transfer( W_ACK_PAYLOAD | ( pipe & B111 ) );
  uint8_t data_len = min(len,32);
  while ( data_len-- )
    SPI.transfer(*current++);

  csn(HIGH);
}

/******************************************************************/

boolean RF24::isAckPayloadAvailable(void)
{
  boolean result = ack_packet_available;
  ack_packet_available = false;
  return result;
}

// vim:ai:cin:sts=2 sw=2 ft=cpp

