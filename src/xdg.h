/* See LICENSE file for copyright and license details. */
#ifndef XDG_H
#define XDG_H

#include <stdbool.h>

#include "list.h"
struct path;

int xdg_get_config_home(struct path **path) __attribute__((warn_unused_result));
struct list *xdg_get_config_dirs(bool include_config_home) __attribute__((warn_unused_result));

#endif
