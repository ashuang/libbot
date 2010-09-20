#include <lcm/lcm.h>

#include <bot2-core/bot2-core.h>
#include <bot2-vis/viewer.h>
#include <bot2-vis/gl_util.h>
#include <bot2-vis/gtk_util.h>
#include <lcmtypes/lcmgl_data_t.h>

#include "lcmgl-decode.h"
#include "lcmgl-bot-renderer.h"

typedef struct
{
    GPtrArray *backbuffer;
    GPtrArray *frontbuffer;
    int enabled;
} lcmgl_channel_t;

typedef struct _LcmglBotRenderer {
    BotRenderer renderer;
    BotGtkParamWidget *pw;
    BotViewer   *viewer;
    lcm_t     *lcm;

    GHashTable *channels;

} LcmglBotRenderer;

static void my_free( BotRenderer *renderer )
{
    LcmglBotRenderer *self = (LcmglBotRenderer*) renderer;

    free( self );
}

static void my_draw( BotViewer *viewer, BotRenderer *renderer )
{
    LcmglBotRenderer *self = (LcmglBotRenderer*) renderer->user;

    // iterate over each channel
    GList *keys = bot_g_hash_table_get_keys(self->channels);

    for (GList *kiter = keys; kiter; kiter=kiter->next) {
        lcmgl_channel_t *chan = g_hash_table_lookup(self->channels,
                                                   kiter->data);
        glPushMatrix();
        glPushAttrib(GL_ENABLE_BIT);

        if (chan->enabled) {
            // iterate over all the messages received for this channel
            for (int i = 0; i < chan->frontbuffer->len; i++) {
                lcmgl_data_t *data =
                    g_ptr_array_index(chan->frontbuffer, i);

                lcmgl_decode(data->data, data->datalen);
            }
        }
        glPopAttrib ();
        glPopMatrix();
    }
    g_list_free (keys);
}

static void on_lcmgl_data (const lcm_recv_buf_t *rbuf, const char *channel,
        const lcmgl_data_t *_msg, void *user_data )
{
    LcmglBotRenderer *self = (LcmglBotRenderer*) user_data;

    lcmgl_channel_t *chan = g_hash_table_lookup(self->channels, _msg->name);

    if (!chan) {
        chan = (lcmgl_channel_t*) calloc(1, sizeof(lcmgl_channel_t));
        chan->enabled=1;
        //chan->backbuffer = g_ptr_array_new();
        chan->frontbuffer = g_ptr_array_new();
        g_hash_table_insert(self->channels, strdup(_msg->name), chan);
        bot_gtk_param_widget_add_booleans (self->pw,
                0, strdup(_msg->name), 1, NULL);
    }

#if 0
    int current_scene = -1;
    if (chan->backbuffer->len > 0) {
        lcmgl_data_t *ld = g_ptr_array_index(chan->backbuffer, 0);
        current_scene = ld->scene;
    }

    // new scene?
    if (current_scene != _msg->scene) {

        // free objects in foreground buffer
        for (int i = 0; i < chan->frontbuffer->len; i++)
            lcmgl_data_t_destroy(g_ptr_array_index(chan->frontbuffer, i));
        g_ptr_array_set_size(chan->frontbuffer, 0);

        // swap front and back buffers
        GPtrArray *tmp = chan->backbuffer;
        chan->backbuffer = chan->frontbuffer;
        chan->frontbuffer = tmp;

        bot_viewer_request_redraw( self->viewer );
    }
#endif

    for (int i = 0; i < chan->frontbuffer->len; i++)
        lcmgl_data_t_destroy(g_ptr_array_index(chan->frontbuffer, i));
    g_ptr_array_set_size (chan->frontbuffer, 0);
    g_ptr_array_add(chan->frontbuffer, lcmgl_data_t_copy(_msg));
    bot_viewer_request_redraw( self->viewer );
}

static void on_param_widget_changed (BotGtkParamWidget *pw, const char *name, void *user)
{
    LcmglBotRenderer *self = (LcmglBotRenderer*) user;

    // iterate over each channel
    GList *keys = bot_g_hash_table_get_keys(self->channels);

    for (GList *kiter=keys; kiter; kiter=kiter->next) {
        lcmgl_channel_t *chan = g_hash_table_lookup(self->channels,
                                                   kiter->data);

        chan->enabled = bot_gtk_param_widget_get_bool (pw, kiter->data);
    }
    g_list_free (keys);

    bot_viewer_request_redraw(self->viewer);
}

static void on_clear_button(GtkWidget *button, LcmglBotRenderer *self)
{
    if (!self->viewer)
        return;

    // iterate over each channel
    GList *keys = bot_g_hash_table_get_keys(self->channels);
    for (GList *kiter = keys; kiter; kiter = kiter->next) {
        lcmgl_channel_t *chan = g_hash_table_lookup(self->channels, kiter->data);
        // iterate over all the messages received for this channel
        for (int i = 0; i < chan->frontbuffer->len; i++)
            lcmgl_data_t_destroy(g_ptr_array_index(chan->frontbuffer, i));
        g_ptr_array_set_size(chan->frontbuffer, 0);

    }
    g_list_free(keys);

    bot_viewer_request_redraw(self->viewer);
}

void 
lcmgl_add_renderer_to_bot_viewer(BotViewer* viewer, lcm_t* lcm, int priority)
{
    LcmglBotRenderer *self =
        (LcmglBotRenderer*) calloc(1, sizeof(LcmglBotRenderer));

    BotRenderer *renderer = &self->renderer;

    self->lcm = lcm;
    self->viewer = viewer;
    self->pw = BOT_GTK_PARAM_WIDGET(bot_gtk_param_widget_new());

    renderer->draw = my_draw;
    renderer->destroy = my_free;
    renderer->name = "LCM GL";
    renderer->widget = GTK_WIDGET(self->pw);
    renderer->enabled = 1;
    renderer->user = self;

    self->channels = g_hash_table_new(g_str_hash, g_str_equal);

    g_signal_connect (G_OBJECT (self->pw), "changed",
                      G_CALLBACK (on_param_widget_changed), self);

    lcmgl_data_t_subscribe(self->lcm, "LCMGL.*", on_lcmgl_data, self);

    bot_viewer_add_renderer(viewer, renderer, priority);

    GtkWidget *clear_button = gtk_button_new_with_label("Clear All");
    gtk_box_pack_start(GTK_BOX(renderer->widget), clear_button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(clear_button), "clicked", G_CALLBACK(on_clear_button), self);
    gtk_widget_show_all(renderer->widget);
}
