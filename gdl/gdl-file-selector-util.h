/*
 * gdl-file-selector-util.h - functions for getting files from a
 * selector
 *
 * Authors:
 *    Jacob Berkman  <jacob@ximian.com>
 *
 * Copyright 2001 Ximian, Inc.
 *
 */

#ifndef _GDL_FILE_SELECTOR_UTIL_H_
#define _GDL_FILE_SELECTOR_UTIL_H_

#include <gtk/gtkwindow.h>

G_BEGIN_DECLS

char  *gdl_file_selector_open       (GtkWindow  *parent,
				       gboolean    enable_vfs,
				       const char *title,
				       const char *mime_types,
				       const char *default_path);

char **gdl_file_selector_open_multi (GtkWindow  *parent,
				       gboolean    enable_vfs,
				       const char *title,
				       const char *mime_types,
				       const char *default_path);

char  *gdl_file_selector_save       (GtkWindow  *parent,
				       gboolean    enable_vfs,
				       const char *title,
				       const char *mime_types,
				       const char *default_path,
				       const char *default_filename);

G_END_DECLS

#endif /* _GDL_FILE_SELECTOR_UTIL_H_ */
