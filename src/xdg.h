/* See LICENSE file for copyright and license details. */
#ifndef XDG_H
#define XDG_H

#include <stdbool.h>

#include "list.h"
struct path;

bool xdg_get_config_home(struct path* path);
list_t *xdg_get_config_dirs(bool include_config_home);

#endif
