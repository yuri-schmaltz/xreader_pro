/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8; c-indent-level: 8 -*- */
/* this file is part of xreader, a mate document viewer
 *
 *  Copyright (C) 2026 Yuri Schmaltz / xreader fork
 *
 * Xreader is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Xreader is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include "ev-tabbed-window.h"

#include <glib/gi18n.h>

#include "ev-tab.h"
#include "ev-document-factory.h"
#include "ev-gtk-compat.h"

struct _EvTabbedWindowPrivate
{
	EvTabManager *tab_manager;
	GtkWidget    *notebook;
	GtkWidget    *main_box;
	GtkWidget    *menubar;
	GtkWidget    *toolbar;
	GtkWidget    *statusbar;
	GtkWidget    *empty_label;  /* shown when no tabs are open */
};

G_DEFINE_TYPE_WITH_PRIVATE (EvTabbedWindow, ev_tabbed_window, GTK_TYPE_APPLICATION_WINDOW)

static gboolean on_key_pressed (GtkEventControllerKey *controller,
                                guint                  keyval,
                                guint                  keycode,
                                GdkModifierType        state,
                                gpointer               user_data);

static void on_page_drop (GtkWidget         *widget,
                          GdkDragContext    *context,
                          gint               x,
                          gint               y,
                          GtkSelectionData  *selection,
                          guint              info,
                          guint              time,
                          gpointer           user_data);

static void on_empty_drop (GtkWidget         *widget,
                           GdkDragContext    *context,
                           gint               x,
                           gint               y,
                           GtkSelectionData  *selection,
                           guint              info,
                           guint              time,
                           gpointer           user_data);

static void
update_tab_bar_visibility (EvTabbedWindow *window)
{
	gboolean visible = ev_tab_manager_get_n_tabs (window->priv->tab_manager) >= 2;
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (window->priv->notebook), visible);
}

static void
update_window_title (EvTabbedWindow *window)
{
	EvTab *active = ev_tab_manager_get_active (window->priv->tab_manager);
	if (active) {
		gchar *title = ev_tab_get_title (active);
		gtk_window_set_title (GTK_WINDOW (window), title);
		g_free (title);
	} else {
		gtk_window_set_title (GTK_WINDOW (window), _("Xreader"));
	}
}

static void
update_empty_state (EvTabbedWindow *window)
{
	gboolean has_tabs = ev_tab_manager_get_n_tabs (window->priv->tab_manager) > 0;
	gtk_widget_set_visible (window->priv->empty_label, !has_tabs);
	gtk_widget_set_visible (window->priv->notebook,    has_tabs);
	gtk_widget_set_sensitive (window->priv->menubar,   has_tabs);
	gtk_widget_set_sensitive (window->priv->toolbar,   has_tabs);
	gtk_widget_set_sensitive (window->priv->statusbar, has_tabs);
}

static GtkWidget *
build_menubar (void)
{
	GtkWidget *menubar = gtk_menu_bar_new ();

	/* File menu (placeholder - full menu integration is a follow-up PR) */
	GtkWidget *file_item = gtk_menu_item_new_with_label (_("File"));
	GtkWidget *file_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_item), file_menu);
	gtk_menu_shell_append (GTK_MENU_SHELL (menubar), file_item);

	GtkWidget *open_item = gtk_menu_item_new_with_label (_("Open..."));
	gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), open_item);

	GtkWidget *close_tab_item = gtk_menu_item_new_with_label (_("Close Tab"));
	gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), close_tab_item);

	GtkWidget *sep = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), sep);

	GtkWidget *quit_item = gtk_menu_item_new_with_label (_("Quit"));
	gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), quit_item);

	gtk_widget_show_all (file_item);

	/* Tabs menu */
	GtkWidget *tabs_item = gtk_menu_item_new_with_label (_("Tabs"));
	GtkWidget *tabs_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (tabs_item), tabs_menu);
	gtk_menu_shell_append (GTK_MENU_SHELL (menubar), tabs_item);

	GtkWidget *next_tab = gtk_menu_item_new_with_label (_("Next Tab"));
	gtk_menu_shell_append (GTK_MENU_SHELL (tabs_menu), next_tab);

	GtkWidget *prev_tab = gtk_menu_item_new_with_label (_("Previous Tab"));
	gtk_menu_shell_append (GTK_MENU_SHELL (tabs_menu), prev_tab);

	gtk_widget_show_all (tabs_item);

	return menubar;
}

static GtkWidget *
build_toolbar (void)
{
	GtkWidget *toolbar = gtk_toolbar_new ();
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
	/* Placeholder for future actions (open, close tab, etc.) */
	return toolbar;
}

static GtkWidget *
build_tab_label (EvTab *tab)
{
	GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
	gchar *title = ev_tab_get_title (tab);
	GtkWidget *label = gtk_label_new (title);
	g_free (title);
	gtk_label_set_max_width_chars (GTK_LABEL (label), 20);
	gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
	ev_gtk_box_append (box, label);

	GtkWidget *close_button = gtk_button_new_from_icon_name (
		"window-close-symbolic", GTK_ICON_SIZE_MENU);
	gtk_button_set_relief (GTK_BUTTON (close_button), GTK_RELIEF_NONE);
	gtk_widget_set_focus_on_click (close_button, FALSE);
	ev_gtk_box_append (box, close_button);

	/* g_object_set_data_full so we can look up the tab from the
	 * close-button signal handler */
	g_object_set_data (G_OBJECT (close_button), "ev-tab-label", box);

	gchar *tooltip = ev_tab_get_tooltip (tab);
	gtk_widget_set_tooltip_text (box, tooltip);
	g_free (tooltip);

	gtk_widget_show_all (box);
	return box;
}

static void
on_close_button_clicked (GtkButton       *button,
                         gpointer         user_data)
{
	GtkWidget *box = g_object_get_data (G_OBJECT (button), "ev-tab-label");
	if (!box)
		return;

	/* Find which tab this close button belongs to by walking
	 * the notebook's children and matching the label widget. */
	EvTabbedWindow *window = EV_TABBED_WINDOW (user_data);
	GtkNotebook *notebook = GTK_NOTEBOOK (window->priv->notebook);

	gint n = gtk_notebook_get_n_pages (notebook);
	gint i;
	for (i = 0; i < n; i++) {
		GtkWidget *page = gtk_notebook_get_nth_page (notebook, i);
		GtkWidget *tab_label = gtk_notebook_get_tab_label (notebook, page);
		if (tab_label == box) {
			EvTab *tab = EV_TAB (page);
			ev_tab_manager_remove_tab (window->priv->tab_manager, tab);
			break;
		}
	}
}

static void
on_tab_changed (EvTab *tab,
                GParamSpec *pspec,
                gpointer user_data)
{
	GtkWidget *tab_label = GTK_WIDGET (user_data);
	GList *children = gtk_container_get_children (GTK_CONTAINER (tab_label));
	if (children) {
		GtkWidget *label = GTK_WIDGET (children->data);
		gchar *title = ev_tab_get_title (tab);
		gtk_label_set_text (GTK_LABEL (label), title);
		g_free (title);
		g_list_free (children);
	}

	gchar *tooltip = ev_tab_get_tooltip (tab);
	gtk_widget_set_tooltip_text (tab_label, tooltip);
	g_free (tooltip);
}

static void
on_tab_added (EvTabManager    *manager,
              EvTab           *tab,
              EvTabbedWindow  *window)
{
	/* Show the EvTab in a new notebook page.  The EvTab
	 * is a GtkBox that contains the view; we add it as
	 * the page widget. */
	gtk_notebook_append_page (GTK_NOTEBOOK (window->priv->notebook),
	                          GTK_WIDGET (tab),
	                          build_tab_label (tab));

	gint page_num = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->priv->notebook)) - 1;
	GtkWidget *tab_label = gtk_notebook_get_tab_label (
		GTK_NOTEBOOK (window->priv->notebook), GTK_WIDGET (tab));

	GList *children = gtk_container_get_children (GTK_CONTAINER (tab_label));
	GtkWidget *close_button = GTK_WIDGET (g_list_nth_data (children, 1));
	g_list_free (children);
	if (close_button) {
		g_signal_connect (close_button, "clicked",
		                  G_CALLBACK (on_close_button_clicked), window);
	}

	/* Update the tab label when the title/tooltip changes. */
	g_signal_connect (tab, "notify::title", G_CALLBACK (on_tab_changed), tab_label);
	g_signal_connect (tab, "notify::tooltip", G_CALLBACK (on_tab_changed), tab_label);

	/* Drag-and-drop: each notebook page is a drop target for files.
	 * Dropping a file on a tab focuses that tab and opens the file
	 * in a new tab (or focuses an already-open tab with the same
	 * file).  The drop happens via the 'page-drop' signal on the
	 * notebook (production-ready since GTK 3.16). */
	gtk_drag_dest_set (GTK_WIDGET (tab), GTK_DEST_DEFAULT_ALL,
	                   NULL, 0, GDK_ACTION_COPY);
	gtk_drag_dest_add_uri_targets (GTK_WIDGET (tab));
	g_signal_connect (tab, "drag-data-received",
	                  G_CALLBACK (on_page_drop), window);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (window->priv->notebook), page_num);
	update_tab_bar_visibility (window);
	update_window_title (window);
	update_empty_state (window);
}

static void
on_tab_removed (EvTabManager    *manager,
                EvTab           *tab,
                EvTabbedWindow  *window)
{
	gint page_num = gtk_notebook_page_num (
		GTK_NOTEBOOK (window->priv->notebook), GTK_WIDGET (tab));
	if (page_num >= 0)
		gtk_notebook_remove_page (
			GTK_NOTEBOOK (window->priv->notebook), page_num);

	update_tab_bar_visibility (window);
	update_window_title (window);
	update_empty_state (window);
}

static void
on_active_changed (EvTabManager    *manager,
                   EvTab           *tab,
                   EvTabbedWindow  *window)
{
	if (!tab) {
		update_window_title (window);
		update_empty_state (window);
		return;
	}

	gint page_num = gtk_notebook_page_num (
		GTK_NOTEBOOK (window->priv->notebook), GTK_WIDGET (tab));
	if (page_num >= 0)
		gtk_notebook_set_current_page (
			GTK_NOTEBOOK (window->priv->notebook), page_num);

	update_window_title (window);
}

static void
ev_tabbed_window_dispose (GObject *object)
{
	EvTabbedWindow *window = EV_TABBED_WINDOW (object);

	g_clear_object (&window->priv->tab_manager);

	G_OBJECT_CLASS (ev_tabbed_window_parent_class)->dispose (object);
}

static void
ev_tabbed_window_init (EvTabbedWindow *window)
{
	window->priv = ev_tabbed_window_get_instance_private (window);

	window->priv->tab_manager = ev_tab_manager_new ();
	g_signal_connect (window->priv->tab_manager, "tab-added",
	                  G_CALLBACK (on_tab_added), window);
	g_signal_connect (window->priv->tab_manager, "tab-removed",
	                  G_CALLBACK (on_tab_removed), window);
	g_signal_connect (window->priv->tab_manager, "active-changed",
	                  G_CALLBACK (on_active_changed), window);

	window->priv->main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add (GTK_CONTAINER (window), window->priv->main_box);

	window->priv->menubar = build_menubar ();
	ev_gtk_box_append (window->priv->main_box, window->priv->menubar);

	window->priv->toolbar = build_toolbar ();
	ev_gtk_box_append (window->priv->main_box, window->priv->toolbar);

	window->priv->notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (window->priv->notebook), FALSE);
	gtk_notebook_set_scrollable (GTK_NOTEBOOK (window->priv->notebook), TRUE);
	ev_gtk_box_append (window->priv->main_box, window->priv->notebook);

	window->priv->empty_label = gtk_label_new (
		_("Open a document to start.  Drag a file here, or use File > Open."));
	gtk_widget_set_vexpand (window->priv->empty_label, TRUE);
	gtk_widget_set_valign (window->priv->empty_label, GTK_ALIGN_CENTER);
	gtk_widget_set_halign (window->priv->empty_label, GTK_ALIGN_CENTER);
	ev_gtk_box_append (window->priv->main_box, window->priv->empty_label);

	/* The empty-state label is also a drop target.  When the
	 * user drags a file to a window with no tabs, the file
	 * is opened in a new tab. */
	gtk_drag_dest_set (window->priv->empty_label,
	                   GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY);
	gtk_drag_dest_add_uri_targets (window->priv->empty_label);
	g_signal_connect (window->priv->empty_label, "drag-data-received",
	                  G_CALLBACK (on_empty_drop), window);

	window->priv->statusbar = gtk_statusbar_new ();
	ev_gtk_box_append (window->priv->main_box, window->priv->statusbar);

	gtk_widget_show_all (window->priv->main_box);
	gtk_widget_show (GTK_WIDGET (window));

	update_empty_state (window);
	gtk_window_set_title (GTK_WINDOW (window), _("Xreader"));
	gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);

	/* Keyboard shortcuts via a GtkEventControllerKey.  Production-ready
	 * approach (works with GTK 3.24+). */
	GtkEventController *key_controller = gtk_event_controller_key_new (GTK_WIDGET (window));
	gtk_event_controller_set_propagation_phase (key_controller,
	                                            GTK_PHASE_BUBBLE);
	g_signal_connect (key_controller, "key-pressed",
	                  G_CALLBACK (on_key_pressed), window);
}

static gboolean
on_key_pressed (GtkEventControllerKey *controller,
                guint                  keyval,
                guint                  keycode,
                GdkModifierType        state,
                gpointer               user_data)
{
	EvTabbedWindow *window = EV_TABBED_WINDOW (user_data);

	/* Ctrl+Tab: next / previous tab (Shift for prev). */
	if (keyval == GDK_KEY_Tab && (state & GDK_CONTROL_MASK)) {
		if (state & GDK_SHIFT_MASK)
			ev_tabbed_window_select_prev_tab (window);
		else
			ev_tabbed_window_select_next_tab (window);
		return TRUE;
	}

	/* Ctrl+PgDn / PgUp: alternative. */
	if ((state & GDK_CONTROL_MASK) &&
	    (keyval == GDK_KEY_Page_Down || keyval == GDK_KEY_KP_Page_Down)) {
		ev_tabbed_window_select_next_tab (window);
		return TRUE;
	}
	if ((state & GDK_CONTROL_MASK) &&
	    (keyval == GDK_KEY_Page_Up || keyval == GDK_KEY_KP_Page_Up)) {
		ev_tabbed_window_select_prev_tab (window);
		return TRUE;
	}

	/* Ctrl+W: close active tab. */
	if (keyval == GDK_KEY_w && (state & GDK_CONTROL_MASK)) {
		ev_tabbed_window_close_active_tab (window);
		return TRUE;
	}

	/* Ctrl+Shift+T: reopen last closed tab. */
	if (keyval == GDK_KEY_t &&
	    (state & GDK_CONTROL_MASK) && (state & GDK_SHIFT_MASK)) {
		ev_tab_manager_reopen_last_closed_tab (window->priv->tab_manager);
		return TRUE;
	}

	return FALSE;
}

static void
ev_tabbed_window_class_init (EvTabbedWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->dispose = ev_tabbed_window_dispose;
}

GtkWidget *
ev_tabbed_window_new (GtkApplication *app)
{
	return g_object_new (EV_TYPE_TABBED_WINDOW, "application", app, NULL);
}

EvTabManager *
ev_tabbed_window_get_tab_manager (EvTabbedWindow *window)
{
	g_return_val_if_fail (EV_IS_TABBED_WINDOW (window), NULL);
	return window->priv->tab_manager;
}

GtkWidget *
ev_tabbed_window_get_active_view (EvTabbedWindow *window)
{
	g_return_val_if_fail (EV_IS_TABBED_WINDOW (window), NULL);
	EvTab *active = ev_tab_manager_get_active (window->priv->tab_manager);
	if (!active)
		return NULL;
	/* The EvTab's view is set when the tab is opened with a
	 * real document.  Currently the EvTab is created with
	 * ev_tabbed_window_open_file() and the view is added by
	 * the caller (or, in the simple demo case, not at all).
	 * For now return NULL -- the full view integration is
	 * a follow-up PR. */
	return ev_tab_get_view (active);
}

EvDocument *
ev_tabbed_window_get_active_document (EvTabbedWindow *window)
{
	g_return_val_if_fail (EV_IS_TABBED_WINDOW (window), NULL);
	EvTab *active = ev_tab_manager_get_active (window->priv->tab_manager);
	if (!active)
		return NULL;
	return ev_tab_get_document (active);
}

EvTab *
ev_tabbed_window_open_file (EvTabbedWindow *window,
                            GFile         *file,
                            GError       **error)
{
	g_return_val_if_fail (EV_IS_TABBED_WINDOW (window), NULL);
	g_return_val_if_fail (G_IS_FILE (file), NULL);

	/* If a tab already holds this file, focus it. */
	EvTab *existing = ev_tab_manager_get_tab_by_location (
		window->priv->tab_manager, file);
	if (existing) {
		ev_tab_manager_set_active (window->priv->tab_manager, existing);
		return existing;
	}

	/* Load the document.  ev_document_factory_get_document()
	 * returns the loaded document or NULL on error. */
	gchar *uri = g_file_get_uri (file);
	EvDocument *document = ev_document_factory_get_document (uri, error);
	g_free (uri);
	if (!document)
		return NULL;

	EvTab *tab = EV_TAB (ev_tab_new (document));
	ev_tab_set_location (tab, file);
	ev_tab_manager_append_tab (window->priv->tab_manager, tab);

	return tab;
}

gboolean
ev_tabbed_window_close_active_tab (EvTabbedWindow *window)
{
	g_return_val_if_fail (EV_IS_TABBED_WINDOW (window), FALSE);

	EvTab *active = ev_tab_manager_get_active (window->priv->tab_manager);
	if (!active)
		return FALSE;

	ev_tab_manager_remove_tab (window->priv->tab_manager, active);
	return TRUE;
}

void
ev_tabbed_window_select_next_tab (EvTabbedWindow *window)
{
	g_return_if_fail (EV_IS_TABBED_WINDOW (window));
	ev_tab_manager_select_next (window->priv->tab_manager);
}

void
ev_tabbed_window_select_prev_tab (EvTabbedWindow *window)
{
	g_return_if_fail (EV_IS_TABBED_WINDOW (window));
	ev_tab_manager_select_prev (window->priv->tab_manager);
}

gboolean
ev_tabbed_window_get_tab_bar_visible (EvTabbedWindow *window)
{
	g_return_val_if_fail (EV_IS_TABBED_WINDOW (window), FALSE);
	return gtk_notebook_get_show_tabs (GTK_NOTEBOOK (window->priv->notebook));
}

void
ev_tabbed_window_reopen_last_closed_tab (EvTabbedWindow *window)
{
	g_return_if_fail (EV_IS_TABBED_WINDOW (window));
	ev_tab_manager_reopen_last_closed_tab (window->priv->tab_manager);
}

guint
ev_tabbed_window_get_reopen_stack_size (EvTabbedWindow *window)
{
	g_return_val_if_fail (EV_IS_TABBED_WINDOW (window), 0);
	return ev_tab_manager_get_reopen_stack_size (window->priv->tab_manager);
}

/* --- drag-and-drop handlers --- */

/* Extract the first file URI from a "drag-data-received" event
 * and return it as a GFile.  Returns NULL if no file URI was
 * found. */
static GFile *
extract_first_file_from_selection (GtkSelectionData *selection)
{
	const gchar *uri_list = (const gchar *) gtk_selection_data_get_data (selection);
	if (!uri_list || !*uri_list)
		return NULL;

	/* The URI list is newline-separated.  Take the first non-empty
	 * line. */
	const gchar *p = uri_list;
	const gchar *end = p;
	while (*end && *end != '\n' && *end != '\r')
		end++;

	gsize len = end - p;
	if (len == 0)
		return NULL;

	gchar *first_uri = g_strndup (p, len);
	GFile *file = g_file_new_for_uri (first_uri);
	g_free (first_uri);
	return file;
}

static void
on_page_drop (GtkWidget         *widget,
              GdkDragContext    *context,
              gint               x,
              gint               y,
              GtkSelectionData  *selection,
              guint              info,
              guint              time,
              gpointer           user_data)
{
	EvTabbedWindow *window = EV_TABBED_WINDOW (user_data);
	GFile *file = extract_first_file_from_selection (selection);
	if (file) {
		ev_tabbed_window_open_file (window, file, NULL);
		g_object_unref (file);
		gtk_drag_finish (context, TRUE, FALSE, time);
	} else {
		gtk_drag_finish (context, FALSE, FALSE, time);
	}
}

static void
on_empty_drop (GtkWidget         *widget,
               GdkDragContext    *context,
               gint               x,
               gint               y,
               GtkSelectionData  *selection,
               guint              info,
               guint              time,
               gpointer           user_data)
{
	on_page_drop (widget, context, x, y, selection, info, time, user_data);
}
