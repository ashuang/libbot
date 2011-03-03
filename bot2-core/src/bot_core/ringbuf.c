#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ringbuf.h"

#ifndef MIN
#define MIN(a,b)((a < b) ? a : b)
#endif

struct _BotRingBuf{
        char * buf;
        int readOffset;
        int writeOffset;
        int numBytes;
        int maxSize;
};

BotRingBuf * bot_ringbuf_create(int size)
{
  //create buffer, and allocate space for size bytes
  BotRingBuf * cbuf = (BotRingBuf *) calloc(1,sizeof(BotRingBuf));
  cbuf->buf = (char *) malloc(size * sizeof(char));
  cbuf->readOffset = cbuf->writeOffset = cbuf->numBytes = 0;
  cbuf->maxSize = size;
  return cbuf;
}

void bot_ringbuf_destroy(BotRingBuf * cbuf)
{
  //destroy
  if (cbuf->buf!=NULL)
    free(cbuf->buf);
  free(cbuf);
}

int bot_ringbuf_read(BotRingBuf * cbuf, int numBytes, char * buf)
{
  //read numBytes
  int bytes_read = bot_ringbuf_peek(cbuf, numBytes, buf);

  if (bytes_read > 0){
    //move readPtr
    cbuf->numBytes -= bytes_read;
    cbuf->readOffset = (cbuf->readOffset + bytes_read) % cbuf->maxSize;
  }

  return bytes_read;

}

int bot_ringbuf_write(BotRingBuf * cbuf, int numBytes, char * buf)
{

  //check if there is enough space... maybe this should just wrap around??
  if (numBytes + cbuf->numBytes > cbuf->maxSize) {
    fprintf(stderr, "CIRC_BUF ERROR: not enough space in circular buffer,Discarding data!\n");
    numBytes = cbuf->maxSize - cbuf->numBytes;

  }
  //write to wrap around point.
  int bytes_written = MIN(cbuf->maxSize - cbuf->writeOffset, numBytes);
  memcpy(cbuf->buf + cbuf->writeOffset, buf, bytes_written * sizeof(char));
  numBytes -= bytes_written;

  //write the rest from start of buffer
  if (numBytes > 0) {
    memcpy(cbuf->buf, buf + bytes_written, numBytes * sizeof(char));
    bytes_written += numBytes;
  }

  //move writePtr
  cbuf->numBytes += bytes_written;
  cbuf->writeOffset = (cbuf->writeOffset + bytes_written) % cbuf->maxSize;

  return bytes_written;

}

int bot_ringbuf_peek(BotRingBuf * cbuf, int numBytes, char * buf)
{
  //read numBytes from start of buffer, but don't move readPtr
  if (numBytes > cbuf->numBytes || numBytes > cbuf->maxSize) {
    fprintf(stderr, "CIRC_BUF ERROR: Can't read %d bytes from the circular buffer, only %d available! \n",
        numBytes,cbuf->numBytes);
    return -1;
  }
  //read up to wrap around point
  int bytes_read = MIN(cbuf->maxSize - cbuf->readOffset, numBytes);
  memcpy(buf, cbuf->buf + cbuf->readOffset, bytes_read * sizeof(char));
  numBytes -= bytes_read;

  //read again from beginning if there are bytes left
  if (numBytes > 0) {
    memcpy(buf + bytes_read, cbuf->buf, numBytes * sizeof(char));
    bytes_read += numBytes;
  }
  return bytes_read;
}

int bot_ringbuf_flush(BotRingBuf * cbuf)
{
  //move pointers to "empty" the read buffer
  cbuf->readOffset = cbuf->writeOffset = cbuf->numBytes = 0;
  return 0;

}

int bot_ringbuf_available(BotRingBuf * cbuf) {
        return cbuf->numBytes;
}

#if 0
void bot_ringbuf_unit_test()
{
  char * testString = "iuerrlfkladbytes_writtenbytes_writte";
  char comp[1000];
  char * comp_p = comp;
  BotRingBuf cbuf;
  bot_ringbuf_create(&cbuf, 13);

  int numWritten = 0;
  int writeAmount = 0;
  int numRead = 0;
  int readAmount = 0;

  writeAmount = 4;
  bot_ringbuf_write(&cbuf, writeAmount, testString + numWritten);
  numWritten += writeAmount;

  writeAmount = 6;
  bot_ringbuf_write(&cbuf, writeAmount, testString + numWritten);
  numWritten += writeAmount;

  readAmount = 9;
  bot_ringbuf_read(&cbuf, readAmount, comp_p + numRead);
  numRead += readAmount;

  writeAmount = 11;
  bot_ringbuf_write(&cbuf, writeAmount, testString + numWritten);
  numWritten += writeAmount;

  readAmount = 12;
  bot_ringbuf_read(&cbuf, readAmount, comp_p + numRead);
  numRead += readAmount;

  printf("at end, there are %d bytes left, should be %d \n", cbuf.numBytes, numWritten - numRead);

  if (strncmp(testString, comp, numRead) == 0)
    printf("WOOOHOO! the strings match :-)\n");
  else
    printf("BOOOO Somethings wrong");

  bot_ringbuf_destroy(&cbuf);
}
#endif
