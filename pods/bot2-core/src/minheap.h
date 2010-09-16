#ifndef __bot_minheap_h__
#define __bot_minheap_h__

#include <glib.h>

/**
 * SECTION:minheap
 * @title: Minheap
 * @short_description: Heap data structure
 * @include: bot2-core/bot2-core.h
 *
 * Linking: -lbot2-core
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _BotMinheap BotMinheap;
typedef struct _BotMinheapNode BotMinheapNode;


BotMinheap *bot_minheap_new(void);

BotMinheap *bot_minheap_sized_new(int capacity);

void bot_minheap_free(BotMinheap *mh);

BotMinheapNode *bot_minheap_add (BotMinheap *mh, void *data, double score);

void bot_minheap_decrease_score (BotMinheap *mh, BotMinheapNode *node,
        double score);

void *bot_minheap_remove_min (BotMinheap *mh, double *score);

int bot_minheap_size (BotMinheap *mh);

gboolean bot_minheap_is_empty (BotMinheap *mh);

#ifdef __cplusplus
}
#endif

#endif
