#include "autoly.h"
#include "toml.h"
#include <stdio.h> // for file manipulation
#include <X11/Xutil.h> //for XClassHint

// cheap way of switching on string
#define IF_KEY(c) if (!strcmp(keyval->key->str, c))
// removes -Wunused-result warnings in GCC
# define ignore_value(x) \
    (__extension__ ({ __typeof__ (x) __x = (x); (void) __x; }))

void save_mode() {
	wm_mode = SAVE;
}

void new_layout(const char *hotkey) {
	FILE *fp;
	fp = fopen("/home/theo/dev/autoly/layouts.toml", "a");
	fprintf(fp, "[%s]\n", hotkey);
	fclose(fp);
}

// if a layout has already been defined for
// the given hotkey, remove it from layouts.toml
void remove_existing_layout(const char *hotkey) {

	FILE *fp;
	fp = fopen("/home/theo/dev/autoly/layouts.toml", "a");

	char buffer[300];
	int start_line = -1;

	// for each line of layouts.toml
	for (int line = 0; fgets(buffer, sizeof(buffer), fp); line++) {

		// check if current line is equal to [<hotkey>], indicating start of
		// an existing layout for the given hotkey
		char hotkey_string[20];
		snprintf(hotkey_string, sizeof(hotkey_string), "[%s]", hotkey);
		if (!strcmp(buffer, hotkey_string)) {
			start_line = line;
		}
		// TODO free hotkey_string?
	}

	// if no existing preset found, return
	if (start_line == -1) {
		return;
	}

	rewind(fp);
	FILE *tmp_file = fopen("/home/theo/dev/autoly/tmp.toml", "a");

	// write other presets until the preset-to-remove to a tmp file
	for (int i = 0; i < start_line; i++) {
		ignore_value(fgets(buffer, sizeof(buffer), fp));
		fprintf(tmp_file, "%s\n", buffer);
	}

	// until a new preset or EOF reached, move through lines without writing
	// them to the tmp file (crude form of line deletion)
	// this works as all lines except new preset declarations start with a tab
	while (strncmp(buffer, "[", 1) || fgets(buffer, sizeof(buffer), fp)) {
		ignore_value(fgets(buffer, sizeof(buffer), fp));
	}


	// copy the rest of the presets to the tmp file
	while (fgets(buffer, sizeof(buffer), fp)) {
		fprintf(tmp_file, "%s\n", buffer);
	}

	// replace old presets file with new presets file
	fclose(tmp_file);
	fclose(fp);
	remove("/home/theo/dev/autoly/layouts.toml");
	rename("/home/theo/dev/autoly/tmp.toml", "/home/theo/dev/autoly/layouts.toml");
}

Application *load_application(TomlTable *table) {

	TomlErr err = TOML_ERR_INIT;
	TomlTableIter *it = toml_table_iter_new(table, &err);
	// if error not TOML_OK, it is always NULL
	if (err.code != TOML_OK) {
		debug_log("layout.c: load_application: toml_table_iter_new failed\n");
		toml_clear_err(&err);
		return NULL;
	}


	Application *app = malloc(sizeof(Application));
	app->next = NULL;

	// each value of the keyvals refers to an attribute of the window
	// assign the attribute to the relevant variable
	while (toml_table_iter_has_next(it)) {
		TomlKeyValue *keyval = toml_table_iter_get(it);

		// switch on key of keyval
		IF_KEY("wm_class") {
			app->wm_class = keyval->value->value.string->str;
		} else IF_KEY("width") {
			app->width = keyval->value->value.integer;
		} else IF_KEY("height") {
			app->height = keyval->value->value.integer;
		} else IF_KEY("x") {
			app->x = keyval->value->value.integer;
		} else IF_KEY("y") {
			app->y = keyval->value->value.integer;
		}

		toml_table_iter_next(it);
	}

	debug_log("layout.c: load_application: freeing table iter\n");
	toml_table_iter_free(it);
	return app;
}

// t is the table referring to the workspace (e.g. [d])
Layout *load_layout(TomlTable *table) {

	TomlErr err = TOML_ERR_INIT;
	TomlTableIter *it = toml_table_iter_new(table, &err);
	if (err.code != TOML_OK) {
		debug_log("layout.c: load_layout: toml_table_iter_new failed\n");
		toml_clear_err(&err);
		return NULL;
	}

	// create the head of the application list and save the 
	// start of the list for later
	Application *new_app = malloc(sizeof(Application));
	Application *app_list = new_app;

	// each value of the keyvals is a table referring to one of the 
	// applications in the preset (e.g. [d.window1])
	while (toml_table_iter_has_next(it)) {
		TomlKeyValue *keyval = toml_table_iter_get(it);

		// append application to end of list
		new_app->next = load_application(keyval->value->value.table);
		new_app = new_app->next;

		toml_table_iter_next(it);
	}

	// return the layout
	Layout *layout = malloc(sizeof(Layout));
	// preset->hotkey = hotkey;
	layout->app_list = app_list;
	layout->next = NULL;

	debug_log("layout.c: load_layout: freeing table iter\n");
	toml_table_iter_free(it);

	return layout;
}

void load_layouts() {

	// load layouts.toml into table
	TomlErr err = TOML_ERR_INIT;
	TomlTable *table = toml_load_filename("/home/theo/dev/autoly/layouts.toml", &err);
	if (err.code != TOML_OK) goto cleanuptable;

	TomlTableIter *it = toml_table_iter_new(table, &err);
	if (err.code != TOML_OK) goto cleanupiter;

	// used for appending to list of layouts
	Layout *current_layout = &layouts;

	// for each layout (e.g. [d]) in layouts.toml...
	while (toml_table_iter_has_next(it)) {
		TomlKeyValue *keyval = toml_table_iter_get(it);
		// build the layout structure
		Layout *layout = load_layout(keyval->value->value.table);
		layout->hotkey =  keyval->key->str;

		// append preset to end of list
		current_layout->next = layout;
		current_layout = current_layout->next;

		toml_table_iter_next(it);
	}

	// free all used resources
	cleanupiter:
		debug_log("layout.c: load_layouts: freeing table iter\n");
		toml_table_iter_free(it);

	cleanuptable:
		// TODO toml_table_free(table);
		if (err.code != TOML_OK) {
			fprintf(stderr, "toml: %d: %s\n", err.code, err.message);
			toml_clear_err(&err);
		}

}

// save current layout to specified hotkey
void save_layout(const char *hotkey) {

	remove_existing_layout(hotkey);

	FILE *fp;
	fp = fopen("/home/theo/dev/autoly/layouts.toml", "a");

	// get file ready to enter each window
	fprintf(fp, "[%s]\n", hotkey);

	// get all windows
	Window dummy1, dummy2, *window_list;
	unsigned int num_windows;
	XQueryTree(dpy, DefaultRootWindow(dpy), &dummy1, &dummy2, &window_list, &num_windows);
	
	// write the properties of each mapped windows to presets.toml
	for (int i=0; i<num_windows; i++) {

		XWindowAttributes attr;
		int status = XGetWindowAttributes(dpy, window_list[i], &attr);
		if (!status) goto cleanup;

		// ignore non-mapped windows
		if (attr.map_state != IsViewable) {
			continue;
		}



		char *wm_class = get_wm_class(window_list[i]);

		fprintf(fp, "\t[%s.window%d]\n", hotkey, i+1);
		fprintf(fp, "\t# load_script = \"...\"\n");
		fprintf(fp, "\twm_class = \"%s\"\n", wm_class);
		fprintf(fp, "\twidth = %d\n", attr.width);
		fprintf(fp, "\theight = %d\n", attr.height);
		fprintf(fp, "\tx = %d\n", attr.x);
		fprintf(fp, "\ty = %d\n\n", attr.y);

		if (wm_class != NULL) {
			debug_log("layout.c: save_layout: freeing wm_class\n");
			XFree(wm_class);
		}
	}


cleanup:
	if (window_list != NULL) {
		debug_log("layout.c: save_layout: freeing window_list\n");
		XFree(window_list);
	}
	fclose(fp);
	wm_mode = NORMAL;
}
