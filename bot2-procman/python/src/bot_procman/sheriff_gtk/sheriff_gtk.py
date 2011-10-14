#!/usr/bin/env python

import os
import sys
import time
import traceback
import getopt
import subprocess
import signal
import pickle

import glib
import gobject
import gtk
import pango

from lcm import LCM

from bot_procman.orders_t import orders_t
import bot_procman.sheriff as sheriff
import bot_procman.sheriff_config as sheriff_config

import bot_procman.sheriff_gtk.command_model as cm
import bot_procman.sheriff_gtk.command_treeview as ctv
import bot_procman.sheriff_gtk.add_modify_command_dialog as amcd
import bot_procman.sheriff_gtk.command_console as cc

try:
    from bot_procman.build_prefix import BUILD_PREFIX
except ImportError:
    BUILD_PREFIX = None

def find_bot_procman_deputy_cmd():
    search_path = []
    if BUILD_PREFIX is not None:
        search_path.append("%s/bin" % BUILD_PREFIX)
    search_path.extend(os.getenv("PATH").split(":"))
    for dirname in search_path:
        fname = "%s/bot-procman-deputy" % dirname
        if os.path.exists(fname) and os.path.isfile(fname):
            return fname
    return None

class SheriffHostModel(gtk.ListStore):
    COL_OBJ, \
    COL_NAME, \
    COL_LAST_UPDATE, \
    COL_LOAD, \
    NUM_ROWS = range(5)

    def __init__(self, _sheriff):
        super(SheriffHostModel, self).__init__(gobject.TYPE_PYOBJECT,
                gobject.TYPE_STRING, # deputy name
                gobject.TYPE_STRING, # last update time
                gobject.TYPE_STRING, # load
                )
        self.sheriff = _sheriff

    def update(self):
        to_update = set(self.sheriff.get_deputies ())
        to_remove = []

        def _deputy_last_update_str (dep):
            if dep.last_update_utime:
                now_utime = time.time () * 1000000
                return "%.1f seconds ago" % ((now_utime-dep.last_update_utime) * 1e-6)
            else:
                return "<never>"

        def _update_host_row (model, path, model_iter, user_data):
            deputy = model.get_value (model_iter, SheriffHostModel.COL_OBJ)
            if deputy in to_update:
                model.set (model_iter, 
                        SheriffHostModel.COL_LAST_UPDATE, 
                        _deputy_last_update_str (deputy),
                        SheriffHostModel.COL_LOAD, 
                        "%f" % deputy.cpu_load,
                        )
                to_update.remove (deputy)
            else:
                to_remove.append (gtk.TreeRowReference (model, path))

        self.foreach (_update_host_row, None)

        for trr in to_remove:
            self.remove (self.get_iter (trr.get_path()))

        for deputy in to_update:
            print "adding %s to treeview" % deputy.name
            new_row = (deputy, deputy.name, _deputy_last_update_str (deputy),
                    "%f" % deputy.cpu_load,
                    )
            self.append (new_row)


class SheriffHostTreeView(gtk.TreeView):
    def __init__(self, _sheriff, hosts_ts):
        super(SheriffHostTreeView, self).__init__(hosts_ts)
        self.sheriff = _sheriff
        self.hosts_ts = hosts_ts

        plain_tr = gtk.CellRendererText ()
        col = gtk.TreeViewColumn ("Host", plain_tr, text=SheriffHostModel.COL_NAME)
        col.set_sort_column_id (1)
        col.set_resizable (True)
        self.append_column (col)

        last_update_tr = gtk.CellRendererText()
        col = gtk.TreeViewColumn ("Last update", last_update_tr, text=SheriffHostModel.COL_LAST_UPDATE)
#        col.set_sort_column_id (2) # XXX this triggers really weird bugs...
        col.set_resizable (True)
        col.set_cell_data_func(last_update_tr, self._deputy_last_update_cell_data_func)
        self.append_column (col)

        col = gtk.TreeViewColumn ("Load", plain_tr, text=SheriffHostModel.COL_LOAD)
        col.set_resizable (True)
        self.append_column (col)

        self.connect ("button-press-event", 
                self._on_hosts_tv_button_press_event)

        # hosts treeview context menu
        self.hosts_ctxt_menu = gtk.Menu ()

        self.cleanup_hosts_ctxt_mi = gtk.MenuItem ("_Cleanup")
        self.hosts_ctxt_menu.append (self.cleanup_hosts_ctxt_mi)
        self.cleanup_hosts_ctxt_mi.connect ("activate", 
                self._cleanup_hosts)
        self.hosts_ctxt_menu.show_all()

    def _on_hosts_tv_button_press_event (self, treeview, event):
        if event.type == gtk.gdk.BUTTON_PRESS and event.button == 3:
            self.hosts_ctxt_menu.popup (None, None, None, event.button, event.time)
            return True

    def _cleanup_hosts(self, *args):
        self.sheriff.purge_useless_deputies()
        self.hosts_ts.update()

    def _deputy_last_update_cell_data_func (self, column, cell, model, model_iter):
        # bit of a hack to pull out the last update time
        try:
            last_update = float(model.get_value(model_iter, SheriffHostModel.COL_LAST_UPDATE).split()[0])
        except:
            last_update = None
        if last_update is None or last_update > 5:
            cell.set_property("cell-background-set", True)
            cell.set_property("cell-background", "Red")
        elif last_update > 2:
            cell.set_property("cell-background-set", True)
            cell.set_property("cell-background", "Yellow")
        else:
            cell.set_property("cell-background-set", False)

#    def _get_selected_hosts (self):
#        model, rows = self.hosts_tv.get_selection ().get_selected_rows ()
#        return [ model.get_value (model.get_iter(path), 0) \
#                for path in rows ]

class SheriffGtkConfig(object):
    def __init__(self):
        self.show_columns = [ True ] * cm.NUM_CMDS_ROWS
        config_dir = os.path.join(glib.get_user_config_dir(), "procman-sheriff")
        if not os.path.exists(config_dir):
            os.makedirs(config_dir)
        self.config_fname = os.path.join(config_dir, "config")
    
    def save(self):
        d = {}
        for i, val in enumerate(self.show_columns):
            d["show_column_%d" % i] = val

        try:
            pickle.dump(d, open(self.config_fname, "w"))
        except Exception, err:
            print err

    def load(self):
        if not os.path.exists(self.config_fname):
            return
        try:
            d = pickle.load(open(self.config_fname, "r"))
            for i in range(len(self.show_columns)):
                self.show_columns[i] = d["show_column_%d" % i]
        except Exception, err:
            print err
            return

class SheriffGtk(object):
    def __init__ (self, lc):
        self.lc = lc
        self.next_cmds_update_time = 0
        self.config_filename = None
        self.gui_config = SheriffGtkConfig()
        self.gui_config.load()

        # deputy spawned by the sheriff
        self.spawned_deputy = None

        # create sheriff and subscribe to events
        self.sheriff = sheriff.Sheriff (self.lc)
        self.sheriff.connect ("command-added", self._update_commands_model)
        self.sheriff.connect ("command-removed", self._update_commands_model)
        self.sheriff.connect ("command-status-changed", self._update_commands_model)
        self.sheriff.connect ("command-group-changed", self._update_commands_model)

        gobject.timeout_add (1000, self._maybe_send_orders)
        gobject.timeout_add (1000, 
                lambda *s: self._update_commands_model () or True)

        self.lc.subscribe ("PMD_ORDERS", self.on_procman_orders)

        # setup GUI
        self.window = gtk.Window (gtk.WINDOW_TOPLEVEL)
        self.window.set_default_size (800, 600)
        self.window.connect ("delete-event", gtk.main_quit)
        self.window.connect ("destroy-event", gtk.main_quit)

        vbox = gtk.VBox ()
        self.window.add (vbox)

        self.cmds_ts = cm.SheriffCommandModel(self.sheriff)
        self.cmds_tv = ctv.SheriffCommandTreeView(self.sheriff, self.cmds_ts, self.gui_config)

        # keyboard accelerators.  This probably isn't the right way to do it...
        self.accel_group = gtk.AccelGroup ()
        self.accel_group.connect_group (ord("n"), gtk.gdk.CONTROL_MASK,
                gtk.ACCEL_VISIBLE, lambda *a: None)
        self.accel_group.connect_group (ord("s"), gtk.gdk.CONTROL_MASK,
                gtk.ACCEL_VISIBLE, lambda *a: None)
        self.accel_group.connect_group (ord("t"), gtk.gdk.CONTROL_MASK,
                gtk.ACCEL_VISIBLE, lambda *a: None)
        self.accel_group.connect_group (ord("e"), gtk.gdk.CONTROL_MASK,
                gtk.ACCEL_VISIBLE, lambda *a: None)
        self.accel_group.connect_group (ord("q"), gtk.gdk.CONTROL_MASK,
                gtk.ACCEL_VISIBLE, gtk.main_quit)
        self.accel_group.connect_group (ord("o"), gtk.gdk.CONTROL_MASK,
                gtk.ACCEL_VISIBLE, lambda *a: None)
        self.accel_group.connect_group (ord("a"), gtk.gdk.CONTROL_MASK,
                gtk.ACCEL_VISIBLE, 
                lambda *a: self.cmds_tv.get_selection ().select_all ())
        self.accel_group.connect_group (ord("d"), gtk.gdk.CONTROL_MASK,
                gtk.ACCEL_VISIBLE, 
                lambda *a: self.cmds_tv.get_selection ().unselect_all ())
#        self.accel_group.connect_group (ord("a"), gtk.gdk.CONTROL_MASK,
#                gtk.ACCEL_VISIBLE, self._do_save_config_dialog)
        self.accel_group.connect_group (gtk.gdk.keyval_from_name ("Delete"), 0,
                gtk.ACCEL_VISIBLE, self.cmds_tv._remove_selected_commands)
        self.window.add_accel_group (self.accel_group)

        # setup the menu bar
        menu_bar = gtk.MenuBar ()
        vbox.pack_start (menu_bar, False, False, 0)

        file_mi = gtk.MenuItem ("_File")
        options_mi = gtk.MenuItem ("_Options")
        commands_mi = gtk.MenuItem ("_Commands")
        view_mi = gtk.MenuItem ("_View")
        
        # file menu
        file_menu = gtk.Menu ()
        file_mi.set_submenu (file_menu)

        self.load_cfg_mi = gtk.ImageMenuItem (gtk.STOCK_OPEN)
        self.load_cfg_mi.add_accelerator ("activate", self.accel_group, 
                ord("o"), gtk.gdk.CONTROL_MASK, gtk.ACCEL_VISIBLE)
        self.save_cfg_mi = gtk.ImageMenuItem (gtk.STOCK_SAVE_AS)
        quit_mi = gtk.ImageMenuItem (gtk.STOCK_QUIT)
        quit_mi.add_accelerator ("activate", self.accel_group, ord("q"),
                gtk.gdk.CONTROL_MASK, gtk.ACCEL_VISIBLE)
        file_menu.append (self.load_cfg_mi)
        file_menu.append (self.save_cfg_mi)
        file_menu.append (quit_mi)
        self.load_cfg_mi.connect ("activate", self._do_load_config_dialog)
        self.save_cfg_mi.connect ("activate", self._do_save_config_dialog)
        quit_mi.connect ("activate", gtk.main_quit)

        # load, save dialogs
        self.load_dlg = None
        self.save_dlg = None
        self.load_save_dir = None
        if BUILD_PREFIX and os.path.exists("%s/data/procman" % BUILD_PREFIX):
            self.load_save_dir = "%s/data/procman" % BUILD_PREFIX

        # commands menu
        commands_menu = gtk.Menu ()
        commands_mi.set_submenu (commands_menu)
        self.start_cmd_mi = gtk.MenuItem ("_Start")
        self.start_cmd_mi.add_accelerator ("activate", 
                self.accel_group, ord("s"),
                gtk.gdk.CONTROL_MASK, gtk.ACCEL_VISIBLE)
        self.start_cmd_mi.connect ("activate", self.cmds_tv._start_selected_commands)
        self.start_cmd_mi.set_sensitive (False)
        commands_menu.append (self.start_cmd_mi)

        self.stop_cmd_mi = gtk.MenuItem ("S_top")
        self.stop_cmd_mi.add_accelerator ("activate", 
                self.accel_group, ord("t"),
                gtk.gdk.CONTROL_MASK, gtk.ACCEL_VISIBLE)
        self.stop_cmd_mi.connect ("activate", self.cmds_tv._stop_selected_commands)
        self.stop_cmd_mi.set_sensitive (False)
        commands_menu.append (self.stop_cmd_mi)

        self.restart_cmd_mi = gtk.MenuItem ("R_estart")
        self.restart_cmd_mi.add_accelerator ("activate",
                self.accel_group, ord ("e"),
                gtk.gdk.CONTROL_MASK, gtk.ACCEL_VISIBLE)
        self.restart_cmd_mi.connect ("activate", 
                self.cmds_tv._restart_selected_commands)
        self.restart_cmd_mi.set_sensitive (False)
        commands_menu.append (self.restart_cmd_mi)

        self.remove_cmd_mi = gtk.MenuItem ("_Remove")
        self.remove_cmd_mi.add_accelerator ("activate", self.accel_group, 
                gtk.gdk.keyval_from_name ("Delete"), 0, gtk.ACCEL_VISIBLE)
        self.remove_cmd_mi.connect ("activate", self.cmds_tv._remove_selected_commands)
        self.remove_cmd_mi.set_sensitive (False)
        commands_menu.append (self.remove_cmd_mi)

        commands_menu.append (gtk.SeparatorMenuItem ())

        self.new_cmd_mi = gtk.MenuItem ("_New command")
        self.new_cmd_mi.add_accelerator ("activate", self.accel_group, ord("n"),
                gtk.gdk.CONTROL_MASK, gtk.ACCEL_VISIBLE)
        self.new_cmd_mi.connect ("activate", 
                lambda *s: amcd.do_add_command_dialog(self.sheriff, self.cmds_ts, self.window))
        commands_menu.append (self.new_cmd_mi)

        # options menu
        options_menu = gtk.Menu ()
        options_mi.set_submenu (options_menu)

        self.is_observer_cmi = gtk.CheckMenuItem ("_Observer")
        self.is_observer_cmi.connect ("activate", self.on_observer_mi_activate)
        options_menu.append (self.is_observer_cmi)

        self.spawn_deputy_cmi = gtk.MenuItem("Spawn Local _Deputy")
        self.spawn_deputy_cmi.connect("activate", self.on_spawn_deputy_activate)
        options_menu.append(self.spawn_deputy_cmi)

        self.terminate_spawned_deputy_cmi = gtk.MenuItem("_Terminate local deputy")
        self.terminate_spawned_deputy_cmi.connect("activate", self.on_terminate_spawned_deputy_activate)
        options_menu.append(self.terminate_spawned_deputy_cmi)
        self.terminate_spawned_deputy_cmi.set_sensitive(False)

        self.bot_procman_deputy_cmd = find_bot_procman_deputy_cmd()
        if not self.bot_procman_deputy_cmd:
            sys.stderr.write("Can't find bot-procman-deputy.  Spawn Deputy disabled")
            self.spawn_deputy_cmi.set_sensitive(False)

        # view menu
        view_menu = gtk.Menu ()
        view_mi.set_submenu (view_menu)

        menu_bar.append (file_mi)
        menu_bar.append (options_mi)
        menu_bar.append (commands_mi)
        menu_bar.append (view_mi)

        vpane = gtk.VPaned ()
        vbox.pack_start (vpane, True, True, 0)

        # setup the command treeview
        hpane = gtk.HPaned ()
        vpane.add1 (hpane)
        
        sw = gtk.ScrolledWindow ()
        sw.set_policy (gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        hpane.pack1 (sw, resize = True)
        sw.add (self.cmds_tv)

        cmds_sel = self.cmds_tv.get_selection()
        cmds_sel.connect ("changed", self._on_cmds_selection_changed)

        # create a checkable item in the View menu for each column to toggle
        # its visibility in the treeview
        for col in self.cmds_tv.get_columns():
            name = col.get_title ()
            if name == "Name":
                continue
            col_cmi = gtk.CheckMenuItem (name)
            col_cmi.set_active (col.get_visible())
            def on_activate(cmi, col_):
                col_.set_visible(cmi.get_active())
                self.gui_config.show_columns[col_.get_sort_column_id()] = cmi.get_active()
            col_cmi.connect ("activate", on_activate, col)
            view_menu.append (col_cmi)

        self.hosts_ts = SheriffHostModel(self.sheriff)
        self.hosts_tv = SheriffHostTreeView(self.sheriff, self.hosts_ts)
        sw = gtk.ScrolledWindow ()
        sw.set_policy (gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        hpane.pack2 (sw, resize = False)
        sw.add (self.hosts_tv)

        hpane.set_position (500)

        gobject.timeout_add (1000, lambda *s: self.hosts_ts.update() or True)

        # stdout textview
        self.cmd_console = cc.SheriffCommandConsole(self.sheriff, self.lc)
        vpane.add2(self.cmd_console)
        vpane.set_position (300)

        # status bar
        self.statusbar = gtk.Statusbar ()
        vbox.pack_start (self.statusbar, False, False, 0)

        vbox.show_all ()
        self.window.show ()

    def cleanup(self):
        self._terminate_spawned_deputy()
        self.gui_config.save()

    def _update_commands_model (self, *unused):
        now = time.time()
        if now < self.next_cmds_update_time:
            return
        self.cmds_ts.repopulate()
        self.next_cmds_update_time = time.time() + 0.3

    def _terminate_spawned_deputy(self):
        if self.spawned_deputy:
            try:
                self.spawned_deputy.terminate()
            except AttributeError: # python 2.4, 2.5 don't have Popen.terminate()
                os.kill(self.spawned_deputy.pid, signal.SIGTERM)
                self.spawned_deputy.wait()
        self.spawned_deputy = None

    def _set_observer (self, is_observer):
        self.sheriff.set_observer (is_observer)

        self._update_menu_item_sensitivities ()

        if is_observer: self.window.set_title ("Procman Observer")
        else: self.window.set_title ("Procman Sheriff")

        if self.is_observer_cmi != is_observer:
            self.is_observer_cmi.set_active (is_observer)

    # GTK signal handlers
    def _do_load_config_dialog (self, *args):
        if not self.load_dlg:
            self.load_dlg = gtk.FileChooserDialog ("Load Config", self.window, 
                    buttons = (gtk.STOCK_OPEN, gtk.RESPONSE_ACCEPT,
                        gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT))
        if self.load_save_dir:
            self.load_dlg.set_current_folder(self.load_save_dir)
        if gtk.RESPONSE_ACCEPT == self.load_dlg.run ():
            self.config_filename = self.load_dlg.get_filename ()
            self.load_save_dir = os.path.dirname(self.config_filename)
            try:
                cfg = sheriff_config.config_from_filename (self.config_filename)
            except Exception:
                msgdlg = gtk.MessageDialog (self.window,
                        gtk.DIALOG_MODAL|gtk.DIALOG_DESTROY_WITH_PARENT,
                        gtk.MESSAGE_ERROR, gtk.BUTTONS_CLOSE, 
                        traceback.format_exc ())
                msgdlg.run ()
                msgdlg.destroy ()
            else:
                self.sheriff.load_config (cfg)
        self.load_dlg.hide()
        self.load_dlg.destroy()
        self.load_dlg = None

    def _do_save_config_dialog (self, *args):
        if not self.save_dlg:
            self.save_dlg = gtk.FileChooserDialog ("Save Config", self.window,
                    action = gtk.FILE_CHOOSER_ACTION_SAVE,
                    buttons = (gtk.STOCK_SAVE, gtk.RESPONSE_ACCEPT,
                        gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT))
        if self.load_save_dir:
            self.save_dlg.set_current_folder(self.load_save_dir)
        if self.config_filename is not None:
            self.save_dlg.set_filename (self.config_filename)
        if gtk.RESPONSE_ACCEPT == self.save_dlg.run ():
            self.config_filename = self.save_dlg.get_filename ()
            self.load_save_dir = os.path.dirname(self.config_filename)
            try:
                self.sheriff.save_config (file (self.config_filename, "w"))
            except IOError, e:
                msgdlg = gtk.MessageDialog (self.window,
                        gtk.DIALOG_MODAL|gtk.DIALOG_DESTROY_WITH_PARENT,
                        gtk.MESSAGE_ERROR, gtk.BUTTONS_CLOSE, str (e))
                msgdlg.run ()
                msgdlg.destroy ()
        self.save_dlg.hide ()
        self.save_dlg.destroy()
        self.save_dlg = None

    def on_observer_mi_activate (self, menu_item):
        self._set_observer (menu_item.get_active ())

    def on_spawn_deputy_activate(self, *args):
        self._terminate_spawned_deputy()
        args = [ self.bot_procman_deputy_cmd, "-n", "localhost" ]
        self.spawned_deputy = subprocess.Popen(args)
        # TODO disable
        self.spawn_deputy_cmi.set_sensitive(False)
        self.terminate_spawned_deputy_cmi.set_sensitive(True)

    def on_terminate_spawned_deputy_activate(self, *args):
        self._terminate_spawned_deputy()
        self.spawn_deputy_cmi.set_sensitive(True)
        self.terminate_spawned_deputy_cmi.set_sensitive(False)

    def _update_menu_item_sensitivities (self):
        # enable/disable menu options based on sheriff state and user selection
        selected_cmds = self.cmds_tv.get_selected_commands ()
        can_modify = len(selected_cmds) > 0 and not self.sheriff.is_observer ()
        can_add_load = not self.sheriff.is_observer ()

        self.start_cmd_mi.set_sensitive (can_modify)
        self.stop_cmd_mi.set_sensitive (can_modify)
        self.restart_cmd_mi.set_sensitive (can_modify)
        self.remove_cmd_mi.set_sensitive (can_modify)

        self.new_cmd_mi.set_sensitive (can_add_load)
        self.load_cfg_mi.set_sensitive (can_add_load)

    def _on_cmds_selection_changed (self, selection):
        selected_cmds = self.cmds_tv.get_selected_commands ()
        if len (selected_cmds) == 1:
            self.cmd_console.show_command_buffer(selected_cmds[0])
        elif len (selected_cmds) == 0:
            self.cmd_console.show_sheriff_buffer()
        self._update_menu_item_sensitivities ()

    def _maybe_send_orders (self):
        if not self.sheriff.is_observer ():
            self.sheriff.send_orders ()
        return True

    # LCM handlers
    def on_procman_orders (self, channel, data):
        msg = orders_t.decode (data)
        if not self.sheriff.is_observer () and \
                self.sheriff.name != msg.sheriff_name:
            # detected the presence of another sheriff that is not this one.
            # self-demote to prevent command thrashing
            self._set_observer (True)

            self.statusbar.push (self.statusbar.get_context_id ("main"),
                    "WARNING: multiple sheriffs detected!  Switching to observer mode");
            gobject.timeout_add (6000, 
                    lambda *s: self.statusbar.pop (self.statusbar.get_context_id ("main")))

def usage():
    sys.stdout.write(
"""usage: %s [options] <procman_config_file>

Process Management operating console.

Options:
  -l, --lone-ranger   Automatically run a deputy within the sheriff process
                      This deputy terminates with the sheriff, along with
                      all the commands it hosts.

  -h, --help          Shows this help text

If <procman_config_file> is specified, then the sheriff tries to load
deputy commands from the file.

""" % os.path.basename(sys.argv[0]))
    sys.exit(1)

def run ():
    try:
        opts, args = getopt.getopt( sys.argv[1:], 'hl',
                ['help','lone-ranger'] )
    except getopt.GetoptError:
        usage()
        sys.exit(2)

    spawn_deputy = False

    for optval, argval in opts:
        if optval in [ '-l', '--lone-ranger' ]:
            spawn_deputy = True
        elif optval in [ '-h', '--help' ]:
            usage()

    cfg = None
    if len(args) > 0:
        cfg = sheriff_config.config_from_filename(args[0])

    lc = LCM ()
    def handle (*a):
        try:
            lc.handle ()
        except Exception:
            traceback.print_exc ()
        return True
    gobject.io_add_watch (lc, gobject.IO_IN, handle)
    gui = SheriffGtk(lc)
    if spawn_deputy:
        gui.on_spawn_deputy_activate()
    if cfg is not None:
        gobject.timeout_add (1300, lambda: gui.sheriff.load_config (cfg))
        gui.load_save_dir = os.path.dirname(args[0])
    try:
        gtk.main ()
    except KeyboardInterrupt:
        print("Exiting")
    gui.cleanup()

if __name__ == "__main__":
    run ()
