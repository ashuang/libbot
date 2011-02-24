#ifndef __bot_serial_h__
#define __bot_serial_h__

/**
 * @defgroup BotCoreSerial Serial ports
 * @brief Reading and writing from serial ports
 * @ingroup BotCoreIO
 * @include: bot_core/bot_core.h
 *
 * TODO
 *
 * Linking: `pkg-config --libs bot2-core`
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Creates a basic fd, setting baud to 9600, raw data i/o (no flow
    control, no fancy character handling. Configures it for blocking
    reads.  8 data bits, 1 stop bit, no parity.

    Returns the fd, -1 on error
**/
int bot_serial_open(const char *port, int baud, int blocking);

/** Set the baud rate, where the baudrate is just the integer value
    desired.

    Returns non-zero on error.
**/
int bot_serial_setbaud(int fd, int baudrate);

/** Enable cts/rts flow control.
    Returns non-zero on error.
**/
int bot_serial_enablectsrts(int fd);
/** Enable xon/xoff flow control.
    Returns non-zero on error.
**/
int bot_serial_enablexon(int fd);

/** Set the port to 8 data bits, 2 stop bits, no parity.
    Returns non-zero on error.
 **/
int bot_serial_set_N82 (int fd);

int bot_serial_close(int fd);


/**
 *
 *
 */
typedef struct _BotSerialCircBuf BotSerialCircBuf;

/*
 * Create buffer, and allocate space for size bytes
 */
BotSerialCircBuf * bot_serial_circbuf_create(int size);

/*
 * Destroy it
 */
void bot_serial_circbuf_destroy(BotSerialCircBuf * cbuf);

/*
 * Copy numBytes from the head of the buffer, and move read pointers
 */
int bot_serial_circbuf_read(BotSerialCircBuf * cbuf, int numBytes, char * buf);

/*
 * Copy numBytes from buf to end of buffer
 */
int bot_serial_circbuf_write(BotSerialCircBuf * cbuf, int numBytes, char * buf);

/*
 * Copy numBytes from the head of the buffer, but DON'T move read pointers
 */
int bot_serial_circbuf_peek(BotSerialCircBuf * cbuf, int numBytes, char * buf); //read numBytes from start of buffer, but don't move readPtr
/*
 * blow everything away
 */
int bot_serial_circbuf_flush(BotSerialCircBuf * cbuf); //move pointers to "empty" the read buffer
/*
 * Get the amount of data currently stored in the buffer (not the allocated size)
 */
int bot_serial_circbuf_available(BotSerialCircBuf * cbuf);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
