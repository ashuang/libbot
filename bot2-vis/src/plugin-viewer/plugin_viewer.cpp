#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <gtk/gtk.h>
#include <iostream>

#include <bot_vis/bot_vis.h>
#include <lcm/lcm.h>
#include <bot_core/bot_core.h>

using namespace std;

static void on_perspective_item(GtkMenuItem *mi, void *user)
{
  BotViewer* viewer = (BotViewer*) user;
  BotViewHandler *vhandler = viewer->view_handler;
  if (vhandler) {
    vhandler->set_camera_perspective(vhandler, 60);
  }
}

static void on_orthographic_item(GtkMenuItem *mi, void *user)
{
  BotViewer* viewer = (BotViewer*) user;
  BotViewHandler *vhandler = viewer->view_handler;
  if (vhandler) {
    vhandler->set_camera_orthographic(vhandler);
  }
}

static void on_top_view_clicked(GtkToggleToolButton *tb, void *user_data)
{
  BotViewer *self = (BotViewer*) user_data;

  double eye[3];
  double look[3];
  double up[3];
  self->view_handler->get_eye_look(self->view_handler, eye, look, up);

  eye[0] = 0;
  eye[1] = 0;
  eye[2] = 10;
  look[0] = 0;
  look[1] = 0;
  look[2] = 0;
  up[0] = 0;
  up[1] = 10;
  up[2] = 0;
  self->view_handler->set_look_at(self->view_handler, eye, look, up);

  bot_viewer_request_redraw(self);
}

//TODO: the view controls stuff could probably be improved
void add_view_controls(BotViewer* viewer)
{
  //add perspective and orthographic controls...
  GtkWidget *view_menuitem = gtk_menu_item_new_with_mnemonic("_View");
  gtk_menu_bar_append(GTK_MENU_BAR(viewer->menu_bar), view_menuitem);

  GtkWidget *view_menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_menuitem), view_menu);

  GSList *view_list = NULL;
  GtkWidget *orthographic_item = gtk_radio_menu_item_new_with_label(view_list, "Orthographic");
  view_list = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(orthographic_item));
  gtk_menu_append(GTK_MENU(view_menu), orthographic_item);
  g_signal_connect(G_OBJECT(orthographic_item), "activate", G_CALLBACK(on_orthographic_item), viewer);

  GtkWidget *perspective_item = gtk_radio_menu_item_new_with_label(view_list, "Perspective");
  view_list = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(perspective_item));
  gtk_menu_append(GTK_MENU(view_menu), perspective_item);
  g_signal_connect(G_OBJECT(perspective_item), "activate", G_CALLBACK(on_perspective_item), viewer);

  gtk_widget_show_all(view_menuitem);

//  // add custom TOP VIEW button
//  GtkWidget *top_view_button;
//  top_view_button = (GtkWidget *) gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_FIT);
//  gtk_tool_button_set_label(GTK_TOOL_BUTTON(top_view_button), "Top View");
//  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(top_view_button), viewer->tips, "Switch to Top View", NULL);
//  gtk_toolbar_insert(GTK_TOOLBAR(viewer->toolbar), GTK_TOOL_ITEM(top_view_button), 4);
//  gtk_widget_show(top_view_button);
//  g_signal_connect(G_OBJECT(top_view_button), "clicked", G_CALLBACK(on_top_view_clicked), viewer);
}

int main(int argc, char *argv[])
{
  gtk_init(&argc, &argv);
  glutInit(&argc, argv);
  g_thread_init(NULL);

  if (argc < 2) {
    fprintf(stderr, "usage: %s <render_plugins>\n", g_path_get_basename(argv[0]));
    exit(1);
  }
  lcm_t * lcm = bot_lcm_get_global(NULL);
  bot_glib_mainloop_attach_lcm(lcm);

  BotViewer* viewer = bot_viewer_new("Plugin Viewer");
  //die cleanly for control-c etc :-)
  bot_gtk_quit_on_interrupt();

  // setup renderers
  bot_viewer_add_stock_renderer(viewer, BOT_VIEWER_STOCK_RENDERER_GRID, 1);

  // load dynamic objects
  for (int i = 1; i < argc; i++) {
    const char *plugin_so_fname = argv[i];

    // open the library
    void* handle = dlopen(plugin_so_fname, RTLD_LAZY);
    if (!handle) {
      cerr << "Cannot open library: " << dlerror() << '\n';
      return 1;
    }

    // load the symbol
    typedef void (*add_renderer_t)(BotViewer* viewer, int priority);

    // reset errors
    dlerror();
    add_renderer_t add_renderer = (add_renderer_t) dlsym(handle, "add_renderer_to_plugin_viewer");
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
      cerr << "Cannot load symbol \"add_renderer_to_plugin_viewe\": " << dlsym_error << '\n';
      dlclose(handle);
      return 1;
    }
    add_renderer(viewer, i);

  }

  //add view Controls
  add_view_controls(viewer);

  //load the renderer params from the config file.
  char *fname = g_build_filename(g_get_user_config_dir(), ".bot-plugin-viewerrc", NULL);
  bot_viewer_load_preferences(viewer, fname);

  gtk_main();

  //save the renderer params to the config file.
  bot_viewer_save_preferences(viewer, fname);

  bot_viewer_unref(viewer);
}
