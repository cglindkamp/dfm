#ifndef XDG_H
#define XDG_H

#include <stdbool.h>

#include "list.h"
struct path;

bool xdg_get_config_home(struct path* path);
list_t *xdg_get_config_dirs();

#endif
