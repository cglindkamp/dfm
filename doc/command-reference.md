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
| sort               | sort mode           | yes                 |
| reload             | none                | -                   |

Command description
===================

navigate\_up
------------
**Purpose**: move one file up  
**Parameter**: none

navigate\_down
--------------
**Purpose**: move one file down  
**Parameter**: none

navigate\_pageup
----------------
**Purpose**: move to the top of the screen or one page up  
**Parameter**: none

navigate\_pagedown
------------------
**Purpose**: move to the bottom of the screen or one page down  
**Parameter**: none

navigate\_left
---------------
**Purpose**: enters parent directory  
**Parameter**: none

navigate\_right
---------------
**Purpose**: enters selected directory or opens files  
**Parameter**: none

If the currently selected file is a directory, dfm enters this directory. If it
is a file, it either opens this file, or, if multiple files are marked, it
opens the marked files. The files are open by using the invoke\_handler command
with the "open" parameter on each file.

navigate\_first
---------------
**Purpose**: goes to first file in the file list  
**Parameter**: none

navigate\_last
--------------
**Purpose**: goes to last file in the file list  
**Parameter**: none

togglemark
----------
**Purpose**: inverts the mark on the currently selected file  
**Parameter**: none

invert\_marks
-------------
**Purpose**: inverts the mark on all files  
**Parameter**: none

mark
----
**Purpose**: sets a mark on the currently selected file  
**Parameter**: none

unmark
------
**Purpose**: unsets a mark on the currently selected file  
**Parameter**: none

cd
--
**Purpose**: changes the current directory  
**Parameter**: path

Only absolute paths and ~ are currently supported as a usable parameter.

invoke\_handler
---------------
**Purpose**: invokes an external script  
**Parameter**: filename

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
**Purpose**: puts files into the clipboard  
**Parameter**: none

The yank command puts all currently marked files into the clipboard. If no
files are marked, the currently selected file is put into the clipboard.

quit
-----
**Purpose**: quits dfm  
**Parameter**: none

mkdir
-----
**Purpose**: creates a directory  
**Parameter**: directory\_name

The mkdir command creates the directory specified by directory\_name.

cmdline
-------
**Purpose**: opens the command line  
**Parameter**: [command]

The cmdline command opens the command line. If a command is given, it is
preentered but not executed. This command is intended to be used in key
bindings to either just open the command line or preenter commands like
*rename* or *mkdir* without a parameter, so that you can fill in the blanks
without typing the full command.

rename
------
**Purpose**: renames selected file  
**Parameter**: filename

The rename command renames the currently selected file to the given filename.

search
------
**Purpose**: searches a file in the file list  
**Parameter**: regular\_expression

The search command jumps to the first file matching the regular expression. it
starts the search at the current cursor position.

search\_reverse
---------------
**Purpose**: searches a file in the file list backwards  
**Parameter**: regular\_expression

Same as search, just search backwards.

search\_next
------------
**Purpose**: repeats previous search  
**Parameter**: none

Repeats the last search or search\_reverse command.

filter
------
**Purpose**: sets or unsets the filter applied to the file list view  
**Parameter**: [regular\_expression]

After issuing the filter command, the files in the list view will be filtered
according to the regular expression given as a parameter. Only files matching
will be displayed. If no parameter is given, the previously set filter will be
removed. Filters don't stack, so only the last one set is a active.

map
---
**Purpose**: binds a key to a command  
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

sort
----
**Purpose**: sets the sort mode  
**Parameter**: sort\_mode

The sort command set the order, in which the files are displayed. The following
modes are currently supported:

- name+
- name-
- size+
- size-
- mtime+
- mtime-

A trailing '+' denotes an ascending order and a '-' a descending order.

reload
------
**Purpose**: reload the directory contents  
**Parameter**: none

This is mainly needed for network filesystems, where inotify cannot detect
changes on the server or when inotify was not, for whatever reason, compiled
into the kernel.
