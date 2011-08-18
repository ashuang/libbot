import os
import platform
import sys
import time
#import types
import signal

import gobject

import lcm
from bot_procman.info_t import info_t
from bot_procman.orders_t import orders_t
from bot_procman.sheriff_cmd_t import sheriff_cmd_t
import bot_procman.sheriff_config as sheriff_config

#def warn (*args):
#    return common.print_to_stderr_with_lineno (*args)

def dbg (*args):
    return
    sys.stderr.write(*args)

def timestamp_now (): return int (time.time () * 1000000)

TRYING_TO_START = "Command Sent"
RUNNING = "Running"
TRYING_TO_STOP = "Command Sent"
REMOVING = "Command Sent"
STOPPED_OK = "Stopped (OK)"
STOPPED_ERROR = "Stopped (Error)"
UNKNOWN = "Unknown"
RESTARTING = "Command Sent"

class SheriffDeputyCommand (gobject.GObject):

    def __init__ (self):
        gobject.GObject.__init__ (self)
        self.sheriff_id = 0
        self.pid = -1
        self.exit_code = 0
        self.cpu_usage = 0
        self.mem_vsize_bytes = 0
        self.mem_rss_bytes = 0
        self.name = ""
        self.nickname = ""
        self.group = ""
        self.desired_runid = 0
        self.force_quit = 0
        self.scheduled_for_removal = False
        self.actual_runid = 0

    def _update_from_cmd_info (self, cmd_info):
        self.pid = cmd_info.pid
        self.actual_runid = cmd_info.actual_runid
        self.exit_code = cmd_info.exit_code
        self.cpu_usage = cmd_info.cpu_usage
        self.mem_vsize_bytes = cmd_info.mem_vsize_bytes
        self.mem_rss_bytes = cmd_info.mem_rss_bytes

    def _update_from_cmd_order (self, cmd_order):
        assert self.sheriff_id == cmd_order.sheriff_id
        self.name = cmd_order.name
        self.nickname = cmd_order.nickname
        self.group = cmd_order.group
        self.desired_runid = cmd_order.desired_runid
        self.force_quit = cmd_order.force_quit

    def _set_group (self, group): 
        self.group = group

    def _start (self):
        # if the command is already running, then ignore
        if self.pid > 0: 
#            warn ("command already running")
            return

        self.desired_runid += 1
        if self.desired_runid > (2 << 31): self.desired_runid = 1
        self.force_quit = 0

    def _restart (self):
        self.desired_runid += 1
        if self.desired_runid > (2 << 31): self.desired_runid = 1
        self.force_quit = 0

    def _stop (self):
        self.force_quit = 1

    def get_group_name (self):
        return self.group

    def status (self):
        if self.desired_runid != self.actual_runid and not self.force_quit:
            if self.pid == 0:
                return TRYING_TO_START
            else:
                return RESTARTING
        elif self.desired_runid == self.actual_runid:
            if self.pid > 0:
                if not self.force_quit and not self.scheduled_for_removal:
                    return RUNNING
                else:
                    return TRYING_TO_STOP
            else:
                if self.scheduled_for_removal: return REMOVING
                elif self.exit_code == 0:      return STOPPED_OK
                elif self.force_quit and \
                     os.WIFSIGNALED(self.exit_code) and \
                     os.WTERMSIG(self.exit_code) in [ signal.SIGTERM,
                             signal.SIGINT, signal.SIGKILL ]:
                         return STOPPED_OK
                else:                          return STOPPED_ERROR
        else: return UNKNOWN

    def __str__ (self):
        return """[%(name)s]
   group:        %(group)s
   sheriff_id:   %(sheriff_id)d
   pid:          %(pid)d
   exit_code:    %(exit_code)d
   cpu_usage:    %(cpu_usage)f
   mem_vsize:    %(mem_vsize_bytes)d
   mem_rss:      %(mem_rss_bytes)d
   actual_runid: %(actual_runid)d""" % self.__dict__

class SheriffDeputy (gobject.GObject):
    """
    Represents a procman sheriff's view of a remote deputy.
    """
    def __init__ (self, name):
        gobject.GObject.__init__ (self)
        self.name = name
        self.commands = {}
        self.last_update_utime = 0
        self.cpu_load = 0
#        self.clock_skew_usec = 0
#        self.last_orders_jitter_usec = 0
        self.phys_mem_total_bytes = 0
        self.phys_mem_free_bytes = 0
        self.variables = {}

    def get_commands (self):
        return self.commands.values ()

    def owns_command (self, command):
        return command.sheriff_id in self.commands and \
                self.commands [command.sheriff_id] is command

    def get_variables(self):
        return self.variables

    def set_variable(self, name, val):
        self.variables[name] = val

    def remove_variable(self, name):
        if name in self.variables:
            del self.variables[name]

    def get_variable(self, name):
        return self.variables[name]

    def _update_from_deputy_info (self, dep_info):
        """
        @dep_info: an instance of bot_procman.info_t
        """
        status_changes = []
        for cmd_info in dep_info.cmds:
            # look up the command, or create a new one if it's not found
            if cmd_info.sheriff_id in self.commands:
                cmd = self.commands[cmd_info.sheriff_id]
                old_status = cmd.status ()
            else:
                cmd = SheriffDeputyCommand ()
                cmd.sheriff_id = cmd_info.sheriff_id
                cmd.name = cmd_info.name
                cmd.nickname = cmd_info.nickname
                cmd.group = cmd_info.group
                cmd.desired_runid = cmd_info.actual_runid
                self._add_command(cmd)
                old_status = None

            cmd._update_from_cmd_info (cmd_info)
            new_status = cmd.status ()
            
            if old_status != new_status:
                status_changes.append ((cmd, old_status, new_status))

        updated_ids = [ cmd_info.sheriff_id for cmd_info in dep_info.cmds ]

        can_safely_remove = [ cmd for cmd in self.commands.values () \
                if cmd.scheduled_for_removal and \
                cmd.sheriff_id not in updated_ids ]

        for toremove in can_safely_remove:
            cmd = self.commands[toremove.sheriff_id]
            old_status = cmd.status ()
            status_changes.append ((cmd, old_status, None))
            del self.commands[toremove.sheriff_id]

        # TODO update variables

        self.last_update_utime = timestamp_now ()
#        self.clock_skew_usec = dep_info.clock_skew_usec
#        self.last_orders_jitter_usec = dep_info.last_orders_jitter_usec
        self.cpu_load = dep_info.cpu_load
        self.phys_mem_total_bytes = dep_info.phys_mem_total_bytes
        self.phys_mem_free_bytes = dep_info.phys_mem_free_bytes
        return status_changes

    def _update_from_deputy_orders (self, dep_orders):
        status_changes = []
        for cmd_order in dep_orders.cmds:
            if cmd_order.sheriff_id in self.commands:
                cmd = self.commands[cmd_order.sheriff_id]
                old_status = cmd.status ()
            else:
                cmd = SheriffDeputyCommand()
                cmd.sheriff_id = cmd_order.sheriff_id
                cmd.name = cmd_order.name
                cmd.nickname = cmd_order.nickname
                cmd.group = cmd_order.group
                cmd.desired_runid = cmd_order.desired_runid
                self._add_command(cmd)
                old_status = None
            cmd._update_from_cmd_order (cmd_order)
            new_status = cmd.status ()
            if old_status != new_status:
                status_changes.append ((cmd, old_status, new_status))
        updated_ids = [ order.sheriff_id for order in dep_orders.cmds ]
        for cmd in self.commands.values ():
            if cmd.sheriff_id not in updated_ids:
                old_status = cmd.status ()
                cmd.scheduled_for_removal = True
                new_status = cmd.status ()
                if old_status != new_status:
                    status_changes.append ((cmd, old_status, new_status))
        # TODO update variables
        return status_changes

    def _add_command (self, newcmd):
        assert newcmd.sheriff_id != 0
        assert isinstance(newcmd, SheriffDeputyCommand)
        self.commands[newcmd.sheriff_id] = newcmd

    def _schedule_command_for_removal (self, cmd):
        if not self.owns_command (cmd): raise KeyError ("invalid command")
        old_status = cmd.status ()
        cmd.scheduled_for_removal = True
        if not self.last_update_utime:
            del self.commands[cmd.sheriff_id]
            new_status = None
        else:
            new_status = cmd.status ()
        return ((cmd, old_status, new_status),)

    def _make_orders_message (self, sheriff_name):
        orders = orders_t ()
        orders.utime = timestamp_now ()
        orders.host = self.name
        orders.ncmds = len (self.commands)
        orders.sheriff_name = sheriff_name
        for cmd in self.commands.values ():
            if cmd.scheduled_for_removal:
                orders.ncmds -= 1
                continue
            cmd_msg = sheriff_cmd_t ()
            cmd_msg.name = cmd.name
            cmd_msg.nickname = cmd.nickname
            cmd_msg.sheriff_id = cmd.sheriff_id
            cmd_msg.desired_runid = cmd.desired_runid
            cmd_msg.force_quit = cmd.force_quit
            cmd_msg.group = cmd.group
            orders.cmds.append (cmd_msg)
        orders.nvars = len(self.variables)
        for name, val in self.variables.items():
            orders.varnames.append(name)
            orders.varvals.append(val)
        return orders

class Sheriff (gobject.GObject):

    __gsignals__ = {
            'deputy-info-received' : (gobject.SIGNAL_RUN_LAST, 
                gobject.TYPE_NONE, (gobject.TYPE_PYOBJECT,)),
            'command-added' : (gobject.SIGNAL_RUN_LAST,
                gobject.TYPE_NONE, 
                (gobject.TYPE_PYOBJECT, gobject.TYPE_PYOBJECT)),
            'command-removed' : (gobject.SIGNAL_RUN_LAST,
                gobject.TYPE_NONE, 
                (gobject.TYPE_PYOBJECT, gobject.TYPE_PYOBJECT)),
            'command-status-changed' : (gobject.SIGNAL_RUN_LAST,
                gobject.TYPE_NONE,
                (gobject.TYPE_PYOBJECT, gobject.TYPE_PYOBJECT, 
                    gobject.TYPE_PYOBJECT)),
            'command-group-changed' : (gobject.SIGNAL_RUN_LAST,
                gobject.TYPE_NONE, (gobject.TYPE_PYOBJECT,))
            }

    def __init__ (self, lc):
        gobject.GObject.__init__ (self)
        self.lc = lc
        self.lc.subscribe ("PMD_INFO", self._on_pmd_info)
        self.lc.subscribe ("PMD_ORDERS", self._on_pmd_orders)
        self.deputies = {}
        self._is_observer = False
        self.name = platform.node () + ":" + str(os.getpid ()) + \
                ":" + str (timestamp_now ())
        self._next_sheriff_id = 1

    def _get_or_make_deputy (self, deputy_name):
        if deputy_name not in self.deputies:
            self.deputies[deputy_name] = SheriffDeputy (deputy_name)
        return self.deputies[deputy_name]

    def _maybe_emit_status_change_signals (self, deputy, status_changes):
        for cmd, old_status, new_status in status_changes:
            if old_status == new_status: continue
            if old_status is None:
                self.emit ("command-added", deputy, cmd)
            elif new_status is None:
                print "command-removed"
                self.emit ("command-removed", deputy, cmd)
            else:
                self.emit ("command-status-changed", cmd,
                        old_status, new_status)

    def _get_command_deputy (self, cmd):
        for deputy in self.deputies.values ():
            if deputy.owns_command (cmd):
                return deputy
        raise KeyError ()

    def _on_pmd_info (self, channel, data):
        try: dep_info = info_t.decode (data)
        except ValueError: 
            print ("invalid info message")
            return

        now = timestamp_now ()
        if (now - dep_info.utime) * 1e-6 > 30 and not self.is_observer:
            # ignore old messages
            return

        dbg ("received pmd info from [%s]" % dep_info.host)

        deputy = self._get_or_make_deputy (dep_info.host)
        status_changes = deputy._update_from_deputy_info (dep_info)

        self.emit ("deputy-info-received", deputy)
        self._maybe_emit_status_change_signals (deputy, status_changes)

    def _on_pmd_orders (self, channel, data):
        dep_orders = orders_t.decode (data)

        if not self._is_observer:
#            if dep_orders.sheriff_name != self.name:
#                warn ("multiple sheriffs.detected: [%s]" % \
#                        dep_orders.sheriff_name)
            return

        deputy = self._get_or_make_deputy (dep_orders.host)
        status_changes = deputy._update_from_deputy_orders (dep_orders)
        self._maybe_emit_status_change_signals (deputy, status_changes)

    def __get_free_sheriff_id (self):
        for i in range (1 << 16):
            collision = False
            for deputy in self.deputies.values ():
                if self._next_sheriff_id in deputy.commands:
                    collision = True
                    break
            
            if not collision: 
                result = self._next_sheriff_id

            self._next_sheriff_id += 1
            if self._next_sheriff_id > (1 << 30):
                self._next_sheriff_id = 1

            if not collision: 
                return result
        raise RuntimeError ("no available sheriff id")

    def send_orders (self):
        if self._is_observer:
            raise ValueError ("Can't send orders in Observer mode")
        for deputy in self.deputies.values ():
            msg = deputy._make_orders_message (self.name)
            self.lc.publish ("PMD_ORDERS", msg.encode ())

    def add_command (self, deputy_name, cmd_name, cmd_nickname, group):
        if self._is_observer:
            raise ValueError ("Can't add commands in Observer mode")
        dep = self._get_or_make_deputy (deputy_name)
        newcmd = SheriffDeputyCommand()
        newcmd.name = cmd_name
        newcmd.nickname = cmd_nickname
        newcmd.group = group
        newcmd.sheriff_id = self.__get_free_sheriff_id ()
        dep._add_command (newcmd)
        self.emit ("command-added", dep, newcmd)
        self.send_orders ()
        return newcmd

    def start_command (self, cmd):
        if self._is_observer:
            raise ValueError ("Can't modify commands in Observer mode")
        old_status = cmd.status ()
        cmd._start ()
        new_status = cmd.status ()
        deputy = self.get_command_deputy (cmd)
        self._maybe_emit_status_change_signals (deputy, 
                ((cmd, old_status, new_status),))
        self.send_orders ()

    def restart_command (self, cmd):
        if self._is_observer:
            raise ValueError ("Can't modify commands in Observer mode")
        old_status = cmd.status ()
        cmd._restart ()
        new_status = cmd.status ()
        deputy = self.get_command_deputy (cmd)
        self._maybe_emit_status_change_signals (deputy, 
                ((cmd, old_status, new_status),))
        self.send_orders ()
    
    def stop_command (self, cmd):
        if self._is_observer:
            raise ValueError ("Can't modify commands in Observer mode")
        old_status = cmd.status ()
        cmd._stop ()
        new_status = cmd.status ()
        deputy = self.get_command_deputy (cmd)
        self._maybe_emit_status_change_signals (deputy, 
                ((cmd, old_status, new_status),))
        self.send_orders ()

    def set_command_name (self, cmd, newname):
        cmd.name = newname

    def set_command_nickname (self, cmd, newnickname):
        cmd.nickname = newnickname

    def set_command_group (self, cmd, group_name):
        if self._is_observer:
            raise ValueError ("Can't modify commands in Observer mode")
#        deputy = self._get_command_deputy (cmd)
        old_group = cmd.get_group_name ()
        if old_group != group_name:
            cmd._set_group (group_name)
            self.emit ("command-group-changed", cmd)

    def schedule_command_for_removal (self, command):
        if self._is_observer:
            raise ValueError ("Can't remove commands in Observer mode")
        deputy = self.get_command_deputy (command)
        status_changes = deputy._schedule_command_for_removal (command)
        self._maybe_emit_status_change_signals (deputy, status_changes)
        self.send_orders ()

    def move_command_to_deputy(self, cmd, newdeputy):
        self.schedule_command_for_removal (cmd)
        self.add_command (newdeputy.name, cmd.name, cmd.nickname, cmd.group)

    def set_observer (self, is_observer): self._is_observer = is_observer
    def is_observer (self): return self._is_observer

    def get_deputies (self):
        return self.deputies.values ()

    def find_deputy (self, name):
        return self.deputies[name]

    def purge_useless_deputies(self):
        for deputy_name, deputy in self.deputies.items():
            cmds = deputy.commands.values() 
            if not deputy.commands or \
                    all([ cmd.scheduled_for_removal for cmd in cmds ]):
                del self.deputies[deputy_name]

    def get_command_by_id (self, command_id):
        for deputy in self.deputies.values ():
            if command_id in deputy.commands:
                return deputy.commands[command_id]
        raise KeyError ("No such command")
    
    def get_command_deputy (self, command):
        for deputy in self.deputies.values ():
            if command.sheriff_id in deputy.commands:
                return deputy
        raise KeyError ("No such command")

    def get_all_commands (self):
        cmds = []
        for dep in self.deputies.values ():
            cmds.extend (dep.commands.values ())
        return cmds

    def load_config (self, config_node):
        """
        config_node should be an instance of sheriff_config.ConfigNode
        """
        if self._is_observer:
            raise ValueError ("Can't load config in Observer mode")

        for dep in self.deputies.values ():
            for cmd in dep.commands.values ():
                self.schedule_command_for_removal (cmd)

        for host in config_node.hosts.values ():
            for cmd in host.commands:
                newcmd = self.add_command(host.name, 
                        cmd.attributes["exec"],
                        cmd.attributes["nickname"],
                        cmd.attributes["group"],
                        )
                dbg ("[%s] %s (%s) -> %s" % (newcmd.group, newcmd.name, newcmd.nickname, host.name))

    def save_config (self, file_obj):
        config_node = sheriff_config.ConfigNode ()
        for deputy in self.deputies.values ():
            host_node = sheriff_config.HostNode(deputy.name)
            config_node.add_host(host_node)
            for cmd in deputy.commands.values ():
                cmd_node = sheriff_config.CommandNode ()
                cmd_node.attributes["exec"] = cmd.name
                cmd_node.attributes["nickname"] = cmd.nickname
                cmd_node.attributes["group"] = cmd.group
                host_node.add_command (cmd_node)
        file_obj.write (str (config_node))

if __name__ == "__main__":
    try:
        cfg_fname = sys.argv[1]
    except IndexError:
        cfg = None
    else:
        cfg = sheriff_config.config_from_filename (cfg_fname)

    lc = lcm.LCM ()
    sheriff = Sheriff (lc)
    if cfg is not None:
        sheriff.load_config (cfg)

    sheriff.connect ("deputy-info-received", 
            lambda s, dep: sys.stdout.write("deputy info received from %s\n" % 
                dep.name))
    mainloop = gobject.MainLoop ()
    gobject.io_add_watch (lc, gobject.IO_IN, lambda *s: lc.handle () or True)
    gobject.timeout_add (1000, lambda *s: sheriff.send_orders () or True)
    mainloop.run ()
