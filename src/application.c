/* See LICENSE file for copyright and license details. */
#include "application.h"

#include "command.h"
#include "dict.h"
#include "keymap.h"
#include "list.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wchar.h>

static void save_current_position(struct application *app)
{
	if(listmodel_count(&app->model.listmodel) != 0) {
		free(dict_get(app->stored_positions, path_tocstr(&app->cwd)));

		size_t index = listview_getindex(&app->view);
		const char *filename = dirmodel_getfilename(&app->model, index);

		char *filename_copy = strdup(filename);
		if(filename_copy == NULL)
			return;
		if(!dict_set(app->stored_positions, path_tocstr(&app->cwd), filename_copy))
			free(filename_copy);
	}
}

static void select_filename(struct application *app, const char *filename)
{
	size_t index;

	dirmodel_get_index(&app->model, filename, &index);
	if(index == listmodel_count(&app->model.listmodel))
		index--;
	listview_setindex(&app->view, index);
}

static void select_stored_position(struct application *app, const char *oldfilename)
{
	const char *filename;

	if(oldfilename)
		filename = oldfilename;
	else
		filename = dict_get(app->stored_positions, path_tocstr(&app->cwd));

	if(filename)
		select_filename(app, filename);
}

static void display_current_path(struct application *app)
{
	wmove(app->status, 0, 0);
	wclear(app->status);
	wprintw(app->status, "%s", path_tocstr(&app->cwd));
	wrefresh(app->status);
}

static bool enter_directory(struct application *app, const char *oldpathname)
{
	while(1) {
		const char *cwd = path_tocstr(&app->cwd);

		if(app->inotify_fd != -1) {
			if(app->inotify_watch != -1)
				inotify_rm_watch(app->inotify_fd, app->inotify_watch);

			app->inotify_watch = inotify_add_watch(app->inotify_fd, cwd,
				IN_CREATE |
				IN_DELETE |
				IN_MOVED_FROM |
				IN_MOVED_TO |
				IN_MODIFY |
				IN_EXCL_UNLINK
				);
		}

		if(chdir(cwd) == 0 && dirmodel_change_directory(&app->model, cwd))
			break;

		if(strcmp(cwd, "/") == 0) {
			puts("Cannot even open \"/\", exiting");
			app->running = false;
			return false;
		}
		path_remove_component(&app->cwd, &oldpathname);
	}

	select_stored_position(app, oldpathname);
	display_current_path(app);

	return true;
}


static void command_invoke_handler(struct application *app, const char *handler_name)
{
	struct path *handler_path = determine_usable_config_file(PROJECT, "handlers", handler_name, X_OK);
	if(handler_path == NULL)
		return;

	char path_template[] = "/tmp/"PROJECT".XXXXXX";
	char *path = mkdtemp(path_template);
	if(path == NULL)
		goto err_dir;

	int dir_fd = open(path, O_RDONLY);
	if(dir_fd < 0)
		goto err_dircreated;

	if(listmodel_count(&app->model.listmodel) > 0) {
		size_t index = listview_getindex(&app->view);
		const char *selected = dirmodel_getfilename(&app->model, index);
		if(!dump_string_to_file(dir_fd, "selected", selected))
			goto err_dircreated;
	}

	const struct list *list;
	int ret = dirmodel_getmarkedfilenames(&app->model, &list);
	if(ret == 0) {
		bool ret = dump_filelist_to_file(dir_fd, "marked", list);
		list_delete(list, free);
		if(!ret)
			goto err_dircreated;
	} else if(ret == ENOMEM) {
		goto err_dircreated;
	}

	if(!dump_string_to_file(dir_fd, "cwd", path_tocstr(&app->cwd)))
		goto err_dircreated;

	if(!clipboard_dump_contents_to_directory(&app->clipboard, dir_fd))
		goto err_dircreated;

	char * const args[] = {
		"open",
		path,
		NULL
	};
	pid_t pid;
	int status;
	bool foreground = run_in_foreground();

	if(foreground) {
		endwin();
		if(processmanager_spawn(&app->pm, path_tocstr(handler_path), args, path, foreground, &pid) == 0) {
			processmanager_waitpid(&app->pm, pid, &status);
			doupdate();
			goto success;
		} else
			doupdate();
	} else if(processmanager_spawn(&app->pm, path_tocstr(handler_path), args, path, foreground, &pid) == 0)
		goto success;

err_dircreated:
	remove_directory_recursively(path);
success:
	close(dir_fd);
err_dir:
	path_delete(handler_path);
}

static void command_navigate_up(struct application *app, const char *unused)
{
	(void)unused;
	listview_up(&app->view);
}

static void command_navigate_down(struct application *app, const char *unused)
{
	(void)unused;
	listview_down(&app->view);
}

static void command_navigate_pageup(struct application *app, const char *unused)
{
	(void)unused;
	listview_pageup(&app->view);
}

static void command_navigate_pagedown(struct application *app, const char *unused)
{
	(void)unused;
	listview_pagedown(&app->view);
}

static void command_navigate_left(struct application *app, const char *unused)
{
	(void)unused;
	const char *oldpathname = NULL;

	save_current_position(app);

	if(path_remove_component(&app->cwd, &oldpathname))
		enter_directory(app, oldpathname);
}

static void command_navigate_right(struct application *app, const char *unused)
{
	(void)unused;

	if(listmodel_count(&app->model.listmodel) == 0)
		return;

	save_current_position(app);

	size_t index = listview_getindex(&app->view);
	if(dirmodel_isdir(&app->model, index)) {
		const char *filename = dirmodel_getfilename(&app->model, index);
		if(!path_add_component(&app->cwd, filename))
			return;
		enter_directory(app, NULL);
	} else {
		command_invoke_handler(app, "open");
	}
}

static void command_navigate_first(struct application *app, const char *unused)
{
	(void)unused;
	listview_setindex(&app->view, 0);
}

static void command_navigate_last(struct application *app, const char *unused)
{
	(void)unused;
	size_t count = listmodel_count(&app->model.listmodel);

	if(count > 0)
		listview_setindex(&app->view, count - 1);
}

static void command_change_directory(struct application *app, const char *path)
{
	save_current_position(app);

	if(path_set_from_string(&app->cwd, path) == 0)
		enter_directory(app, NULL);
}

static void command_togglemark(struct application *app, const char *unused)
{
	(void)unused;
	if(listmodel_count(&app->model.listmodel) == 0)
		return;

	size_t index = listview_getindex(&app->view);
	listmodel_setmark(&app->model.listmodel, index, !listmodel_ismarked(&app->model.listmodel, index));
	listview_down(&app->view);
}

static void command_invert_marks(struct application *app, const char *unused)
{
	(void)unused;
	size_t count = listmodel_count(&app->model.listmodel);

	if(count > 0) {
		for(size_t i = 0; i < count; i++)
			listmodel_setmark(&app->model.listmodel, i, !listmodel_ismarked(&app->model.listmodel, i));
	}
}

static void command_mark(struct application *app, const char *regex)
{
	dirmodel_regex_setmark(&app->model, regex, true);
}

static void command_unmark(struct application *app, const char *regex)
{
	dirmodel_regex_setmark(&app->model, regex, false);
}

static void command_yank(struct application *app, const char *unused)
{
	(void)unused;
	const struct list *list = NULL;

	int ret = dirmodel_getmarkedfilenames(&app->model, &list);
	char *cwd_copy = strdup(path_tocstr(&app->cwd));

	if(ret == ENOMEM)
		goto err;

	if(ret == 0) {
		if(cwd_copy == NULL)
			goto err;
	} else if(ret == ENOENT) {
		if(listmodel_count(&app->model.listmodel) == 0)
			goto err;

		struct list *selectedlist = list_new(1);
		if(selectedlist == NULL)
			goto err;
		list = selectedlist;

		size_t index = listview_getindex(&app->view);
		char *filename = strdup(dirmodel_getfilename(&app->model, index));
		if(filename == NULL)
			goto err;

		if(!list_append(selectedlist, filename)) {
			free(filename);
			goto err;
		}
	}
	clipboard_set_contents(&app->clipboard, cwd_copy, list);
	return;
err:
	free(cwd_copy);
	list_delete(list, free);
	clipboard_set_contents(&app->clipboard, NULL, NULL);
}

static void command_quit(struct application *app, const char *unused)
{
	(void)unused;
	app->running = false;
}

static void command_mkdir(struct application *app, const char *dirname)
{
	(void)app;
	mkdir(dirname, 0777);
}

static void command_cmdline(struct application *app, const char *command)
{
	app->mode = MODE_COMMAND;
	curs_set(1);
	commandline_start(&app->commandline, L':');

	if(command != NULL) {
		size_t length = mbstowcs(NULL, command, 0);
		if(length != (size_t)-1) {
			wchar_t buffer[length + 1];
			mbstowcs(buffer, command, length);

			for(size_t i = 0; i < length; i++)
				commandline_handlekey(&app->commandline, buffer[i], false);
		}
	}
}

static void command_rename(struct application *app, const char *newfilename)
{
	if(listmodel_count(&app->model.listmodel) == 0)
		return;

	size_t index = listview_getindex(&app->view);
	const char *filename = dirmodel_getfilename(&app->model, index);

	if(rename(filename, newfilename) == 0) {
		dirmodel_notify_file_added_or_changed(&app->model, newfilename);
		select_filename(app, newfilename);
	}
}

static void command_search_next(struct application *app, const char *unused)
{
	(void)unused;
	if(!app->lastsearch_regex)
		return;

	size_t index = listview_getindex(&app->view);

	index = dirmodel_regex_getnext(&app->model, app->lastsearch_regex, index, app->lastsearch_direction);

	listview_setindex(&app->view, index);
}

static void search(struct application *app, const char *regex, int direction)
{
	free((void*)app->lastsearch_regex);
	app->lastsearch_regex = strdup(regex);
	app->lastsearch_direction = direction;

	command_search_next(app, NULL);
}

static void command_search(struct application *app, const char *regex)
{
	search(app, regex, 1);
}

static void command_search_reverse(struct application *app, const char *regex)
{
	search(app, regex, -1);
}

static void command_filter(struct application *app, const char *regex)
{
	dirmodel_setfilter(&app->model, regex);
	enter_directory(app, NULL);
}

struct command_map application_command_map[] = {
	{ "navigate_up", command_navigate_up, false },
	{ "navigate_down", command_navigate_down, false },
	{ "navigate_pageup", command_navigate_pageup, false },
	{ "navigate_pagedown", command_navigate_pagedown, false },
	{ "navigate_left", command_navigate_left, false },
	{ "navigate_right", command_navigate_right, false },
	{ "navigate_first", command_navigate_first, false },
	{ "navigate_last", command_navigate_last, false },
	{ "togglemark", command_togglemark, false },
	{ "invert_marks", command_invert_marks, false },
	{ "mark", command_mark, true },
	{ "unmark", command_unmark, true },
	{ "cd", command_change_directory, true },
	{ "invoke_handler", command_invoke_handler, true },
	{ "yank", command_yank, false },
	{ "quit", command_quit, false },
	{ "mkdir", command_mkdir, true },
	{ "cmdline", command_cmdline, false },
	{ "rename", command_rename, true },
	{ "search", command_search, true },
	{ "search_next", command_search_next, false },
	{ "search_reverse", command_search_reverse, true },
	{ "filter", command_filter, false },
	{ NULL, NULL, false },
};

static void handle_stdin(struct application *app)
{
	wint_t key;
	int ret;

	ret = wget_wch(app->status, &key);
	if(ret == ERR)
		return;

	if(ret != KEY_CODE_YES && key == 27) {
		app->mode = MODE_NORMAL;
		curs_set(0);
		display_current_path(app);
		return;
	}
	if(app->mode == MODE_COMMAND) {
		if(ret != KEY_CODE_YES && key == L'\n') {
			app->mode = MODE_NORMAL;
			curs_set(0);
			display_current_path(app);

			const wchar_t *wcommand = commandline_getcommand(&app->commandline);
			char command[wcstombs(NULL, wcommand, 0) + 1];
			wcstombs(command, wcommand, sizeof(command));
			command_execute(command, app, application_command_map);
			commandline_history_add(&app->commandline, wcsdup(wcommand));
		} else if(ret != KEY_CODE_YES && key == L'\t') {
		} else
			commandline_handlekey(&app->commandline, key, ret == KEY_CODE_YES ? true : false);

	} else
		keymap_handlekey(app->keymap, app, key, ret == KEY_CODE_YES ? true : false, application_command_map);
}

static void handle_signal(struct application *app)
{
	struct signalfd_siginfo info;
	int status;

	if(read(app->signal_fd, &info, sizeof(info)) != sizeof(info))
		return;

	switch(info.ssi_signo) {
	case SIGWINCH:
		endwin();
		doupdate();
		mvwin(app->status, LINES - 1, 0);
		wresize(app->status, 1, COLS);
		listview_resize(&app->view, COLS, LINES - 1);
		commandline_resize(&app->commandline, 0, LINES - 1, COLS);
		if(app->mode == MODE_NORMAL)
			display_current_path(app);
		break;
	case SIGCHLD:
		if(info.ssi_code == CLD_EXITED ||
		   info.ssi_code == CLD_KILLED ||
		   info.ssi_code == CLD_DUMPED) {
			processmanager_waitpid(&app->pm, info.ssi_pid, &status);
		}
		break;
	}
}

static void handle_inotify(struct application *app)
{
	char buf[4096]
		__attribute__((aligned(__alignof(struct inotify_event))));
	char *ptr;
	const struct inotify_event *event;
	ssize_t len;

	len = read(app->inotify_fd, &buf, sizeof(buf));
	if(len <= 0)
		return;

	for(ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {
		event = (const struct inotify_event *)ptr;

		if(event->wd != app->inotify_watch)
			continue;

		if(event->mask & (IN_DELETE | IN_MOVED_FROM)) {
			dirmodel_notify_file_deleted(&app->model, event->name);
		} else if(event->mask & (IN_CREATE | IN_MOVED_TO | IN_MODIFY)) {
			(void)dirmodel_notify_file_added_or_changed(&app->model, event->name);
		}
	}
	if(app->mode == MODE_COMMAND)
		commandline_updatecursor(&app->commandline);
}

void application_run(struct application *app)
{
	struct pollfd pollfds[3] = {
		{ .events = POLLIN, .fd = 0, },
		{ .events = POLLIN, .fd = app->signal_fd, },
		{ .events = POLLIN, .fd = app->inotify_fd, },
	};
	int nfds = app->inotify_fd == -1 ? 2 : 3;

	app->running = true;
	while(app->running) {
		int ret = poll(pollfds, nfds, -1);
		if(ret > 0) {
			if(pollfds[0].revents & POLLIN)
				handle_stdin(app);
			if(pollfds[1].revents & POLLIN)
				handle_signal(app);
			if(pollfds[2].revents & POLLIN)
				handle_inotify(app);
		}
	}
}


static int create_signalfd()
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGWINCH);
	sigaddset(&sigset, SIGCHLD);
	sigprocmask(SIG_BLOCK, &sigset, NULL);

	return signalfd(-1, &sigset, SFD_CLOEXEC);
}

static bool load_keymap(struct application *app)
{
	struct path *keymap_path = determine_usable_config_file(PROJECT, NULL, "keymap", R_OK);
	if(keymap_path == NULL) {
		app->keymap = NULL;
		return false;
	}

	if(keymap_newfromfile(&app->keymap, path_tocstr(keymap_path), application_command_map) != 0) {
		app->keymap = NULL;
		path_delete(keymap_path);
		return false;
	}

	path_delete(keymap_path);
	return true;
}

bool application_init(struct application *app)
{
	bool ret = true;

	app->mode = MODE_NORMAL;
	app->lastsearch_regex = NULL;
	curs_set(0);

	processmanager_init(&app->pm);

	const char *shared_clipboard_path = getenv("DFM_CLIPBOARD_DIR");
	if(shared_clipboard_path) {
		if(access(shared_clipboard_path, R_OK | W_OK | X_OK) != 0)
			shared_clipboard_path = NULL;
	}
	clipboard_init(&app->clipboard, shared_clipboard_path);

	app->inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
	app->inotify_watch = -1;

	dirmodel_init(&app->model);

	app->stored_positions = dict_new();
	if(app->stored_positions == NULL)
		ret = false;

	app->status = newwin(1, COLS, LINES - 1, 0);
	if(app->status != NULL) {
		keypad(app->status, TRUE);
		nodelay(app->status, TRUE);
	} else
		ret = false;

	if(!commandline_init(&app->commandline, 0, LINES - 1, COLS))
		ret = false;

	if(!path_init(&app->cwd, PATH_MAX))
		ret = false;

	if(!listview_init(&app->view, &app->model.listmodel, 0, 0, COLS, LINES - 1))
		ret = false;

	if(!load_keymap(app))
		ret = false;

	if(ret == false)
		return false;

	if(!path_set_to_current_working_directory(&app->cwd))
		return false;

	if(!enter_directory(app, NULL))
		return false;

	app->signal_fd = create_signalfd();
	if(app->signal_fd == -1)
		return false;

	return true;
}

void application_destroy(struct application *app)
{
	free((void*)app->lastsearch_regex);
	keymap_delete(app->keymap);
	listview_destroy(&app->view);
	path_destroy(&app->cwd);
	commandline_destroy(&app->commandline);
	if(app->status)
		delwin(app->status);
	dict_delete(app->stored_positions, true);
	dirmodel_destroy(&app->model);
	clipboard_destroy(&app->clipboard);
	processmanager_destroy(&app->pm);
}
