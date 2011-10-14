import gobject
import gtk

import bot_procman.sheriff as sheriff

COL_CMDS_TV_OBJ, \
COL_CMDS_TV_CMD, \
COL_CMDS_TV_NICKNAME, \
COL_CMDS_TV_HOST, \
COL_CMDS_TV_STATUS_ACTUAL, \
COL_CMDS_TV_CPU_USAGE, \
COL_CMDS_TV_MEM_VSIZE, \
COL_CMDS_TV_AUTO_RESPAWN, \
NUM_CMDS_ROWS = range(9)

class SheriffCommandModel(gtk.TreeStore):
    def __init__(self, _sheriff):
        super(SheriffCommandModel, self).__init__( \
                gobject.TYPE_PYOBJECT,
                gobject.TYPE_STRING, # command name
                gobject.TYPE_STRING, # command nickname
                gobject.TYPE_STRING, # host name
                gobject.TYPE_STRING, # status actual
                gobject.TYPE_STRING, # CPU usage
                gobject.TYPE_INT,    # memory vsize
                gobject.TYPE_BOOLEAN,# auto-respawn
                )

        self.sheriff = _sheriff
        self.group_row_references = {}

    def _find_or_make_group_row_reference (self, group_name):
        if not group_name:
            return None
        if group_name in self.group_row_references:
            return self.group_row_references[group_name]
        else:
            # add the group name to the command name column if visible
            # otherwise, add it to the nickname column
            ts_iter = self.append (None, 
                      ((None, group_name, "", "", "", "", 0, False)))

            trr = gtk.TreeRowReference (self, 
                    self.get_path (ts_iter))
            self.group_row_references[group_name] = trr
            return trr

    def get_known_group_names (self):
        return self.group_row_references.keys ()

    def _delete_group_row_reference (self, group_name):
        del self.group_row_references[group_name]

    def _update_cmd_row (self, model, path, model_iter, user_data):
        cmds, cmd_deps, to_remove, to_reparent = user_data

        obj_col = COL_CMDS_TV_OBJ
        cmd = model.get_value (model_iter, obj_col)
        if not cmd: 
            # row represents a procman group
            
            # get a list of all the row's children
            child_iter = model.iter_children (model_iter)
            children = []
            while child_iter:
                children.append (model.get_value (child_iter, obj_col))
                child_iter = model.iter_next (child_iter)

            if not children: 
                to_remove.append (gtk.TreeRowReference (model, path))
                return
            statuses = [ cmd.status () for cmd in children ]
            stopped_statuses = [sheriff.STOPPED_OK, sheriff.STOPPED_ERROR]
            if all ([s == statuses[0] for s in statuses]):
                status_str = statuses[0]
            elif all ([s in stopped_statuses for s in statuses]):
                status_str = "Stopped (Mixed)"
            else:
                status_str = "Mixed"
            cpu_total = sum ([cmd.cpu_usage for cmd in children])
            mem_total = sum ([cmd.mem_vsize_bytes / 1024 \
                    for cmd in children])
            cpu_str = "%.2f" % (cpu_total * 100)
            
            model.set (model_iter, 
                    COL_CMDS_TV_STATUS_ACTUAL, status_str,
                    COL_CMDS_TV_CPU_USAGE, cpu_str,
                    COL_CMDS_TV_MEM_VSIZE, mem_total)

            cur_grpname = \
                    model.get_value(model_iter, COL_CMDS_TV_CMD)

            if not cur_grpname:
                # add the group name to the command name column
                model.set (model_iter, 
                           COL_CMDS_TV_CMD, cmd.group)
            return
        if cmd in cmds:
#                extradata = cmd.get_data ("extradata")
            cpu_str = "%.2f" % (cmd.cpu_usage * 100)
            mem_usage = int (cmd.mem_vsize_bytes / 1024)

            name = cmd.name
            if cmd.nickname.strip():
                name = cmd.nickname

            model.set (model_iter, 
                    COL_CMDS_TV_CMD, name,
                    COL_CMDS_TV_NICKNAME, cmd.nickname,
                    COL_CMDS_TV_STATUS_ACTUAL, cmd.status (),
                    COL_CMDS_TV_HOST, cmd_deps[cmd].name,
                    COL_CMDS_TV_CPU_USAGE, cpu_str,
                    COL_CMDS_TV_MEM_VSIZE, mem_usage,
                    COL_CMDS_TV_AUTO_RESPAWN, cmd.auto_respawn)

            # check that the command is in the correct group in the
            # treemodel
            correct_grr = self._find_or_make_group_row_reference (cmd.group)
            correct_parent_iter = None
            correct_parent_path = None
            actual_parent_path = None
            if correct_grr:
                correct_parent_iter = model.get_iter(correct_grr.get_path())
            actual_parent_iter = model.iter_parent(model_iter)

            if correct_parent_iter:
                correct_parent_path = model.get_path(correct_parent_iter)
            if actual_parent_iter:
                actual_parent_path = model.get_path(actual_parent_iter)

            if correct_parent_path != actual_parent_path:
                print "moving %s (%s) (%s)" % (cmd.name,
                        correct_parent_path, actual_parent_path)
                # schedule the command to be moved
                to_reparent.append ((gtk.TreeRowReference (model, path),
                    correct_grr))

            cmds.remove (cmd)
        else:
            to_remove.append (gtk.TreeRowReference (model, path))

    def repopulate(self):
        cmds = set()
        cmd_deps = {}
        for deputy in self.sheriff.get_deputies ():
            for cmd in deputy.get_commands ():
                cmd_deps [cmd] = deputy
                cmds.add (cmd)
        to_remove = []
        to_reparent = []

        self.foreach(self._update_cmd_row, 
                (cmds, cmd_deps, to_remove, to_reparent))

        # reparent rows that are in the wrong group
        for trr, newparent_rr in to_reparent:
            orig_iter = self.get_iter (trr.get_path ())
            rowdata = self.get (orig_iter, *range(NUM_CMDS_ROWS))
            self.remove (orig_iter)

            newparent_iter = None
            if newparent_rr:
                newparent_iter = self.get_iter(newparent_rr.get_path())
            self.append(newparent_iter, rowdata)

        # remove rows that have been marked for deletion
        for trr in to_remove:
            cmds_iter = self.get_iter (trr.get_path())
            if not self.get_value (cmds_iter, 
                    COL_CMDS_TV_OBJ):
                self._delete_group_row_reference (self.get_value (cmds_iter,
                    COL_CMDS_TV_CMD))
            self.remove (cmds_iter)

        # remove group rows with no children
        groups_to_remove = []
        def _check_for_lonely_groups (model, path, model_iter, user_data):
            isgroup = not model.get_value(model_iter, COL_CMDS_TV_OBJ)
            if isgroup and not model.iter_has_child (model_iter): 
                groups_to_remove.append (gtk.TreeRowReference (model, path))
        self.foreach (_check_for_lonely_groups, None)
        for trr in groups_to_remove:
            model_iter = self.get_iter (trr.get_path())
            self._delete_group_row_reference (self.get_value (model_iter,
                COL_CMDS_TV_CMD))
            self.remove (model_iter)

        # create new rows for new commands
        for cmd in cmds:
            deputy = cmd_deps[cmd]
            parent = self._find_or_make_group_row_reference (cmd.group)

            new_row = (cmd,        # COL_CMDS_TV_OBJ
                cmd.name,          # COL_CMDS_TV_CMD
                cmd.nickname,      # COL_CMDS_TV_NICKNAME
                deputy.name,       # COL_CMDS_TV_HOST
                cmd.status (),     # COL_CMDS_TV_STATUS_ACTUAL
                "0",               # COL_CMDS_TV_CPU_USAGE
                0,                 # COL_CMDS_TV_MEM_VSIZE
                cmd.auto_respawn,  # COL_CMDS_TV_AUTO_RESPAWN
                )
            if parent:
                parent_iter = self.get_iter (parent.get_path ())
            else:
                parent_iter = None
            model_iter = self.append (parent_iter, new_row)

    def rows_to_commands(self, rows):
        col = COL_CMDS_TV_OBJ
        selected = []
        for path in rows:
            cmds_iter = self.get_iter (path)
            cmd = self.get_value (cmds_iter, col)
            if not cmd:
                child_iter = self.iter_children (cmds_iter)
                while child_iter:
                    selected.append (self.get_value (child_iter, col))
                    child_iter = self.iter_next (child_iter)
            else:
                selected.append (cmd)
        return selected

    def iter_to_command(self, model_iter):
        return self.get_value(model_iter, COL_CMDS_TV_OBJ)

    def path_to_command(self, path):
        return self.iter_to_command(self.get_iter(path))
