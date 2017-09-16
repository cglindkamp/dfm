Keymap file format
==================
Each line in the keymap file contains one key binding. Empty lines and leading
whitespace is ignored. A binding consists of one key specifier, a command and
an optional parameter for the command each separated by one or more whitespace
characters.

A key specifier is either a single character or a special key string. The
following key strings are currently supported:
- space
- up
- down
- left
- right
- pageup
- pagedown

The table below shows all supported commands:

| Command           | Parameter           |
| ----------------- | ------------------- |
| navigate_up       | none                |
| navigate_down     | none                |
| navigate_pageup   | none                |
| navigate_pagedown | none                |
| navigate_left     | none                |
| navigate_right    | none                |
| mark              | none                |
| change_directory  | target directory    |
| invoke_handler    | handler script name |
| yank              | none                |
| quit              | none                |

The optional parameter starts with first non-whitespace character after the
command and goes to the end of the line. So all further whitespace characters
are included in the parameter.

See default keymap file in examples/keymap as an example.