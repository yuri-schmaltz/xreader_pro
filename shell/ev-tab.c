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
#include "ev-tab.h"
#include "ev-gtk-compat.h"

#include <glib/gi18n.h>

struct _EvTabPrivate
{
	EvDocument  *document;
	GtkWidget   *view;        /* EvView, owned by the box */
	GtkWidget   *box;         /* GtkBox that holds the view + scrolled window */
	GFile       *location;
	gint         page;
	gboolean     modified;
	gchar       *title;
	gchar       *tooltip;
};

G_DEFINE_TYPE_WITH_PRIVATE (EvTab, ev_tab, GTK_TYPE_BOX)

enum {
	PROP_0,
	PROP_DOCUMENT,
	PROP_LOCATION,
	PROP_PAGE,
	PROP_MODIFIED,
	PROP_TITLE,
	PROP_TOOLTIP,
};

enum {
	SIGNAL_CHANGED,
	N_SIGNALS,
};

static guint signals[N_SIGNALS];

static void
ev_tab_set_property (GObject      *object,
                     guint         prop_id,
                     const GValue *value,
                     GParamSpec   *pspec)
{
	EvTab *tab = EV_TAB (object);

	switch (prop_id) {
	case PROP_DOCUMENT:
		tab->priv->document = g_value_dup_object (value);
		break;
	case PROP_LOCATION:
		ev_tab_set_location (tab, g_value_get_object (value));
		break;
	case PROP_PAGE:
		ev_tab_set_page (tab, g_value_get_int (value));
		break;
	case PROP_MODIFIED:
		ev_tab_set_modified (tab, g_value_get_boolean (value));
		break;
	case PROP_TITLE:
		ev_tab_set_title (tab, g_value_get_string (value));
		break;
	case PROP_TOOLTIP:
		ev_tab_set_tooltip (tab, g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
ev_tab_get_property (GObject    *object,
                     guint       prop_id,
                     GValue     *value,
                     GParamSpec *pspec)
{
	EvTab *tab = EV_TAB (object);

	switch (prop_id) {
	case PROP_DOCUMENT:
		g_value_set_object (value, tab->priv->document);
		break;
	case PROP_LOCATION:
		g_value_set_object (value, tab->priv->location);
		break;
	case PROP_PAGE:
		g_value_set_int (value, tab->priv->page);
		break;
	case PROP_MODIFIED:
		g_value_set_boolean (value, tab->priv->modified);
		break;
	case PROP_TITLE:
		g_value_set_string (value, tab->priv->title);
		break;
	case PROP_TOOLTIP:
		g_value_set_string (value, tab->priv->tooltip);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
ev_tab_finalize (GObject *object)
{
	EvTab *tab = EV_TAB (object);

	g_clear_object (&tab->priv->document);
	g_clear_object (&tab->priv->location);
	g_clear_pointer (&tab->priv->title, g_free);
	g_clear_pointer (&tab->priv->tooltip, g_free);

	G_OBJECT_CLASS (ev_tab_parent_class)->finalize (object);
}

static void
emit_changed (EvTab *tab)
{
	g_signal_emit (tab, signals[SIGNAL_CHANGED], 0);
}

static void
ev_tab_init (EvTab *tab)
{
	tab->priv = ev_tab_get_instance_private (tab);

	tab->priv->page = -1;  /* -1 = not set */
	tab->priv->modified = FALSE;
	tab->priv->title = NULL;
	tab->priv->tooltip = NULL;
	tab->priv->location = NULL;

	tab->priv->box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	ev_gtk_box_append (tab, tab->priv->box);
	gtk_widget_show (tab->priv->box);
}

static void
ev_tab_class_init (EvTabClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = ev_tab_set_property;
	object_class->get_property = ev_tab_get_property;
	object_class->finalize     = ev_tab_finalize;

	g_object_class_install_property (object_class, PROP_DOCUMENT,
		g_param_spec_object ("document", "Document", "The document in this tab",
		                     EV_TYPE_DOCUMENT, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class, PROP_LOCATION,
		g_param_spec_object ("location", "Location", "The file the document was loaded from",
		                     G_TYPE_FILE, G_PARAM_READWRITE));

	g_object_class_install_property (object_class, PROP_PAGE,
		g_param_spec_int ("page", "Page", "The page currently displayed",
		                  -1, G_MAXINT, -1,
		                  G_PARAM_READWRITE));

	g_object_class_install_property (object_class, PROP_MODIFIED,
		g_param_spec_boolean ("modified", "Modified", "Whether the tab has unsaved changes",
		                     FALSE, G_PARAM_READWRITE));

	g_object_class_install_property (object_class, PROP_TITLE,
		g_param_spec_string ("title", "Title", "The user-visible title of the tab",
		                     NULL, G_PARAM_READWRITE));

	g_object_class_install_property (object_class, PROP_TOOLTIP,
		g_param_spec_string ("tooltip", "Tooltip", "The tooltip shown on hover",
		                     NULL, G_PARAM_READWRITE));

	signals[SIGNAL_CHANGED] = g_signal_new ("changed",
	                                        G_TYPE_FROM_CLASS (object_class),
	                                        G_SIGNAL_RUN_LAST,
	                                        0, NULL, NULL, NULL,
	                                        G_TYPE_NONE, 0);
}

GtkWidget *
ev_tab_new (EvDocument *document)
{
	g_return_val_if_fail (EV_IS_DOCUMENT (document), NULL);

	return g_object_new (EV_TYPE_TAB,
	                     "document", document,
	                     "orientation", GTK_ORIENTATION_VERTICAL,
	                     NULL);
}

GtkWidget *
ev_tab_get_view (EvTab *tab)
{
	g_return_val_if_fail (EV_IS_TAB (tab), NULL);
	return tab->priv->view;
}

EvDocument *
ev_tab_get_document (EvTab *tab)
{
	g_return_val_if_fail (EV_IS_TAB (tab), NULL);
	return tab->priv->document;
}

GFile *
ev_tab_get_location (EvTab *tab)
{
	g_return_val_if_fail (EV_IS_TAB (tab), NULL);
	return tab->priv->location ? g_object_ref (tab->priv->location) : NULL;
}

void
ev_tab_set_location (EvTab *tab,
                     GFile *location)
{
	g_return_if_fail (EV_IS_TAB (tab));

	g_clear_object (&tab->priv->location);
	tab->priv->location = location ? g_object_ref (location) : NULL;
	emit_changed (tab);
}

gint
ev_tab_get_page (EvTab *tab)
{
	g_return_val_if_fail (EV_IS_TAB (tab), -1);
	return tab->priv->page;
}

void
ev_tab_set_page (EvTab *tab,
                 gint   page)
{
	g_return_if_fail (EV_IS_TAB (tab));

	if (tab->priv->page == page)
		return;
	tab->priv->page = page;
	emit_changed (tab);
}

gboolean
ev_tab_get_modified (EvTab *tab)
{
	g_return_val_if_fail (EV_IS_TAB (tab), FALSE);
	return tab->priv->modified;
}

void
ev_tab_set_modified (EvTab *tab,
                     gboolean modified)
{
	g_return_if_fail (EV_IS_TAB (tab));

	if (tab->priv->modified == modified)
		return;
	tab->priv->modified = modified;
	emit_changed (tab);
}

gchar *
ev_tab_get_title (EvTab *tab)
{
	g_return_val_if_fail (EV_IS_TAB (tab), NULL);

	if (tab->priv->title)
		return g_strdup (tab->priv->title);

	if (tab->priv->document) {
		const gchar *title = ev_document_get_title (tab->priv->document);
		if (title && *title)
			return g_strdup (title);
	}

	if (tab->priv->location) {
		gchar *basename = g_file_get_basename (tab->priv->location);
		if (basename)
			return basename;
	}

	return g_strdup (_("Untitled"));
}

void
ev_tab_set_title (EvTab      *tab,
                  const gchar *title)
{
	g_return_if_fail (EV_IS_TAB (tab));

	g_clear_pointer (&tab->priv->title, g_free);
	tab->priv->title = g_strdup (title);
	emit_changed (tab);
}

gchar *
ev_tab_get_tooltip (EvTab *tab)
{
	g_return_val_if_fail (EV_IS_TAB (tab), NULL);

	if (tab->priv->tooltip)
		return g_strdup (tab->priv->tooltip);

	if (tab->priv->location) {
		gchar *uri = g_file_get_uri (tab->priv->location);
		if (uri)
			return uri;
	}

	return g_strdup (_("Untitled"));
}

void
ev_tab_set_tooltip (EvTab      *tab,
                    const gchar *tooltip)
{
	g_return_if_fail (EV_IS_TAB (tab));

	g_clear_pointer (&tab->priv->tooltip, g_free);
	tab->priv->tooltip = g_strdup (tooltip);
	emit_changed (tab);
}

GtkWidget *
ev_tab_get_box (EvTab *tab)
{
	g_return_val_if_fail (EV_IS_TAB (tab), NULL);
	return tab->priv->box;
}
