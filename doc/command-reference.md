Overview
========
Each command consists of a command name and an optional parameter for the
command separated by one or more whitespace characters.

White space before the command name is ignored. The optional parameter starts
with first non-whitespace character after the command and goes to the end of
the line. So all further whitespace characters are included in the parameter.
Some command might split this parameter again into several subparameters.

The table below shows all supported commands:

| Command            | Parameter           | Parameter mandatory |
| ------------------ | ------------------- | ------------------- |
| navigate\_up       | none                | -                   |
| navigate\_down     | none                | -                   |
| navigate\_pageup   | none                | -                   |
| navigate\_pagedown | none                | -                   |
| navigate\_left     | none                | -                   |
| navigate\_right    | none                | -                   |
| navigate\_first    | none                | -                   |
| navigate\_last     | none                | -                   |
| togglemark         | none                | -                   |
| invert\_marks      | none                | -                   |
| mark               | filename regex      | yes                 |
| unmark             | filename regex      | yes                 |
| cd                 | target directory    | yes                 |
| invoke\_handler    | handler script name | yes                 |
| yank               | none                | -                   |
| quit               | none                | -                   |
| mkdir              | directory to create | yes                 |
| cmdline            | command             | no                  |
| rename             | new filename        | yes                 |
| search             | filename regex      | yes                 |
| search\_reverse    | filename regex      | yes                 |
| search\_next       | none                | -                   |
| filter             | filename regex      | no                  |
| map                | add key binding     | yes                 |

Command description
===================

navigate\_up
------------
**short description**: move one file up
**parameter**: none

navigate\_down
--------------
**short description**: move one file down
**parameter**: none

navigate\_pageup
----------------
**short description**: move to the top of the screen or one page up
**parameter**: none

navigate\_pagedown
------------------
**short description**: move to the bottom of the screen or one page down
**parameter**: none

navigate\_left
---------------
**short description**: enters parent directory
**parameter**: none

navigate\_right
---------------
**short description**: enters selected directory or opens files
**parameter**: none

If the currently selected file is a directory, dfm enters this directory. If it
is a file, it either opens this file, or, if multiple files are marked, it
opens the marked files. The files are open by using the invoke\_handler command
with the "open" parameter on each file.

navigate\_first
---------------
**short description**: goes to first file in the file list
**parameter**: none

navigate\_last
--------------
**short description**: goes to last file in the file list
**parameter**: none

togglemark
----------
**short description**: inverts the mark on the currently selected file
**parameter**: none

invert\_marks
-------------
**short description**: inverts the mark on all files
**parameter**: none

mark
----
**short description**: sets a mark on the currently selected file
**parameter**: none

unmark
------
**short description**: unsets a mark on the currently selected file
**parameter**: none

cd
--
**short description**: changes the current directory
**parameter**: path

Only absolute paths and ~ are currently supported as a usable parameter.

invoke\_handler
---------------
**short description**: invokes an external script
**parameter**: filename

This command invokes an external script/program to execute certain file actions like open, copy, move and delete. By default the handler scripts are located system wide in /etc/xdg/dfm/handlers or custom for a user in ~/.config/dfm/handlers.

The script is given a path as parameter. This path points to a directory containing information about dfm program state. See the following table for the files contained in this directory.

| Filename        | Description                                              |
| --------------- | -------------------------------------------------------- |
| clipboard\_list | NUL character separated list of files in the clipboard   |
| clipboard\_path | path in which the files from clipboard\_list are located |
| cwd             | full path of current working directory                   |
| marked          | NUL character separated list of marked files             |
| selected        | name of the currently selected file                      |

Not all files are always present, so the script has to handle this.

yank
----
**short description**: puts files into the clipboard
**parameter**: none

The yank command puts all currently marked files into the clipboard. If no
files are marked, the currently selected file is put into the clipboard.

quit
-----
**short description**: quits dfm
**parameter**: none

mkdir
-----
**short description**: creates a directory
**parameter**: directory\_name

The mkdir command creates the directory specified by directory\_name.

cmdline
-------
**short description**: opens the command line
**parameter**: [command]

The cmdline command opens the command line. If a command is given, it is
preentered but not executed. This command is intended to be used in key
bindings to either just open the command line or preenter commands like
*rename* or *mkdir* without a parameter, so that you can fill in the blanks
without typing the full command.

rename
------
**short description**: renames selected file
**parameter**: filename

The rename command renames the currently selected file to the given filename.

search
------
**short description**: searches a file in the file list
**parameter**: regular\_expression

The search command jumps to the first file matching the regular expression. it
starts the search at the current cursor position.

search\_reverse
---------------
**short description**: searches a file in the file list backwards
**parameter**: regular\_expression

Same as search, just search backwards.

search\_next
------------
**short description**: repeats previous search
**parameter**: none

Repeats the last search or search\_reverse command.

filter
------
**Short description**: sets or unsets the filter applied to the file list view
**Parameter**: [regular\_expression]

After issuing the filter command, the files in the list view will be filtered
according to the regular expression given as a parameter. Only files matching
will be displayed. If no parameter is given, the previously set filter will be
removed. Filters don't stack, so only the last one set is a active.

map
---
**Short description**: binds a key to a command
**Parameter**: key\_specifier command [parameter]

The map command binds a key decribed by key\_specifier to a command. The
command starts with the first non-whitespace character after the keyspecifier.

The key specifier is either a single character or a special key string. The
following key strings are currently supported:
- space
- up
- down
- left
- right
- home
- end
- pageup
- pagedown
- f1 - f10

See default rc file in examples/rc as an example.
