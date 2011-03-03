#ifndef __bot_ringbuf_h__
#define __bot_ringbuf_h__

/**
 * @defgroup BotCoreRingbuf A simple ring buffer
 * @brief A fixed capacity ring buffer
 * @ingroup BotCoreDataStructures
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

/**
 *
 *
 */
typedef struct _BotRingBuf BotRingBuf;

/*
 * Create buffer, and allocate space for size bytes
 */
BotRingBuf * bot_ringbuf_create(int size);

/*
 * Destroy it
 */
void bot_ringbuf_destroy(BotRingBuf * cbuf);

/*
 * Copy numBytes from the head of the buffer, and move read pointers
 */
int bot_ringbuf_read(BotRingBuf * cbuf, int numBytes, char * buf);

/*
 * Copy numBytes from buf to end of buffer
 */
int bot_ringbuf_write(BotRingBuf * cbuf, int numBytes, char * buf);

/*
 * Copy numBytes from the head of the buffer, but DON'T move read pointers
 */
int bot_ringbuf_peek(BotRingBuf * cbuf, int numBytes, char * buf); //read numBytes from start of buffer, but don't move readPtr
/*
 * blow everything away
 */
int bot_ringbuf_flush(BotRingBuf * cbuf); //move pointers to "empty" the read buffer
/*
 * Get the amount of data currently stored in the buffer (not the allocated size)
 */
int bot_ringbuf_available(BotRingBuf * cbuf);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
