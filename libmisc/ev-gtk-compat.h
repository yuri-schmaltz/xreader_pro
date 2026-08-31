/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8; c-indent-level: 8 -*- */
/*
 * ev-gtk-compat.h - GTK3/GTK4 compatibility and modernization header for Xreader
 *
 * Copyright (C) 2026 Yuri Schmaltz / xreader fork
 *
 * This header defines abstraction macros and inline helpers to ease the
 * progressive transition from GTK 3.x to GTK 4.x.
 */

#ifndef EV_GTK_COMPAT_H
#define EV_GTK_COMPAT_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Container & Box parenting helpers */
#if GTK_CHECK_VERSION(4, 0, 0)
#define ev_gtk_box_append(box, child) gtk_box_append(GTK_BOX(box), GTK_WIDGET(child))
#define ev_gtk_box_prepend(box, child) gtk_box_prepend(GTK_BOX(box), GTK_WIDGET(child))
#define ev_gtk_widget_destroy(widget) gtk_window_destroy(GTK_WINDOW(widget))
#else
#define ev_gtk_box_append(box, child) gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(child), TRUE, TRUE, 0)
#define ev_gtk_box_prepend(box, child) gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(child), FALSE, FALSE, 0)
#define ev_gtk_widget_destroy(widget) gtk_widget_destroy(GTK_WIDGET(widget))
#endif

/* Asynchronous Dialog Response Helper */
typedef void (*EvDialogResponseFunc) (GtkDialog *dialog, gint response_id, gpointer user_data);

typedef struct {
	EvDialogResponseFunc func;
	gpointer             user_data;
} EvDialogResponseData;

static inline void
_ev_gtk_dialog_response_cb (GtkDialog *dialog, gint response_id, gpointer data)
{
	EvDialogResponseData *d = (EvDialogResponseData *) data;
	if (d && d->func)
		d->func (dialog, response_id, d->user_data);
	g_free (d);
	ev_gtk_widget_destroy (GTK_WIDGET (dialog));
}

static inline void
ev_gtk_dialog_run_async (GtkDialog           *dialog,
                         EvDialogResponseFunc func,
                         gpointer             user_data)
{
	EvDialogResponseData *d = g_new0 (EvDialogResponseData, 1);
	d->func = func;
	d->user_data = user_data;

	g_signal_connect (dialog, "response",
	                  G_CALLBACK (_ev_gtk_dialog_response_cb), d);
	gtk_widget_show (GTK_WIDGET (dialog));
}

/* Cursor helpers */
static inline void
ev_gtk_widget_set_cursor_name (GtkWidget *widget, const gchar *cursor_name)
{
#if GTK_CHECK_VERSION(4, 0, 0)
	gtk_widget_set_cursor_from_name (widget, cursor_name);
#else
	GdkWindow *window = gtk_widget_get_window (widget);
	if (window) {
		GdkDisplay *display = gtk_widget_get_display (widget);
		GdkCursor *cursor = cursor_name ? gdk_cursor_new_from_name (display, cursor_name) : NULL;
		gdk_window_set_cursor (window, cursor);
		if (cursor)
			g_object_unref (cursor);
	}
#endif
}

/* GAction helper: Activate action by name on an action map */
static inline void
ev_g_action_activate (GActionMap  *action_map,
                      const gchar *action_name,
                      GVariant    *parameter)
{
	GAction *action = g_action_map_lookup_action (action_map, action_name);
	if (action)
		g_action_activate (action, parameter);
}

/* Box container helpers -- single-child container.  GTK 3 used
 * gtk_container_add() which was removed in GTK 4 in favour of
 * widget-specific append/prepend.  These shims map to the
 * appropriate API based on the container type at compile time. */
static inline void
ev_gtk_container_add (GtkContainer *container,
                      GtkWidget    *widget)
{
#if GTK_CHECK_VERSION(4, 0, 0)
	/* Best effort: try Box append, fall back to set_child. */
	if (GTK_IS_BOX (container))
		gtk_box_append (GTK_BOX (container), widget);
	else
		gtk_widget_set_child (GTK_WIDGET (container), widget);
#else
	gtk_container_add (container, widget);
#endif
}

/* gtk_box_pack_start with default expand/fill.  The legacy
 * 5-arg form has been removed in GTK 4 in favour of explicit
 * expand/fill properties on the child.  Use ev_gtk_box_append()
 * above for new code; this shim keeps the existing call sites
 * working under both GTK 3 and GTK 4. */
static inline void
ev_gtk_box_pack_start (GtkBox    *box,
                       GtkWidget *child,
                       gboolean   expand,
                       gboolean   fill,
                       guint      padding)
{
#if GTK_CHECK_VERSION(4, 0, 0)
	gtk_box_append (box, child);
	gtk_widget_set_hexpand (child, expand);
	gtk_widget_set_vexpand (child, expand);
	(void) fill;
	(void) padding;
#else
	gtk_box_pack_start (box, child, expand, fill, padding);
#endif
}

/* gtk_widget_show_all() has been removed in GTK 4 (widget tree
 * is shown by default once mapped).  This shim is a no-op on
 * GTK 4 and the original on GTK 3. */
static inline void
ev_gtk_widget_show_all (GtkWidget *widget)
{
#if !GTK_CHECK_VERSION(4, 0, 0)
	if (widget)
		gtk_widget_show_all (widget);
#endif
}

G_END_DECLS

#endif /* EV_GTK_COMPAT_H */
