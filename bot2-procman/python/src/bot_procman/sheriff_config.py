TokIdentifier = "Identifier"
TokOpenStruct = "OpenStruct"
TokCloseStruct = "CloseStruct"
TokAssign = "Assign"
TokEndStatement = "EndStatement"
TokString = "String"
TokEOF = "EOF"
TokComment = "Comment"
TokInteger = "Integer"

class Token(object):
    def __init__ (self, type, val):
        self.type = type
        self.val = val

class Tokenizer(object):
    def __init__ (self, f):
        self.f = f
        self.unget_char = None
        self.line_pos = 0
        self.line_len = 0
        self.line_buf = ""
        self.line_num = 1

    def _next_char (self):
        if self.unget_char is not None:
            c = self.unget_char
            self.unget_char = None
            return c
        else:
            if self.line_pos == self.line_len:
                self.line_buf = self.f.readline ()
                if not len (self.line_buf):
                    return ''
                self.line_len = len (self.line_buf)
                self.line_pos = 0

            c = self.line_buf[self.line_pos]
            self.line_pos += 1

        if c == '\n':
            self.line_num += 1
        return c

    def _ungetc (self, c):
        if not c: return
        self.unget_char = c

    def _unescape (self, c):
        d = { "n": "\n",
              "r": "\r",
              "t": "\t" }
        if c in d: return d[c]
        return c

    def next_token (self):
        c = self._next_char ()

        while c and c.isspace ():
            c = self._next_char ()
        if not c: return Token (TokEOF, "")

        simple_tokens = { \
                "=" : TokAssign,
                ";" : TokEndStatement,
                "{" : TokOpenStruct,
                "}" : TokCloseStruct
                }
        if c in simple_tokens:
            return Token (simple_tokens[c], c)

        tok_chars = [ c ]

        if c == "#":
            while True:
                c = self._next_char ()
                if not c or c == "\n":
                    return Token (TokComment, "".join (tok_chars))
                tok_chars.append (c)

        if c == "\"":
            tok_chars = []
            while True:
                c = self._next_char ()
                if c == "\n":
                    se = SyntaxError ("Unterminated string constant")
                    se.filename = ""
                    se.lineno = self.line_num
                    se.offset = self.line_pos
                    se.text = self.line_buf
                    raise se
                if c == "\\":   c = self._unescape (self._next_char ())
                elif not c or c == "\"":
                    return Token (TokString, "".join (tok_chars))
                tok_chars.append (c)

        if c.isalpha () or c == "_":
            while True:
                c = self._next_char ()
                if not c.isalnum () and c not in "_-":
                    self._ungetc (c)
                    return Token (TokIdentifier, "".join (tok_chars))
                tok_chars.append (c)

        if c.isdigit():
            while True:
                c = self._next_char()
                if not c.isdigit():
                    self._ungetc(c)
                    return Token(TokInteger, "".join(tok_chars))
                tok_chars.append(c)

        se = SyntaxError ()
        se.filename = ""
        se.lineno = self.line_num
        se.offset = self.line_pos
        se.text = self.line_buf
        raise se

def escape_str(text):
    def escape_char(c):
        if c in r'\"':
            return '\\' + c
        return c

    return "".join([ escape_char(c) for c in text ])

class CommandNode(object):
    def __init__ (self):
        self.attributes = { \
                "exec" : None,
                "host" : None,
                "group" : "",
                "nickname" : "",
                }

    def _get_str (self, indent = 0):
        s = "    " * indent
        lines = []
        nickname = self.attributes["nickname"]
        if len(nickname):
            lines.append (s + "cmd \"%s\" {" % escape_str(nickname))
        else:
            lines.append (s + "cmd {")
        pairs = self.attributes.items()
        pairs.sort()
        for key, val in pairs:
            if not val:
                continue
            if key in [ "group", "nickname" ]:
                continue
            lines.append (s + "    %s = \"%s\";" % (key, escape_str(val)))
        lines.append (s + "}")
        return ("\n".join (lines))

    def __str__ (self):
        return self._get_str ()

class GroupNode(object):
    def __init__ (self, name):
        self.name = name
        self.commands = []

    def add_command (self, command):
        command.attributes["group"] = self.name
        self.commands.append (command)

    def __str__ (self):
        if self.name == "":
            val = "\n".join([cmd._get_str(0) for cmd in self.commands])
        else:
            val = "group \"%s\" {\n" % self.name
            val += "\n".join([cmd._get_str(1) for cmd in self.commands])
            val = val + "\n}\n"
        return val

class StartStopRestartActionNode(object):
    def __init__(self, action_type, ident_type, ident, wait_status):
        assert action_type in ["start", "stop", "restart"]
        assert ident_type in [ "everything", "group", "cmd" ]
        self.action_type = action_type
        self.ident_type = ident_type
        self.wait_status = wait_status
        if self.ident_type == "everything":
            self.ident = None
        else:
            self.ident = ident
            assert self.ident is not None

    def __str__(self):
        if self.wait_status is not None:
            return "%s %s \"%s\" wait \"%s\";" % (self.action_type,
                    self.ident_type, escape_str(self.ident), self.wait_status)
        elif self.ident_type == "everything":
            return "%s %s;" % (self.action_type, self.ident_type)
        else:
            return "%s %s \"%s\";" % \
                    (self.action_type, self.ident_type, escape_str(self.ident))

class WaitMsActionNode(object):
    def __init__(self, delay_ms):
        self.delay_ms = delay_ms
        self.action_type = "wait_ms"

    def __str__(self):
        return "wait ms %d;" % self.delay_ms

class WaitStatusActionNode(object):
    def __init__(self, ident_type, ident, wait_status):
        self.ident_type = ident_type
        self.ident = ident
        self.wait_status = wait_status
        self.action_type = "wait_status"

    def __str__(self):
        return "wait %s \"%s\" status \"%s\";" % \
                (self.ident_type, escape_str(self.ident), self.wait_status)

class ScriptNode(object):
    def __init__(self, name):
        self.name = name
        self.actions = []

    def add_action(self, action):
        self.actions.append(action)

    def __str__(self):
        val = "script \"%s\" {" % escape_str(self.name)
        for action in self.actions:
            val = val + "\n    " + str(action)
        val = val + "\n}\n"
        return val

class ConfigNode(object):
    def __init__ (self):
        self.groups = {}
        self.scripts = {}
        self.add_group (GroupNode (""))

    def has_group (self, group_name):
        return group_name in self.groups

    def get_group (self, group_name):
        return self.groups[group_name]

    def add_group (self, group):
        assert group.name not in self.groups
        self.groups[group.name] = group

    def add_script (self, script):
        assert script.name not in self.scripts
        self.scripts[script.name] = script

    def add_command (self, command):
        none_group = self.groups[""]
        none_group.add_command (command)

    def __str__ (self):
        val = ""
        groups = sorted(self.groups.values(), key=lambda g: g.name.lower())
        val += "\n".join ([str (group) for group in groups ])
        val += "\n"

        scripts = sorted(self.scripts.values(), key=lambda s: s.name.lower())
        val += "\n".join ([str (script) for script in scripts])
        return val

class ParseError (ValueError):
    def __init__ (self, tokenizer, token, msg):
        self.lineno = tokenizer.line_num
        self.offset = tokenizer.line_pos
        self.text = tokenizer.line_buf
        self.token = token
        self.msg = msg

    def __str__ (self):
        ntabs = self.text.count ("\t")
        s = """%s

line %d col %s token %s
%s
""" % (self.msg, self.lineno, self.offset, self.token.val, self.text)
        s += " " * (self.offset-1 - \
                len (self.token.val) - ntabs) + "\t" * ntabs + "^"
        return s

class Parser:
    def __init__ (self):
        self.tokenizer = None
        self._cur_tok = None
        self._next_tok = None

    def _get_token (self):
        self._cur_tok = self._next_tok
        self._next_tok = self.tokenizer.next_token ()
        while self._next_tok.type == TokComment:
            self._next_tok = self.tokenizer.next_token ()
        return self._cur_tok

    def _eat_token (self, tok_type):
        if self._next_tok and self._next_tok.type == tok_type:
            self._get_token ()
            return True
        return False

    def _fail (self, msg):
        raise ParseError (self.tokenizer, self._cur_tok, msg)

    def _eat_token_or_fail(self, tok_type, err_msg):
        if not self._eat_token(tok_type):
            self._fail(err_msg)
        return self._cur_tok.val

    def _expect_identifier(self, identifier, err_msg = None):
        if err_msg is None:
            err_msg = "Expected %s" % identifier
        self._eat_token_or_fail(TokIdentifier, err_msg)
        if self._cur_tok.val != identifier:
            self._fail(err_msg)

    def _parse_identifier_one_of(self, valid_identifiers):
        err_msg = "Expected one of %s" % str(valid_identifiers)
        self._eat_token_or_fail(TokIdentifier, err_msg)
        result = self._cur_tok.val
        if result not in valid_identifiers:
            self._fail(err_msg)
        return result

    def _parse_string_or_fail(self):
        self._eat_token_or_fail(TokString, "Expected string literal")
        return self._cur_tok.val

    def _parse_command_param_list (self, cmd):
        if not self._eat_token (TokIdentifier):
            return
        attrib_name = self._cur_tok.val
        if attrib_name not in [ "exec", "host", "nickname", "auto_respawn", "group" ]:
            self._fail("Unrecognized attribute %s" % attrib_name)

        self._eat_token_or_fail(TokAssign, "Expected '='")
        attrib_val = self._parse_string_or_fail()
        self._eat_token_or_fail(TokEndStatement, "Expected ';'")
        if attrib_name == "nickname" and len(cmd.attributes["nickname"]):
            self._fail("Command already has a nickname %s" % \
                cmd.attributes["nickname"])
        cmd.attributes[attrib_name] = attrib_val

        return self._parse_command_param_list (cmd)

    def _parse_command (self):
        cmd = CommandNode ()
        if self._eat_token(TokString):
            cmd.attributes["nickname"] = self._cur_tok.val
        self._eat_token_or_fail (TokOpenStruct, "Expected '{'")
        self._parse_command_param_list (cmd)
        self._eat_token_or_fail (TokCloseStruct, "Expected '}'")
        if not cmd.attributes["exec"]:
            self._fail ("Invalid command defined -- no executable specified")
        return cmd

    def _parse_command_list (self):
        cmds = []
        while self._eat_token (TokIdentifier) and self._cur_tok.val == "cmd":
            cmds.append (self._parse_command ())
        return cmds

    def _parse_group (self):
        self._eat_token_or_fail (TokString, "Expected group name string")
        name = self._cur_tok.val
        group = GroupNode (name)
        self._eat_token_or_fail (TokOpenStruct, "Expected '{'")
        for cmd in self._parse_command_list ():
            group.add_command (cmd)
        self._eat_token_or_fail(TokCloseStruct, "Expected '}'")
        return group

    def _parse_start_stop_restart_action(self, action_type):
        valid_ident_types = [ "everything", "cmd", "group" ]
        ident_type = self._parse_identifier_one_of(valid_ident_types)
        ident = None
        if ident_type != "everything":
            ident = self._parse_string_or_fail()
        if self._eat_token(TokEndStatement):
            return StartStopRestartActionNode(action_type, ident_type, ident,
                    None)
        self._expect_identifier("wait", "Expected ';' or 'wait'")
        wait_status = self._parse_string_or_fail()
        self._eat_token_or_fail(TokEndStatement, "Expected ';'")
        return StartStopRestartActionNode(action_type, ident_type, ident,
                wait_status)

    def _parse_wait_action(self):
        wait_type = self._parse_identifier_one_of(["ms", "cmd", "group"])
        if wait_type == "ms":
            err_msg = "Expected integer constant"
            delay_ms = int(self._eat_token_or_fail(TokInteger, err_msg))
            self._eat_token_or_fail(TokEndStatement, "Expected ';'")
            return WaitMsActionNode(delay_ms)
        else:
            ident = self._parse_string_or_fail()
            self._expect_identifier("status")
            wait_status = self._parse_string_or_fail()
            self._eat_token_or_fail(TokEndStatement, "Expected ';'")
            return WaitStatusActionNode(wait_type, ident, wait_status)

    def _parse_script_action_list(self):
        actions = []
        while self._eat_token(TokIdentifier):
            action_type = self._cur_tok.val
            if action_type in [ "start", "stop", "restart" ]:
                actions.append(self._parse_start_stop_restart_action(action_type))
            elif action_type == "wait":
                actions.append(self._parse_wait_action())
            else:
                self._fail("Unexpected token %s" % action_type)
        return actions

    def _parse_script(self):
        self._eat_token_or_fail(TokString, "expected script name")
        name = self._cur_tok.val
        node = ScriptNode(name)
        self._eat_token_or_fail (TokOpenStruct, "Expected '{'")
        for action in self._parse_script_action_list():
            node.add_action(action)
        self._eat_token_or_fail(TokCloseStruct, "Expected '}'")
        return node

    def _parse_listdecl (self, node):
        if self._eat_token (TokEOF):
            return node

        if not self._eat_token (TokIdentifier) or \
                self._cur_tok.val not in [ "cmd", "group", "script" ]:
            self._fail ("Expected 'cmd', 'group', or 'script'")

        if self._cur_tok.val == "cmd":
            node.add_command (self._parse_command ())

        if self._cur_tok.val == "group":
            node.add_group (self._parse_group ())

        if self._cur_tok.val == "script":
            node.add_script(self._parse_script())

        return self._parse_listdecl (node)

    def parse (self, f):
        self.tokenizer = Tokenizer (f)
        self._cur_tok = None
        self._next_tok = None
        self._get_token ()
        return self._parse_listdecl (ConfigNode ())

def config_from_filename (fname):
    return Parser ().parse (file (fname))

if __name__ == "__main__":
    import sys
    try:
        fname = sys.argv[1]
    except IndexError:
        print "usage: sheriff_config.py <fname>"
        sys.exit (1)

    print config_from_filename (fname)
