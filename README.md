Dynamic File Manager [![Build Status](https://api.travis-ci.com/cglindkamp/dfm.svg?branch=master)](https://travis-ci.com/cglindkamp/dfm)
====================
Dynamic File Manager (dfm) is a simple ncurses based file manager. It is
inspired by ranger but tries to keep the user interface much simpler and memory
footprint as well a CPU usage low.

Features
--------
* Uses inotify to always show an accurate view of the current directory
* Delegates file actions (open, copy, move, delete) to external scripts. The
  default scripts use see (run-mailcap), cp, mv and rm to perform the requested
  actions.
* Remembers the last selected file in every directory to quickly navigate to
  previously visited paths
* Filtered view: Show only files/directories matching a regular expression,
  i.e. certain file types. Can also be used to exclude dot-files.
* Shared clipboard: Multiple instances of dfm can share a clipboard if the
  environment variable DFM\_CLIPBOARD\_DIR is set to a writable directory. This
  way you can emulate tabs with screen mutliplexers or mutliple xterns running
  several dfm instances

Installation
------------
Building dfm requires wide-char enabled ncurses. To build just invoke

	make

in the toplevel directory of the project.

To install dfm, just enter

	sudo make install

You might want to edit the Makefile to adjust the installation paths.

Usage
-----
To start dfm, just enter

	dfm

The following table shows the default key bindings. They can be configured
globally in the file /etc/xdg/dfm/rc or for each user in ~/.config/dfm/rc. A
command reference can be found in doc/command-reference.md. Additionally, the
behaviour of the open, copy, move, delete or shell bindings can be customized
through the corresponding handler scripts. See examples/handlers. If you want
to change them, copy the handler scripts to ~/.config/dfm/handlers and edit
them to your liking.

| Key      | Action                                                                               |
| -------- | ------------------------------------------------------------------------------------ |
| Up       | Move up                                                                              |
| Down     | Move down                                                                            |
| PageUp   | Move to the top of the screen or one page up                                         |
| PageDown | Move to the bottom of the screen or one page down                                    |
| Left     | Go to the parent directory                                                           |
| Right    | Enter the selected directory or open the selected/marked file(s)                     |
| Home     | Go to the first file                                                                 |
| End      | Go to the last file                                                                  |
| Space    | Mark/unmark currently selected file                                                  |
| *        | Invert mark status of every file                                                     |
| +        | Mark all files matching the entered regular expression                               |
| -        | Unmark all files matching the entered regular expression                             |
| 1        | Go to home (~) directory                                                             |
| 2        | Go to root (/) directory                                                             |
| r        | Reload directory contents, mainly useful on network filesystems                      |
| e        | Open selected or marked files for editing                                            |
| D        | Delete currently marked files, or when no file is marked the currently selected file |
| p        | Copy files in clipboard to the current directory                                     |
| P        | Move files in clipboard to the current directory                                     |
| s        | Open a shell in the current direcotory                                               |
| y        | Copy currently marked files or the currently selected file to clipboard              |
| q        | Quit dfm                                                                             |
| :        | Open the command line                                                                |
| F6       | Rename currently selected file/directory                                             |
| F7       | Create a new directory                                                               |
| /        | Jump to the next file matching the entered regular expression                        |
| ?        | Jump to the previous file matching the entered regular expression                    |
| n        | Repeat the previous search                                                           |

