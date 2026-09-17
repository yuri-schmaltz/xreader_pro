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

#if !defined (__EV_XREADER_SHELL_H_INSIDE__) && !defined (XREADER_COMPILATION)
#error "Only <xreader-shell.h> can be included directly."
#endif

#ifndef EV_TAB_H
#define EV_TAB_H

#include <glib-object.h>
#include <gtk/gtk.h>

#include "ev-document.h"
#include "ev-document-model.h"
#include "ev-view.h"

G_BEGIN_DECLS

#define EV_TYPE_TAB (ev_tab_get_type ())
#define EV_TAB(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), EV_TYPE_TAB, EvTab))
#define EV_IS_TAB(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EV_TYPE_TAB))

typedef struct _EvTab EvTab;
typedef struct _EvTabClass EvTabClass;
typedef struct _EvTabPrivate EvTabPrivate;

/**
 * EvTab:
 *
 * A single document tab.  Wraps an EvView + per-tab state (filename,
 * page, scroll, find query, modified flag).  The tab is the unit of
 * the tabbed document view; the EvTabManager owns a list of them.
 *
 * Since: 4.8.0
 */
struct _EvTab
{
	GtkBox parent;

	/*< private >*/
	EvTabPrivate *priv;
};

struct _EvTabClass
{
	GtkBoxClass parent_class;
};

GType         ev_tab_get_type          (void) G_GNUC_CONST;

GtkWidget    *ev_tab_new               (EvDocument *document);

/* The EvView inside this tab.  Returns a borrowed reference. */
GtkWidget    *ev_tab_get_view          (EvTab      *tab);

/* Set the EvView (and its scrolled window) inside this tab.
 * The tab takes ownership of @view via the container. */
void          ev_tab_set_view          (EvTab      *tab,
                                       EvView      *view);

/* The document inside this tab.  Returns a borrowed reference. */
EvDocument   *ev_tab_get_document      (EvTab      *tab);

/* The file the document was loaded from, or NULL if the document is
 * not associated with a file (e.g. it was loaded from a stream or
 * a remote URI).  Returns a copy that the caller must g_free(). */
GFile        *ev_tab_get_location      (EvTab      *tab);

void          ev_tab_set_location      (EvTab      *tab,
                                       GFile      *location);

/* The page currently displayed in the tab. */
gint          ev_tab_get_page          (EvTab      *tab);
void          ev_tab_set_page          (EvTab      *tab,
                                       gint        page);

/* "Modified" flag (e.g. the user has an unsaved form fill in progress
 * or an annotation that hasn't been persisted).  The tab UI shows
 * an indicator when this is TRUE. */
gboolean      ev_tab_get_modified      (EvTab      *tab);
void          ev_tab_set_modified      (EvTab      *tab,
                                       gboolean    modified);

/* The user-visible title of the tab.  Returns a copy that the caller
 * must g_free().  Defaults to the document's title or the location's
 * basename if no title is available. */
gchar        *ev_tab_get_title         (EvTab      *tab);
void          ev_tab_set_title         (EvTab      *tab,
                                       const gchar *title);

/* The "tooltip" shown when the user hovers over the tab.  Returns
 * a copy that the caller must g_free().  Defaults to the full
 * location URI or "Untitled" if no location is set. */
gchar        *ev_tab_get_tooltip       (EvTab      *tab);
void          ev_tab_set_tooltip       (EvTab      *tab,
                                       const gchar *tooltip);

/* Internal: the GtkBox inside the tab that holds the view + scrollbar.
 * Used by EvTabManager to pack the tab into the notebook. */
GtkWidget    *ev_tab_get_box           (EvTab      *tab);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (EvTab, g_object_unref)

G_END_DECLS

#endif /* EV_TAB_H */
