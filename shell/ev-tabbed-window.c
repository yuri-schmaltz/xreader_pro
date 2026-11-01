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
#include "ev-document-model.h"
#include "ev-gtk-compat.h"
#include "ev-sidebar.h"
#include "ev-sidebar-thumbnails.h"

struct _EvTabbedWindowPrivate
{
	EvTabManager *tab_manager;
	GtkWidget    *notebook;
	GtkWidget    *main_box;
	GtkWidget    *menubar;
	GtkWidget    *toolbar;
	GtkWidget    *statusbar;
	GtkWidget    *empty_label;  /* shown when no tabs are open */
	GtkWidget    *hpaned;
	GtkWidget    *sidebar;
	EvDocumentModel *sidebar_model;  /* owned; bound to EvSidebar for its lifetime */
	gboolean        updating_sidebar; /* guard against page-changed feedback loop */

	/* Statusbar: an outer box holding a GtkStatusbar (left) and a
	 * zoom label (right). */
	GtkWidget    *statusbar_box;
	GtkWidget    *statusbar_inner;
	GtkWidget    *zoom_label;
	guint         statusbar_context;

	/* The active tab's model we watch for page/zoom changes (borrowed). */
	EvDocumentModel *watched_model;
	gulong           watched_page_handler;
	gulong           watched_scale_handler;
	gulong           watched_sizing_handler;
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

static void update_statusbar (EvTabbedWindow *window);
static void watch_active_model (EvTabbedWindow *window);

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
	gtk_widget_set_visible (window->priv->hpaned,      has_tabs);
	gtk_widget_set_sensitive (window->priv->menubar,   has_tabs);
	gtk_widget_set_sensitive (window->priv->toolbar,   has_tabs);
	gtk_widget_set_sensitive (window->priv->statusbar_box, has_tabs);
}

/* Point the thumbnail sidebar at the active tab's document.  The
 * EvSidebar is bound to a single EvDocumentModel for its lifetime
 * (ev_sidebar_add_page wires each page to that model), so we keep
 * one sidebar model per window and swap its document/page/rotation
 * to mirror the active tab's model. */
static void
update_sidebar_model (EvTabbedWindow *window,
                      EvTab          *tab)
{
	if (!tab) {
		gtk_widget_set_visible (window->priv->sidebar, FALSE);
		watch_active_model (window);
		update_statusbar (window);
		return;
	}

	EvDocument *document = ev_tab_get_document (tab);
	if (!document) {
		gtk_widget_set_visible (window->priv->sidebar, FALSE);
		watch_active_model (window);
		update_statusbar (window);
		return;
	}

	gtk_widget_set_visible (window->priv->sidebar, TRUE);

	EvDocumentModel *sidebar_model = window->priv->sidebar_model;
	EvDocumentModel *tab_model = ev_tab_get_model (tab);

	window->priv->updating_sidebar = TRUE;
	ev_document_model_set_document (sidebar_model, document);
	if (tab_model) {
		ev_document_model_set_page (
			sidebar_model,
			ev_document_model_get_page (tab_model));
		ev_document_model_set_rotation (
			sidebar_model,
			ev_document_model_get_rotation (tab_model));
	}
	window->priv->updating_sidebar = FALSE;

	watch_active_model (window);
	update_statusbar (window);
}

/* A thumbnail was clicked (or the sidebar changed page): mirror it
 * back onto the active tab's model so the main view follows. */
static void
on_sidebar_model_page_changed (EvDocumentModel *model,
                               GParamSpec      *pspec,
                               EvTabbedWindow  *window)
{
	if (window->priv->updating_sidebar)
		return;

	EvTab *tab = ev_tab_manager_get_active (window->priv->tab_manager);
	if (!tab)
		return;

	EvDocumentModel *tab_model = ev_tab_get_model (tab);
	if (!tab_model)
		return;

	gint page = ev_document_model_get_page (model);
	if (page != ev_document_model_get_page (tab_model))
		ev_document_model_set_page (tab_model, page);
}

/* --- helpers to reach the active tab's view/model --- */

static EvView *
get_active_view (EvTabbedWindow *window)
{
	EvTab *active = ev_tab_manager_get_active (window->priv->tab_manager);
	GtkWidget *view = active ? ev_tab_get_view (active) : NULL;
	return view ? EV_VIEW (view) : NULL;
}

static EvDocumentModel *
get_active_model (EvTabbedWindow *window)
{
	EvTab *active = ev_tab_manager_get_active (window->priv->tab_manager);
	return active ? ev_tab_get_model (active) : NULL;
}

/* --- statusbar --- */

static void
update_statusbar (EvTabbedWindow *window)
{
	EvDocumentModel *model = get_active_model (window);

	if (window->priv->statusbar_context)
		gtk_statusbar_remove_all (
			GTK_STATUSBAR (window->priv->statusbar_inner),
			window->priv->statusbar_context);

	if (!model) {
		gtk_label_set_text (GTK_LABEL (window->priv->zoom_label), "");
		return;
	}

	EvTab *active = ev_tab_manager_get_active (window->priv->tab_manager);
	EvDocument *doc = active ? ev_tab_get_document (active) : NULL;
	gint page = ev_document_model_get_page (model);
	gint n_pages = doc ? ev_document_get_n_pages (doc) : 0;

	gchar *msg = g_strdup_printf (_("Page %d of %d"), page + 1, n_pages);
	gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar_inner),
	                    window->priv->statusbar_context, msg);
	g_free (msg);

	/* Zoom readout on the right. */
	switch (ev_document_model_get_sizing_mode (model)) {
	case EV_SIZING_FREE: {
		gint zoom = (gint) (ev_document_model_get_scale (model) * 100.0 + 0.5);
		gchar *z = g_strdup_printf ("%d%%", zoom);
		gtk_label_set_text (GTK_LABEL (window->priv->zoom_label), z);
		g_free (z);
		break;
	}
	case EV_SIZING_FIT_WIDTH:
		gtk_label_set_text (GTK_LABEL (window->priv->zoom_label),
		                    _("Fit Width"));
		break;
	case EV_SIZING_BEST_FIT:
	default:
		gtk_label_set_text (GTK_LABEL (window->priv->zoom_label),
		                    _("Fit Page"));
		break;
	}
}

static void
on_watched_model_notify (EvDocumentModel *model,
                         GParamSpec      *pspec,
                         EvTabbedWindow  *window)
{
	update_statusbar (window);
}

static void
disconnect_watched_model (EvTabbedWindow *window)
{
	if (window->priv->watched_model) {
		g_signal_handler_disconnect (window->priv->watched_model,
		                             window->priv->watched_page_handler);
		g_signal_handler_disconnect (window->priv->watched_model,
		                             window->priv->watched_scale_handler);
		g_signal_handler_disconnect (window->priv->watched_model,
		                             window->priv->watched_sizing_handler);
		window->priv->watched_model = NULL;
	}
}

/* Watch the active tab's model so the statusbar tracks page/zoom. */
static void
watch_active_model (EvTabbedWindow *window)
{
	disconnect_watched_model (window);

	EvDocumentModel *model = get_active_model (window);
	if (!model)
		return;

	window->priv->watched_model = model;
	window->priv->watched_page_handler =
		g_signal_connect (model, "notify::page",
		                  G_CALLBACK (on_watched_model_notify), window);
	window->priv->watched_scale_handler =
		g_signal_connect (model, "notify::scale",
		                  G_CALLBACK (on_watched_model_notify), window);
	window->priv->watched_sizing_handler =
		g_signal_connect (model, "notify::sizing-mode",
		                  G_CALLBACK (on_watched_model_notify), window);
}

static void
on_open_activate (GtkMenuItem    *item,
                  gpointer        user_data)
{
	EvTabbedWindow *window = EV_TABBED_WINDOW (user_data);

#if !GTK_CHECK_VERSION (4, 0, 0)
	GtkWidget *dialog = gtk_file_chooser_dialog_new (
		_("Open Document"),
		GTK_WINDOW (window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		_("_Cancel"), GTK_RESPONSE_CANCEL,
		_("_Open"), GTK_RESPONSE_ACCEPT,
		NULL);

	gtk_file_chooser_set_select_multiple (
		GTK_FILE_CHOOSER (dialog), TRUE);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		GSList *files = gtk_file_chooser_get_filenames (
			GTK_FILE_CHOOSER (dialog));
		GSList *l;
		for (l = files; l != NULL; l = l->next) {
			gchar *uri = g_filename_to_uri ((const gchar *) l->data,
			                                NULL, NULL);
			if (!uri)
				continue;
			GFile *file = g_file_new_for_uri (uri);
			ev_tabbed_window_open_file (window, file, NULL);
			g_object_unref (file);
			g_free (uri);
			g_free (l->data);
		}
		g_slist_free (files);
	}

	gtk_widget_destroy (dialog);
#else
	(void) window;
#endif
}

static void
on_close_tab_activate (GtkMenuItem *item,
                       gpointer     user_data)
{
	ev_tabbed_window_close_active_tab (EV_TABBED_WINDOW (user_data));
}

static void
on_quit_activate (GtkMenuItem *item,
                  gpointer     user_data)
{
	g_application_quit (G_APPLICATION (gtk_window_get_application (
		GTK_WINDOW (user_data))));
}

static void
on_reopen_tab_activate (GtkMenuItem *item,
                        gpointer     user_data)
{
	ev_tabbed_window_reopen_last_closed_tab (EV_TABBED_WINDOW (user_data));
}

/* --- Edit menu --- */

static void
on_copy_activate (GtkMenuItem *item,
                  gpointer     user_data)
{
	EvView *view = get_active_view (EV_TABBED_WINDOW (user_data));
	if (view)
		ev_view_copy (view);
}

static void
on_select_all_activate (GtkMenuItem *item,
                        gpointer     user_data)
{
	EvView *view = get_active_view (EV_TABBED_WINDOW (user_data));
	if (view)
		ev_view_select_all (view);
}

/* --- View menu --- */

static void
on_zoom_in_activate (GtkMenuItem *item,
                     gpointer     user_data)
{
	EvView *view = get_active_view (EV_TABBED_WINDOW (user_data));
	if (view && ev_view_can_zoom_in (view))
		ev_view_zoom_in (view);
}

static void
on_zoom_out_activate (GtkMenuItem *item,
                      gpointer     user_data)
{
	EvView *view = get_active_view (EV_TABBED_WINDOW (user_data));
	if (view && ev_view_can_zoom_out (view))
		ev_view_zoom_out (view);
}

static void
on_zoom_normal_activate (GtkMenuItem *item,
                         gpointer     user_data)
{
	EvDocumentModel *model = get_active_model (EV_TABBED_WINDOW (user_data));
	if (!model)
		return;
	ev_document_model_set_sizing_mode (model, EV_SIZING_FREE);
	ev_document_model_set_scale (model, 1.0);
}

static void
on_rotate_left_activate (GtkMenuItem *item,
                         gpointer     user_data)
{
	EvDocumentModel *model = get_active_model (EV_TABBED_WINDOW (user_data));
	if (!model)
		return;
	gint rotation = ev_document_model_get_rotation (model);
	ev_document_model_set_rotation (model, (rotation + 270) % 360);
}

static void
on_rotate_right_activate (GtkMenuItem *item,
                          gpointer     user_data)
{
	EvDocumentModel *model = get_active_model (EV_TABBED_WINDOW (user_data));
	if (!model)
		return;
	gint rotation = ev_document_model_get_rotation (model);
	ev_document_model_set_rotation (model, (rotation + 90) % 360);
}

static void
on_fullscreen_activate (GtkMenuItem *item,
                        gpointer     user_data)
{
	GtkWindow *window = GTK_WINDOW (user_data);
	GdkWindow *gdkwin = gtk_widget_get_window (GTK_WIDGET (window));
	gboolean is_fullscreen = FALSE;

	if (gdkwin != NULL)
		is_fullscreen = (gdk_window_get_state (gdkwin) &
		                 GDK_WINDOW_STATE_FULLSCREEN) != 0;

	if (is_fullscreen)
		gtk_window_unfullscreen (window);
	else
		gtk_window_fullscreen (window);
}

/* --- Go menu --- */

static void
go_to_page (EvTabbedWindow *window,
            gint            page)
{
	EvDocumentModel *model = get_active_model (window);
	if (!model)
		return;
	EvTab *active = ev_tab_manager_get_active (window->priv->tab_manager);
	EvDocument *doc = active ? ev_tab_get_document (active) : NULL;
	if (!doc)
		return;
	gint n_pages = ev_document_get_n_pages (doc);
	if (page < 0)
		page = 0;
	else if (page >= n_pages)
		page = n_pages - 1;
	ev_document_model_set_page (model, page);
}

static void
on_prev_page_activate (GtkMenuItem *item,
                       gpointer     user_data)
{
	EvTabbedWindow *window = EV_TABBED_WINDOW (user_data);
	EvDocumentModel *model = get_active_model (window);
	if (!model)
		return;
	go_to_page (window, ev_document_model_get_page (model) - 1);
}

static void
on_next_page_activate (GtkMenuItem *item,
                       gpointer     user_data)
{
	EvTabbedWindow *window = EV_TABBED_WINDOW (user_data);
	EvDocumentModel *model = get_active_model (window);
	if (!model)
		return;
	go_to_page (window, ev_document_model_get_page (model) + 1);
}

static void
on_first_page_activate (GtkMenuItem *item,
                        gpointer     user_data)
{
	go_to_page (EV_TABBED_WINDOW (user_data), 0);
}

static void
on_last_page_activate (GtkMenuItem *item,
                       gpointer     user_data)
{
	EvTabbedWindow *window = EV_TABBED_WINDOW (user_data);
	EvTab *active = ev_tab_manager_get_active (window->priv->tab_manager);
	EvDocument *doc = active ? ev_tab_get_document (active) : NULL;
	if (!doc)
		return;
	go_to_page (window, ev_document_get_n_pages (doc) - 1);
}

/* --- Help menu --- */

static void
on_about_activate (GtkMenuItem *item,
                   gpointer     user_data)
{
	GtkWindow *window = GTK_WINDOW (user_data);
	gtk_show_about_dialog (window,
	                       "program-name", "Xreader Pro",
	                       "version", VERSION,
	                       "comments", _("Document Viewer"),
	                       "website", "https://github.com/yuri-schmaltz/xreader_pro",
	                       "logo-icon-name", "xreader",
	                       "license-type", GTK_LICENSE_GPL_2_0,
	                       "wrap-license", TRUE,
	                       NULL);
}

/* Append a mnemonic menu item, wire its "activate" signal and add an
 * optional keyboard accelerator. */
static void
menu_append_item (GtkWidget     *menu,
                  const gchar   *label,
                  GCallback      callback,
                  gpointer       user_data,
                  GtkAccelGroup *accel_group,
                  guint          accel_key,
                  GdkModifierType accel_mods)
{
	GtkWidget *item = gtk_menu_item_new_with_mnemonic (label);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);
	if (callback)
		g_signal_connect (item, "activate", callback, user_data);
	if (accel_group && accel_key != 0)
		gtk_widget_add_accelerator (item, "activate", accel_group,
		                            accel_key, accel_mods,
		                            GTK_ACCEL_VISIBLE);
}

static void
menu_append_separator (GtkWidget *menu)
{
	GtkWidget *item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);
}

/* Create a top-level menu with a mnemonic label and return its submenu. */
static GtkWidget *
menu_append_menu (GtkWidget   *menubar,
                  const gchar *label)
{
	GtkWidget *item = gtk_menu_item_new_with_mnemonic (label);
	GtkWidget *menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
	gtk_menu_shell_append (GTK_MENU_SHELL (menubar), item);
	gtk_widget_show (item);
	return menu;
}

static GtkWidget *
build_menubar (EvTabbedWindow *window)
{
	GtkAccelGroup *accel_group = gtk_accel_group_new ();
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

	GtkWidget *menubar = gtk_menu_bar_new ();

	/* File */
	GtkWidget *file_menu = menu_append_menu (menubar, _("_File"));
	menu_append_item (file_menu, _("_Open..."),
	                  G_CALLBACK (on_open_activate), window,
	                  accel_group, GDK_KEY_o, GDK_CONTROL_MASK);
	menu_append_separator (file_menu);
	/* Close Tab (Ctrl+W) and Reopen Closed Tab (Ctrl+Shift+T) are
	 * handled by the window's key controller; no accelerators here to
	 * avoid double-firing. */
	menu_append_item (file_menu, _("Close _Tab"),
	                  G_CALLBACK (on_close_tab_activate), window,
	                  NULL, 0, 0);
	menu_append_item (file_menu, _("Reopen Closed Tab"),
	                  G_CALLBACK (on_reopen_tab_activate), window,
	                  NULL, 0, 0);
	menu_append_separator (file_menu);
	menu_append_item (file_menu, _("_Quit"),
	                  G_CALLBACK (on_quit_activate), window,
	                  accel_group, GDK_KEY_q, GDK_CONTROL_MASK);

	/* Edit */
	GtkWidget *edit_menu = menu_append_menu (menubar, _("_Edit"));
	menu_append_item (edit_menu, _("_Copy"),
	                  G_CALLBACK (on_copy_activate), window,
	                  accel_group, GDK_KEY_c, GDK_CONTROL_MASK);
	menu_append_item (edit_menu, _("Select _All"),
	                  G_CALLBACK (on_select_all_activate), window,
	                  accel_group, GDK_KEY_a, GDK_CONTROL_MASK);

	/* View */
	GtkWidget *view_menu = menu_append_menu (menubar, _("_View"));
	menu_append_item (view_menu, _("Zoom _In"),
	                  G_CALLBACK (on_zoom_in_activate), window,
	                  accel_group, GDK_KEY_plus, GDK_CONTROL_MASK);
	menu_append_item (view_menu, _("Zoom _Out"),
	                  G_CALLBACK (on_zoom_out_activate), window,
	                  accel_group, GDK_KEY_minus, GDK_CONTROL_MASK);
	menu_append_item (view_menu, _("_Normal"),
	                  G_CALLBACK (on_zoom_normal_activate), window,
	                  accel_group, GDK_KEY_0, GDK_CONTROL_MASK);
	menu_append_separator (view_menu);
	menu_append_item (view_menu, _("Rotate _Left"),
	                  G_CALLBACK (on_rotate_left_activate), window,
	                  NULL, 0, 0);
	menu_append_item (view_menu, _("Rotate _Right"),
	                  G_CALLBACK (on_rotate_right_activate), window,
	                  NULL, 0, 0);
	menu_append_separator (view_menu);
	menu_append_item (view_menu, _("Full Screen"),
	                  G_CALLBACK (on_fullscreen_activate), window,
	                  accel_group, GDK_KEY_F11, 0);

	/* Go.  Ctrl+Page_Up/Down are reserved for tab switching by the
	 * window's key controller, so page navigation is exposed as plain
	 * menu actions (plus Ctrl+Home/End for first/last). */
	GtkWidget *go_menu = menu_append_menu (menubar, _("_Go"));
	menu_append_item (go_menu, _("_Previous Page"),
	                  G_CALLBACK (on_prev_page_activate), window,
	                  NULL, 0, 0);
	menu_append_item (go_menu, _("_Next Page"),
	                  G_CALLBACK (on_next_page_activate), window,
	                  NULL, 0, 0);
	menu_append_item (go_menu, _("_First Page"),
	                  G_CALLBACK (on_first_page_activate), window,
	                  accel_group, GDK_KEY_Home, GDK_CONTROL_MASK);
	menu_append_item (go_menu, _("_Last Page"),
	                  G_CALLBACK (on_last_page_activate), window,
	                  accel_group, GDK_KEY_End, GDK_CONTROL_MASK);

	/* Tabs.  Ctrl+Tab / Ctrl+Shift+Tab are handled by the key
	 * controller, so no accelerators are attached here. */
	GtkWidget *tabs_menu = menu_append_menu (menubar, _("_Tabs"));
	menu_append_item (tabs_menu, _("_Next Tab"),
	                  G_CALLBACK (ev_tabbed_window_select_next_tab),
	                  window, NULL, 0, 0);
	menu_append_item (tabs_menu, _("P_revious Tab"),
	                  G_CALLBACK (ev_tabbed_window_select_prev_tab),
	                  window, NULL, 0, 0);

	/* Help */
	GtkWidget *help_menu = menu_append_menu (menubar, _("_Help"));
	menu_append_item (help_menu, _("_About"),
	                  G_CALLBACK (on_about_activate), window,
	                  NULL, 0, 0);

	g_object_unref (accel_group);
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

/* Tab width policy: labels start at the title's natural length and are
 * clamped to [TAB_MIN_WIDTH_CHARS, TAB_MAX_WIDTH_CHARS].  As the bar
 * fills, update_tab_widths() shrinks each label toward the minimum
 * before the notebook starts scrolling. */
#define TAB_MIN_WIDTH_CHARS 8
#define TAB_MAX_WIDTH_CHARS 24

static GtkWidget *
build_tab_label (EvTab *tab)
{
	GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
	gchar *title = ev_tab_get_title (tab);
	GtkWidget *label = gtk_label_new (title);
	gint len = title ? g_utf8_strlen (title, -1) : 0;
	gint initial = CLAMP (len, TAB_MIN_WIDTH_CHARS, TAB_MAX_WIDTH_CHARS);
	g_free (title);

	/* Fix the requested width so the tab occupies real space.  Setting
	 * only max_width_chars would cap an already-tiny request and the
	 * tab would shrink to an ellipsis; width_chars forces the label to
	 * request this many characters. */
	gtk_label_set_width_chars (GTK_LABEL (label), initial);
	gtk_label_set_max_width_chars (GTK_LABEL (label), initial);
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

/* Tab labels shrink as the tab bar fills up: each tab gets an equal
 * share of the notebook width, clamped to [TAB_MIN_WIDTH_CHARS,
 * TAB_MAX_WIDTH_CHARS].  With few tabs the title is shown in full
 * (up to the max); as more tabs appear the titles ellipsize down to
 * the minimum before the bar starts scrolling. */
static void
update_tab_widths (EvTabbedWindow *window)
{
	GtkNotebook *notebook = GTK_NOTEBOOK (window->priv->notebook);
	gint n_tabs = gtk_notebook_get_n_pages (notebook);
	if (n_tabs <= 0)
		return;

	gint width = gtk_widget_get_allocated_width (window->priv->notebook);
	if (width <= 1)
		return;

	/* Reserve room for the close button (~28px), box padding and the
	 * notebook border, then divide the rest across the tabs.  ~7px per
	 * character is a reasonable average for the UI font. */
	gint per_tab = width / n_tabs;
	gint chars = (per_tab - 48) / 7;
	chars = CLAMP (chars, TAB_MIN_WIDTH_CHARS, TAB_MAX_WIDTH_CHARS);

	gint i;
	for (i = 0; i < n_tabs; i++) {
		GtkWidget *page = gtk_notebook_get_nth_page (notebook, i);
		GtkWidget *tab_label = gtk_notebook_get_tab_label (notebook, page);
		if (!tab_label)
			continue;
		GList *children = gtk_container_get_children (GTK_CONTAINER (tab_label));
		if (children) {
			GtkWidget *label = GTK_WIDGET (children->data);
			if (GTK_IS_LABEL (label)) {
				/* width_chars fixes the requested width so tabs share
				 * the bar; max_width_chars caps it.  Ellipsize long
				 * titles instead of letting them push the bar. */
				gtk_label_set_ellipsize (GTK_LABEL (label),
				                         PANGO_ELLIPSIZE_END);
				gtk_label_set_width_chars (GTK_LABEL (label), chars);
				gtk_label_set_max_width_chars (GTK_LABEL (label), chars);
			}
			g_list_free (children);
		}
	}
}

static void
on_notebook_size_allocate (GtkWidget     *widget,
                           GdkRectangle  *allocation,
                           gpointer       user_data)
{
	update_tab_widths (EV_TABBED_WINDOW (user_data));
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

	/* GTK3 does not show notebook pages automatically; make
	 * sure the tab (and the view inside it) is visible. */
	gtk_widget_show_all (GTK_WIDGET (tab));

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
	update_tab_widths (window);
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
	update_tab_widths (window);
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
	update_sidebar_model (window, tab);
}

static void
ev_tabbed_window_dispose (GObject *object)
{
	EvTabbedWindow *window = EV_TABBED_WINDOW (object);

	disconnect_watched_model (window);
	g_clear_object (&window->priv->sidebar_model);
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

	window->priv->menubar = build_menubar (window);
	ev_gtk_box_pack_start (GTK_BOX (window->priv->main_box),
	                       window->priv->menubar,
	                       FALSE, FALSE, 0);

	window->priv->toolbar = build_toolbar ();
	ev_gtk_box_pack_start (GTK_BOX (window->priv->main_box),
	                       window->priv->toolbar,
	                       FALSE, FALSE, 0);

	window->priv->hpaned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
	/* The page thumbnails are THUMBNAIL_DEFAULT_WIDTH (100px) wide;
	 * size the sidebar just wide enough to fit them plus the icon-view
	 * border and scrollbar, instead of a fixed oversized column. */
	gtk_paned_set_position (GTK_PANED (window->priv->hpaned), 130);

	/* Left sidebar: page thumbnails.  EvSidebar binds each page to
	 * one EvDocumentModel for its lifetime, so we own a single
	 * sidebar model and swap its document/page to mirror the active
	 * tab (see update_sidebar_model). */
	window->priv->sidebar_model = ev_document_model_new ();
	g_signal_connect (window->priv->sidebar_model, "notify::page",
	                  G_CALLBACK (on_sidebar_model_page_changed), window);

	window->priv->sidebar = ev_sidebar_new ();
	ev_sidebar_set_model (EV_SIDEBAR (window->priv->sidebar),
	                      window->priv->sidebar_model);
	{
		GtkWidget *thumbs = ev_sidebar_thumbnails_new ();
		ev_sidebar_add_page (EV_SIDEBAR (window->priv->sidebar), thumbs);
	}
	gtk_paned_pack1 (GTK_PANED (window->priv->hpaned),
	                 window->priv->sidebar, FALSE, FALSE);
	gtk_widget_show (window->priv->sidebar);

	window->priv->notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (window->priv->notebook), FALSE);
	gtk_notebook_set_scrollable (GTK_NOTEBOOK (window->priv->notebook), TRUE);
	/* Recompute per-tab widths whenever the notebook is resized so
	 * tabs shrink/grow as the window (and tab count) changes. */
	g_signal_connect (window->priv->notebook, "size-allocate",
	                  G_CALLBACK (on_notebook_size_allocate), window);
	gtk_paned_pack2 (GTK_PANED (window->priv->hpaned),
	                 window->priv->notebook, TRUE, TRUE);

	ev_gtk_box_pack_start (GTK_BOX (window->priv->main_box),
	                       window->priv->hpaned,
	                       TRUE, TRUE, 0);
	gtk_widget_show (window->priv->hpaned);

	window->priv->empty_label = gtk_label_new (
		_("Open a document to start.  Drag a file here, or use File > Open."));
	gtk_widget_set_vexpand (window->priv->empty_label, TRUE);
	gtk_widget_set_valign (window->priv->empty_label, GTK_ALIGN_CENTER);
	gtk_widget_set_halign (window->priv->empty_label, GTK_ALIGN_CENTER);
	ev_gtk_box_pack_start (GTK_BOX (window->priv->main_box),
	                       window->priv->empty_label,
	                       TRUE, TRUE, 0);

	/* The empty-state label is also a drop target.  When the
	 * user drags a file to a window with no tabs, the file
	 * is opened in a new tab. */
	gtk_drag_dest_set (window->priv->empty_label,
	                   GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY);
	gtk_drag_dest_add_uri_targets (window->priv->empty_label);
	g_signal_connect (window->priv->empty_label, "drag-data-received",
	                  G_CALLBACK (on_empty_drop), window);

	/* Statusbar: GtkStatusbar on the left (page info) with a zoom
	 * label pushed to the right (the statusbar expands, the label
	 * does not). */
	window->priv->statusbar_box =
		gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_start (window->priv->statusbar_box, 4);
	gtk_widget_set_margin_end (window->priv->statusbar_box, 4);
	gtk_widget_set_margin_top (window->priv->statusbar_box, 2);
	gtk_widget_set_margin_bottom (window->priv->statusbar_box, 2);

	window->priv->statusbar_inner = gtk_statusbar_new ();
	window->priv->statusbar_context = gtk_statusbar_get_context_id (
		GTK_STATUSBAR (window->priv->statusbar_inner), "tabbed-statusbar");

	window->priv->zoom_label = gtk_label_new ("");
	gtk_widget_set_halign (window->priv->zoom_label, GTK_ALIGN_END);
	gtk_widget_set_margin_start (window->priv->zoom_label, 8);

	ev_gtk_box_pack_start (GTK_BOX (window->priv->statusbar_box),
	                       window->priv->statusbar_inner,
	                       TRUE, TRUE, 0);
	ev_gtk_box_pack_start (GTK_BOX (window->priv->statusbar_box),
	                       window->priv->zoom_label,
	                       FALSE, FALSE, 0);

	ev_gtk_box_pack_start (GTK_BOX (window->priv->main_box),
	                       window->priv->statusbar_box,
	                       FALSE, FALSE, 0);

	gtk_widget_show_all (window->priv->main_box);

	update_empty_state (window);

	gtk_window_set_title (GTK_WINDOW (window), _("Xreader"));

	/* Set the default size *before* showing the window, otherwise the
	 * window is realized with a stale (tiny) geometry and the default
	 * never takes effect. */
	gtk_window_set_default_size (GTK_WINDOW (window), 600, 600);

	gtk_widget_show (GTK_WIDGET (window));

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

	/* Render the document inside the tab: build an EvDocumentModel
	 * and an EvView, and hand the view to the tab.  The EvView
	 * takes a reference on the model, so dropping our own model
	 * reference below is safe. */
	EvDocumentModel *model = ev_document_model_new ();
	EvView *view = EV_VIEW (ev_view_new ());
	ev_view_set_model (view, model);
	ev_document_model_set_document (model, document);
	g_object_unref (model);
	ev_tab_set_view (tab, view);
	ev_tab_set_model (tab, model);

	g_object_unref (document);
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
