/** Simplified serial utilities. As opposed to my earlier
    serial class, we let you use the fd (or FILE*) directly.

    eolson@mit.edu, 2004

    ---------
    added the circbuf circulur buffer utilities
    abachrac@mit.edu, 2011
**/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#ifdef __linux__
#include <linux/serial.h>
#define SUPPORT_HISPEED 1
#endif

#include <stdio.h>

#include "serial.h"

#ifndef MIN
#define MIN(a,b)((a < b) ? a : b)
#endif

static int bot_serial_translate_baud(int inrate);


/** Creates a basic fd, setting baud to 9600, raw data i/o (no flow
    control, no fancy character handling. Configures it for blocking
    reads.

    Returns the fd, -1 on error
**/
int bot_serial_open(const char *port, int baud, int blocking)
{
	struct termios opts;

	int flags = O_RDWR | O_NOCTTY;
	if (!blocking)
		flags |= O_NONBLOCK;

	int fd=open(port, flags, 0);
	if (fd==-1)
		return -1;

	if (tcgetattr(fd, &opts))
	{
		printf("*** %i\n",fd);
		perror("tcgetattr");
		return -1;
	}

 
	cfsetispeed(&opts, bot_serial_translate_baud(baud));
	cfsetospeed(&opts, bot_serial_translate_baud(baud));

	cfmakeraw(&opts);
        
        // set one stop bit
        opts.c_cflag &= ~CSTOPB;
/*
	opts.c_cflag |= (CLOCAL | CREAD);
	opts.c_cflag &= ~CRTSCTS;
	opts.c_cflag &= ~PARENB;
	opts.c_cflag &= ~CSTOPB;
	opts.c_cflag &= ~CSIZE;
	opts.c_cflag |= CS8;
  
	opts.c_lflag = 0;

	opts.c_iflag &= ~(IXON | IXOFF);
	opts.c_iflag |= IXANY;
	opts.c_iflag |= IGNPAR;
	//  opts.c_iflag |= IXON | IXOFF | IXANY;

	//  opts.c_iflag &= ~IGNPAR;
	opts.c_iflag &= ~(INLCR | IGNCR | ICRNL | IUCLC);

	opts.c_cc[VTIME]=0;   // synchronous I/O
	opts.c_cc[VMIN]=1;

	opts.c_oflag &= ~OPOST;
*/


	if (tcsetattr(fd,TCSANOW,&opts))
	{
		perror("tcsetattr");
		return -1;
	}

	tcflush(fd, TCIOFLUSH);
	return fd;
}

int bot_serial_set_N82 (int fd)
{
    struct termios opts;

    if (tcgetattr(fd, &opts))
    {
        perror("tcgetattr");
        return -1;
    }

    opts.c_cflag &= ~CSIZE;
    opts.c_cflag |= CS8;
    opts.c_cflag |= CSTOPB;

    if (tcsetattr(fd,TCSANOW,&opts))
    {
        perror("tcsetattr");
        return -1;
    }

    return 0;
}

/** Enable cts/rts flow control. 
    Returns non-zero on error. 
**/
int bot_serial_enablectsrts(int fd)
{
	struct termios opts;

	if (tcgetattr(fd, &opts))
	{
		perror("tcgetattr");
		return -1;
	}

	opts.c_cflag |= CRTSCTS;

	if (tcsetattr(fd,TCSANOW,&opts))
	{
		perror("tcsetattr");
		return -1;
	}

	return 0;
}


/** Enable xon/xoff flow control. 
    Returns non-zero on error. 
**/
int bot_serial_enablexon(int fd)
{
	struct termios opts;

	if (tcgetattr(fd, &opts))
	{
		perror("tcgetattr");
		return -1;
	}

	opts.c_iflag |= (IXON | IXOFF);

	if (tcsetattr(fd,TCSANOW,&opts))
	{
		perror("tcsetattr");
		return -1;
	}

	return 0;
}



/** Set the baud rate, where the baudrate is just the integer value
    desired.

    Returns non-zero on error.
**/
int bot_serial_setbaud(int fd, int baudrate)
{
	struct termios tios;
#ifdef SUPPORT_HISPEED
	struct serial_struct ser;
#endif

	int baudratecode=bot_serial_translate_baud(baudrate);

	if (baudratecode>0)
	{
		tcgetattr(fd, &tios);
		cfsetispeed(&tios, baudratecode);
		cfsetospeed(&tios, baudratecode);
		tcflush(fd, TCIFLUSH);
		tcsetattr(fd, TCSANOW, &tios);

#ifdef SUPPORT_HISPEED
		ioctl(fd, TIOCGSERIAL, &ser);

		ser.flags=(ser.flags&(~ASYNC_SPD_MASK));
		ser.custom_divisor=0;

		ioctl(fd, TIOCSSERIAL, &ser);
#endif
	}
	else
	{
#ifdef SUPPORT_HISPEED
		//      printf("Setting custom divisor\n");

		if (tcgetattr(fd, &tios))
			perror("tcgetattr");

		cfsetispeed(&tios, B38400);
		cfsetospeed(&tios, B38400);
		tcflush(fd, TCIFLUSH);

		if (tcsetattr(fd, TCSANOW, &tios))
			perror("tcsetattr");

		if (ioctl(fd, TIOCGSERIAL, &ser))
			perror("ioctl TIOCGSERIAL");
      
		ser.flags=(ser.flags&(~ASYNC_SPD_MASK)) | ASYNC_SPD_CUST;
		ser.custom_divisor=48;
		ser.custom_divisor=ser.baud_base/baudrate;
		ser.reserved_char[0]=0; // what the hell does this do?

		//      printf("baud_base %i\ndivisor %i\n", ser.baud_base,ser.custom_divisor);

		if (ioctl(fd, TIOCSSERIAL, &ser))
			perror("ioctl TIOCSSERIAL");

#endif

	}
  
	tcflush(fd, TCIFLUSH);
  
	return 0;
}


// private function
int bot_serial_translate_baud(int inrate)
{
	switch(inrate)
	{
	case 0:
		return B0;
	case 300:
		return B300;
	case 1200:
		return B1200;
	case 2400:
		return B2400;
	case 4800:
		return B4800;
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
#ifdef SUPPORT_HISPEED
	case 460800:
		return B460800;
#endif
	default:
		return -1; // do custom divisor
	}
}

int bot_serial_close(int fd)
{
	return close(fd);
}




///////////////////////////////////////////////////////////////////////////////


struct _BotSerialCircBuf{
        char * buf;
        int readOffset;
        int writeOffset;
        int numBytes;
        int maxSize;
};

BotSerialCircBuf * bot_serial_circbuf_create(int size)
{
  //create buffer, and allocate space for size bytes
  BotSerialCircBuf * cbuf = (BotSerialCircBuf *) calloc(1,sizeof(BotSerialCircBuf));
  cbuf->buf = (char *) malloc(size * sizeof(char));
  cbuf->readOffset = cbuf->writeOffset = cbuf->numBytes = 0;
  cbuf->maxSize = size;
  return cbuf;
}

void bot_serial_circbuf_destroy(BotSerialCircBuf * cbuf)
{
  //destroy
  if (cbuf->buf!=NULL)
    free(cbuf->buf);
  free(cbuf);
}

int bot_serial_circbuf_read(BotSerialCircBuf * cbuf, int numBytes, char * buf)
{
  //read numBytes
  int bytes_read = bot_serial_circbuf_peek(cbuf, numBytes, buf);

  if (bytes_read > 0){
    //move readPtr
    cbuf->numBytes -= bytes_read;
    cbuf->readOffset = (cbuf->readOffset + bytes_read) % cbuf->maxSize;
  }

  return bytes_read;

}

int bot_serial_circbuf_write(BotSerialCircBuf * cbuf, int numBytes, char * buf)
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

int bot_serial_circbuf_peek(BotSerialCircBuf * cbuf, int numBytes, char * buf)
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

int bot_serial_circbuf_flush(BotSerialCircBuf * cbuf)
{
  //move pointers to "empty" the read buffer
  cbuf->readOffset = cbuf->writeOffset = cbuf->numBytes = 0;
  return 0;

}

int bot_serial_circbuf_available(BotSerialCircBuf * cbuf) {
        return cbuf->numBytes;
}

#if 0
void bot_serial_circbuf_unit_test()
{
  char * testString = "iuerrlfkladbytes_writtenbytes_writte";
  char comp[1000];
  char * comp_p = comp;
  BotSerialCircBuf cbuf;
  bot_serial_circbuf_create(&cbuf, 13);

  int numWritten = 0;
  int writeAmount = 0;
  int numRead = 0;
  int readAmount = 0;

  writeAmount = 4;
  bot_serial_circbuf_write(&cbuf, writeAmount, testString + numWritten);
  numWritten += writeAmount;

  writeAmount = 6;
  bot_serial_circbuf_write(&cbuf, writeAmount, testString + numWritten);
  numWritten += writeAmount;

  readAmount = 9;
  bot_serial_circbuf_read(&cbuf, readAmount, comp_p + numRead);
  numRead += readAmount;

  writeAmount = 11;
  bot_serial_circbuf_write(&cbuf, writeAmount, testString + numWritten);
  numWritten += writeAmount;

  readAmount = 12;
  bot_serial_circbuf_read(&cbuf, readAmount, comp_p + numRead);
  numRead += readAmount;

  printf("at end, there are %d bytes left, should be %d \n", cbuf.numBytes, numWritten - numRead);

  if (strncmp(testString, comp, numRead) == 0)
    printf("WOOOHOO! the strings match :-)\n");
  else
    printf("BOOOO Somethings wrong");

  bot_serial_circbuf_destroy(&cbuf);
}
#endif
