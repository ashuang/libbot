#include <gtk/gtk.h>

#include <bot/gl/gl_util.h>
#include <bot/bot_core.h>
#include <bot/viewer/viewer.h>

void setup_view_menu(Viewer *viewer);

static void 
on_perspective_item(GtkMenuItem *mi, void *user)
{
    if(! gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(mi)))
        return;
    Viewer *viewer = (Viewer*)user;
    if(viewer->view_handler)
        viewer->view_handler->set_camera_perspective(viewer->view_handler, 60);
}

static void on_orthographic_item(GtkMenuItem *mi, void *user)
{
    if(! gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(mi)))
        return;
    Viewer *viewer = (Viewer*)user;
    if(viewer->view_handler)
        viewer->view_handler->set_camera_orthographic(viewer->view_handler);
}

static void
on_setview_y_up_item(GtkMenuItem *mi, void *user)
{
    Viewer *viewer = (Viewer*)user;
    double eye[] = { 0, 0, 50 };
    double lookat[] = { 0, 0, 0 };
    double up[] = { 0, 1, 0 };
    if(viewer->view_handler)
        viewer->view_handler->set_look_at(viewer->view_handler, eye, lookat, up);
}

static void
on_setview_x_up_item(GtkMenuItem *mi, void *user)
{
    Viewer *viewer = (Viewer*)user;
    double eye[] = { 0, 0, 50 };
    double lookat[] = { 0, 0, 0 };
    double up[] = { 1, 0, 0 };
    if(viewer->view_handler)
        viewer->view_handler->set_look_at(viewer->view_handler, eye, lookat, up);
}

void
setup_view_menu(Viewer *viewer)
{
    // create the View menu
    GtkWidget *view_menuitem = gtk_menu_item_new_with_mnemonic("_View");
    gtk_menu_bar_append(GTK_MENU_BAR(viewer->menu_bar), view_menuitem);

    GtkWidget *view_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_menuitem), view_menu);

    // perspective menu item
    GSList *view_list = NULL;
    GtkWidget *perspective_item = gtk_radio_menu_item_new_with_label(view_list, 
            "Perspective");
    view_list = 
        gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(perspective_item));
    gtk_menu_append(GTK_MENU(view_menu), perspective_item);
    g_signal_connect(G_OBJECT(perspective_item), "activate", 
            G_CALLBACK(on_perspective_item), viewer);

    // orthographic menu item
    GtkWidget *orthographic_item = 
        gtk_radio_menu_item_new_with_label(view_list, "Orthographic");
    view_list = 
        gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(orthographic_item));
    gtk_menu_append(GTK_MENU(view_menu), orthographic_item);
    g_signal_connect(G_OBJECT(orthographic_item), "activate", 
            G_CALLBACK(on_orthographic_item), viewer);

    // bookmarks

    gtk_menu_append(GTK_MENU(view_menu), gtk_separator_menu_item_new());

    // predefined viewpoints
    GtkWidget *setview_y_up_item = gtk_menu_item_new_with_mnemonic("Y axis up");
    gtk_menu_append(GTK_MENU(view_menu), setview_y_up_item);
    g_signal_connect(G_OBJECT(setview_y_up_item), "activate", 
            G_CALLBACK(on_setview_y_up_item), viewer);

    // predefined viewpoints
    GtkWidget *setview_x_up_item = gtk_menu_item_new_with_mnemonic("X axis up");
    gtk_menu_append(GTK_MENU(view_menu), setview_x_up_item);
    g_signal_connect(G_OBJECT(setview_x_up_item), "activate", 
            G_CALLBACK(on_setview_x_up_item), viewer);

    gtk_widget_show_all(view_menuitem);
}
