#include <stdio.h>

#include <glib.h>

#include <lcm/lcm.h>
#include "bot_frames.h"
#include <GL/gl.h>
#include <bot_param/param_util.h>

//TOOD:Should figure out a way to add new frames at runtime to enable LCMGLish

typedef struct {

  char * frame_name;
  char * relative_to;
  char * update_channel;
  bot_core_isometry_t_subscription_t * subscription;
  BotCTransLink * ctrans_link;
  int was_updated;
} frame_handle_t;

typedef struct {
  bot_frames_link_update_handler_t * callback_func;
  void * user;
} update_handler_t;

static void frame_handle_destroy(lcm_t * lcm, frame_handle_t * fh)
{
  if (fh->frame_name != NULL)
    free(fh->frame_name);
  if (fh->relative_to != NULL)
    free(fh->relative_to);
  if (fh->update_channel != NULL)
    free(fh->update_channel);
  if (fh->subscription != NULL)
    bot_core_isometry_t_unsubscribe(lcm, fh->subscription);
  free(fh);
}

struct _BotFrames {
  BotCTrans * ctrans;
  lcm_t *lcm;
  BotParam *bot_param;

  GMutex * mutex;

  char * root_name;
  GHashTable* frame_handles_by_name;
  GHashTable* frame_handles_by_channel;

  GList * update_callbacks;

};

static void on_frame_update(const lcm_recv_buf_t *rbuf, const char *channel, const bot_core_isometry_t *msg,
    void *user_data)
{
  BotFrames * bot_frames = (BotFrames *) user_data;
  g_mutex_lock(bot_frames->mutex);
  BotTrans link_transf;
  bot_trans_set_from_quat_trans(&link_transf, msg->quat, msg->trans);

  frame_handle_t * frame_handle = (frame_handle_t *) g_hash_table_lookup(bot_frames->frame_handles_by_channel, channel);
  assert(frame_handle != NULL);
  frame_handle->was_updated = 1;
  bot_ctrans_link_update(frame_handle->ctrans_link, &link_transf, msg->utime);
  g_mutex_unlock(bot_frames->mutex);

  GList * p = bot_frames->update_callbacks;
  for (p; p != NULL; p = g_list_next(p)) {
    update_handler_t * uh = (update_handler_t *) p->data;
    uh->callback_func(bot_frames, frame_handle->frame_name, frame_handle->relative_to, uh->user);
  }

}

BotFrames *
bot_frames_new(lcm_t *lcm, BotParam *bot_param)
{
  BotFrames *self = g_slice_new0(BotFrames);

  self->lcm = lcm;
  self->bot_param = bot_param;
  self->mutex = g_mutex_new();

  g_mutex_lock(self->mutex);
  // setup the coordinate frame graph
  self->ctrans = bot_ctrans_new();

  //allocate frame handles
  self->frame_handles_by_name = g_hash_table_new(g_str_hash, g_str_equal);
  self->frame_handles_by_channel = g_hash_table_new(g_str_hash, g_str_equal);

  //create the callback lists
  self->update_callbacks = NULL;

  int num_frames = bot_param_get_num_subkeys(self->bot_param, "coordinate_frames");
  if (num_frames <= 0) {
    fprintf(stderr, "BotFrames Error: param file does not contain a 'coordinate_frames' block\n");
    bot_frames_destroy(self);
    return NULL;
  }

  int ret = bot_param_get_str(self->bot_param, "coordinate_frames.root_frame", &self->root_name);
  if (ret < 0) {
    fprintf(stderr, "BotFrames Error: root_frame not defined!\n");
    goto fail;
  }
  bot_ctrans_add_frame(self->ctrans, self->root_name);
  frame_handle_t * root_handle = (frame_handle_t *) calloc(1, sizeof(frame_handle_t));
  root_handle->ctrans_link = NULL;
  root_handle->frame_name = self->root_name;
  root_handle->relative_to = NULL;
  root_handle->was_updated = 0;
  root_handle->update_channel = NULL;
  root_handle->subscription = NULL;
  g_hash_table_insert(self->frame_handles_by_name, (gpointer) self->root_name, (gpointer) root_handle);

  char ** frame_names = bot_param_get_subkeys(self->bot_param, "coordinate_frames");

  for (int i = 0; i < num_frames; i++) {
    char * frame_name = strdup(frame_names[i]);
    //add this frame to ctrans
    bot_ctrans_add_frame(self->ctrans, frame_name);

    char param_key[2048];
    sprintf(param_key, "coordinate_frames.%s", frame_name);
    int num_sub_keys = bot_param_get_num_subkeys(self->bot_param, param_key);
    if (num_sub_keys == 0) {
      continue; // probably the root_frame definition
    }
    //setup the link parameters if this isn't a root frame

    //get which frame this is relative to
    sprintf(param_key, "coordinate_frames.%s.relative_to", frame_name);
    char * relative_to;
    int ret = bot_param_get_str(self->bot_param, param_key, &relative_to);
    if (ret < 0) {
      fprintf(stderr, "BotFrames Error: frame %s does not have a 'relative_to' field block\n", frame_name);
      goto fail;
    }

    //get the history size
    sprintf(param_key, "coordinate_frames.%s.history", frame_name);
    int history;
    ret = bot_param_get_int(self->bot_param, param_key, &history);
    if (ret < 0) {
      history = 0;//assume it's a static transformation
    }

    //get the initial transform
    sprintf(param_key, "coordinate_frames.%s.initial_transform", frame_name);
    if (bot_param_get_num_subkeys(self->bot_param, param_key) != 2) {
      fprintf(stderr,
          "BotFrames Error: frame %s does not have the right number of fields in the 'initial_transform' block\n",
          frame_name);
      goto fail;
    }
    BotTrans init_trans;
    ret = bot_param_get_trans(self->bot_param, param_key, &init_trans);
    if (ret < 0) {
      fprintf(stderr, "BotFrames Error: could not get 'initial_transform' for frame %s\n", frame_name);
      goto fail;
    }

    //create and initialize the link
    BotCTransLink *link = bot_ctrans_link_frames(self->ctrans, frame_name, relative_to, history + 1);
    bot_ctrans_link_update(link, &init_trans, 0);

    //    BotTrans t = init_trans;
    //    fprintf(stderr, "%s->%s= (%f,%f,%f) - (%f,%f,%f,%f)\n", frame_name, relative_to, t.trans_vec[0], t.trans_vec[1],
    //        t.trans_vec[2], t.rot_quat[0], t.rot_quat[1], t.rot_quat[2], t.rot_quat[3]);

    //add the frame to the hash table
    frame_handle_t * frame_handle = (frame_handle_t *) g_hash_table_lookup(self->frame_handles_by_name, frame_name);
    if (frame_handle != NULL) {
      fprintf(stderr, "BotFrames Error: frame %s duplicated\n", frame_name);
      goto fail;
    }
    //first time around, allocate and set the timer goin...
    frame_handle = (frame_handle_t *) calloc(1, sizeof(frame_handle_t));
    frame_handle->ctrans_link = link;
    frame_handle->frame_name = frame_name;
    frame_handle->relative_to = relative_to;
    frame_handle->was_updated = 0;
    frame_handle->update_channel = NULL;
    frame_handle->subscription = NULL;
    g_hash_table_insert(self->frame_handles_by_name, (gpointer) frame_name, (gpointer) frame_handle);

    //get the update channel
    char * update_channel = NULL;
    if (history > 0) {
      sprintf(param_key, "coordinate_frames.%s.update_channel", frame_name);
      ret = bot_param_get_str(self->bot_param, param_key, &update_channel);
      if (ret < 0) {
        fprintf(stderr, "BotFrames Error: could not get 'update_channel' for frame %s\n", frame_name);
        goto fail;
      }

      //add the entry to the update_channel hash table
      frame_handle_t * entry = (frame_handle_t *) g_hash_table_lookup(self->frame_handles_by_channel, update_channel);
      if (entry != NULL) {
        fprintf(stderr, "BotFrames Error: update_channel %s for frame %s already used for frame %s\n", update_channel,
            frame_name, entry->frame_name);
        goto fail;
      }
      //first time around, allocate and set the timer goin...
      frame_handle->update_channel = update_channel;
      frame_handle->subscription = bot_core_isometry_t_subscribe(self->lcm, update_channel, on_frame_update,
          (void*) self);
      g_hash_table_insert(self->frame_handles_by_channel, (gpointer) update_channel, (gpointer) frame_handle);

    }

  }
  g_strfreev(frame_names);
  g_mutex_unlock(self->mutex);
  return self;

  fail: g_mutex_unlock(self->mutex);
  bot_frames_destroy(self);
  return NULL;

}

static void _update_handler_t_destroy(void * data, void * user)
{
  g_slice_free(update_handler_t, data);
}

void bot_frames_destroy(BotFrames * bot_frames)
{

  g_mutex_free(bot_frames->mutex);

  bot_ctrans_destroy(bot_frames->ctrans);

  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init(&iter, bot_frames->frame_handles_by_name);
  int frame_num;
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    frame_handle_t * han = (frame_handle_t *) value;
    frame_handle_destroy(bot_frames->lcm, han);
  }
  g_hash_table_destroy(bot_frames->frame_handles_by_name);
  g_hash_table_destroy(bot_frames->frame_handles_by_channel);
  free(bot_frames->root_name);

  if (bot_frames->update_callbacks != NULL) {
    g_list_foreach(bot_frames->update_callbacks, _update_handler_t_destroy, NULL);
    g_list_free(bot_frames->update_callbacks);
  }

  g_slice_free(BotFrames, bot_frames);
}

void bot_frames_add_update_subscriber(BotFrames *bot_frames, bot_frames_link_update_handler_t * callback_func,
    void * user)
{
  update_handler_t * uh = g_slice_new0(update_handler_t);
  uh->callback_func = callback_func;
  uh->user = user;
  bot_frames->update_callbacks = g_list_append(bot_frames->update_callbacks, uh);

}

int bot_frames_get_trans_with_utime(BotFrames *bot_frames, const char *from_frame, const char *to_frame, int64_t utime,
    BotTrans *result)
{
  g_mutex_lock(bot_frames->mutex);
  int status = bot_ctrans_get_trans(bot_frames->ctrans, from_frame, to_frame, utime, result);
  g_mutex_unlock(bot_frames->mutex);
  return status;
}

int bot_frames_get_trans(BotFrames *bot_frames, const char *from_frame, const char *to_frame, BotTrans *result)
{
  g_mutex_lock(bot_frames->mutex);
  int status = bot_ctrans_get_trans_latest(bot_frames->ctrans, from_frame, to_frame, result);
  g_mutex_unlock(bot_frames->mutex);
  return status;
}

int bot_frames_get_trans_mat_3x4(BotFrames *bot_frames, const char *from_frame, const char *to_frame, double mat[12])
{
  BotTrans bt;
  if (!bot_frames_get_trans(bot_frames, from_frame, to_frame, &bt))
    return 0;
  bot_trans_get_mat_3x4(&bt, mat);
  return 1;
}

int bot_frames_get_trans_mat_3x4_with_utime(BotFrames *bot_frames, const char *from_frame, const char *to_frame,
    int64_t utime, double mat[12])
{
  BotTrans bt;
  if (!bot_frames_get_trans_with_utime(bot_frames, from_frame, to_frame, utime, &bt))
    return 0;
  bot_trans_get_mat_3x4(&bt, mat);
  return 1;
}

int bot_frames_get_trans_mat_4x4(BotFrames *bot_frames, const char *from_frame, const char *to_frame, double mat[12])
{
  BotTrans bt;
  if (!bot_frames_get_trans(bot_frames, from_frame, to_frame, &bt))
    return 0;
  bot_trans_get_mat_4x4(&bt, mat);
  return 1;
}

int bot_frames_get_trans_mat_4x4_with_utime(BotFrames *bot_frames, const char *from_frame, const char *to_frame,
    int64_t utime, double mat[12])
{
  BotTrans bt;
  if (!bot_frames_get_trans_with_utime(bot_frames, from_frame, to_frame, utime, &bt))
    return 0;
  bot_trans_get_mat_4x4(&bt, mat);
  return 1;
}

int bot_frames_get_trans_latest_timestamp(BotFrames *bot_frames, const char *from_frame, const char *to_frame,
    int64_t *timestamp)
{
  g_mutex_lock(bot_frames->mutex);
  int status = bot_ctrans_get_trans_latest_timestamp(bot_frames->ctrans, from_frame, to_frame, timestamp);
  g_mutex_unlock(bot_frames->mutex);
  return status;
}

int bot_frames_have_trans(BotFrames *bot_frames, const char *from_frame, const char *to_frame)
{
  g_mutex_lock(bot_frames->mutex);
  int status = bot_ctrans_have_trans(bot_frames->ctrans, from_frame, to_frame);
  g_mutex_unlock(bot_frames->mutex);
  return status;
}

int bot_frames_transform_vec(BotFrames *bot_frames, const char *from_frame, const char *to_frame, const double src[3],
    double dst[3])
{
  BotTrans rbtrans;
  if (!bot_frames_get_trans(bot_frames, from_frame, to_frame, &rbtrans))
    return 0;
  bot_trans_apply_vec(&rbtrans, src, dst);
  return 1;
}

int bot_frames_rotate_vec(BotFrames *bot_frames, const char *from_frame, const char *to_frame, const double src[3],
    double dst[3])
{
  BotTrans rbtrans;
  if (!bot_frames_get_trans(bot_frames, from_frame, to_frame, &rbtrans))
    return 0;
  bot_trans_rotate_vec(&rbtrans, src, dst);
  return 1;
}

int bot_frames_get_n_trans(BotFrames *bot_frames, const char *from_frame, const char *to_frame, int nth_from_latest)
{
  BotCTransLink *link = bot_ctrans_get_link(bot_frames->ctrans, from_frame, to_frame);
  if (!link)
    return 0;
  return bot_ctrans_link_get_n_trans(link);
}

/**
 * Returns: 1 on success, 0 on failure
 */
int bot_frames_get_nth_trans(BotFrames *bot_frames, const char *from_frame, const char *to_frame, int nth_from_latest,
    BotTrans *btrans, int64_t *timestamp)
{
  BotCTransLink *link = bot_ctrans_get_link(bot_frames->ctrans, from_frame, to_frame);
  if (!link)
    return 0;
  int status = bot_ctrans_link_get_nth_trans(link, nth_from_latest, btrans, timestamp);
  if (status && btrans && 0 != strcmp(to_frame, bot_ctrans_link_get_to_frame(link))) {
    bot_trans_invert(btrans);
  }
  return status;
}

char * bot_frames_get_root_name(BotFrames * bot_frames)
{
  return strdup(bot_frames->root_name);
}

int bot_frames_get_num_frames(BotFrames * bot_frames)
{
  return g_hash_table_size(bot_frames->frame_handles_by_name);
}

char ** bot_frames_get_frame_names(BotFrames * bot_frames)
{
  int num_frames = bot_frames_get_num_frames(bot_frames);

  char ** frames = calloc(num_frames + 1, sizeof(char*));

  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init(&iter, bot_frames->frame_handles_by_name);
  int frame_num = 0;
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    frame_handle_t * han = (frame_handle_t *) value;
    frames[frame_num++] = strdup(han->frame_name);
  }
  assert(frame_num==num_frames);
  return frames;
}

static BotFrames *global_bot_frames = NULL;
static GStaticMutex bot_frames_global_mutex = G_STATIC_MUTEX_INIT;

BotFrames*
bot_frames_get_global(lcm_t *lcm, BotParam *bot_param)
{
  if (lcm == NULL)
    lcm = bot_lcm_get_global(NULL);
  if (bot_param == NULL)
    bot_param = bot_param_get_global(lcm, 0);

  g_static_mutex_lock(&bot_frames_global_mutex);
  if (global_bot_frames == NULL) {
    global_bot_frames = bot_frames_new(lcm, bot_param);
    if (!global_bot_frames)
      goto fail;
  }

  BotFrames *result = global_bot_frames;
  g_static_mutex_unlock(&bot_frames_global_mutex);
  return result;

  fail: g_static_mutex_unlock(&bot_frames_global_mutex);
  fprintf(stderr, "ERROR: Could not get global bot_frames!\n");
  return NULL;
}
