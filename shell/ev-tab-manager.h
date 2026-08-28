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

#ifndef EV_TAB_MANAGER_H
#define EV_TAB_MANAGER_H

#include <glib-object.h>
#include <gtk/gtk.h>

#include "ev-tab.h"

G_BEGIN_DECLS

#define EV_TYPE_TAB_MANAGER (ev_tab_manager_get_type ())
#define EV_TAB_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), EV_TYPE_TAB_MANAGER, EvTabManager))
#define EV_IS_TAB_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EV_TYPE_TAB_MANAGER))

typedef struct _EvTabManager EvTabManager;
typedef struct _EvTabManagerClass EvTabManagerClass;
typedef struct _EvTabManagerPrivate EvTabManagerPrivate;

/**
 * EvTabManager:
 *
 * Owns a list of EvTab instances and tracks the active one.
 * Used by EvWindow to host multiple documents in a single
 * window (the C3 tabbed view).
 *
 * The manager is independent of the notebook widget: it
 * just maintains the list + active index, and emits
 * signals that EvWindow's notebook code listens to.
 *
 * Since: 4.8.0
 */
struct _EvTabManager
{
	GObject parent;

	/*< private >*/
	EvTabManagerPrivate *priv;
};

struct _EvTabManagerClass
{
	GObjectClass parent_class;
};

GType         ev_tab_manager_get_type  (void) G_GNUC_CONST;

EvTabManager *ev_tab_manager_new       (void);

/* Number of tabs currently open. */
guint         ev_tab_manager_get_n_tabs (EvTabManager *manager);

/* The active tab, or NULL if no tabs are open. */
EvTab        *ev_tab_manager_get_active (EvTabManager *manager);
void          ev_tab_manager_set_active (EvTabManager *manager,
                                        EvTab        *tab);

/* The tab at the given index, or NULL if the index is out of range. */
EvTab        *ev_tab_manager_get_tab    (EvTabManager *manager,
                                        guint         index);

/* The index of the given tab, or -1 if the tab is not in the list. */
gint          ev_tab_manager_get_tab_index (EvTabManager *manager,
                                            EvTab        *tab);

/* Find the tab that holds the given location, or NULL if no tab holds
 * it.  Used to focus an already-open tab instead of opening a
 * duplicate. */
EvTab        *ev_tab_manager_get_tab_by_location (EvTabManager *manager,
                                                  GFile        *location);

/* Append a tab to the list.  The manager takes a reference on the
 * tab.  The new tab becomes the active one.  Returns the new tab. */
EvTab        *ev_tab_manager_append_tab  (EvTabManager *manager,
                                          EvTab        *tab);

/* Remove a tab from the list.  The manager drops its reference.
 * If the removed tab was the active one, the previous tab (or the
 * next one if no previous exists) becomes the active one.  If the
 * list becomes empty, the active tab becomes NULL. */
void          ev_tab_manager_remove_tab  (EvTabManager *manager,
                                          EvTab        *tab);

/* Select the next / previous tab in the list (wraps around).  No-op
 * if the list has fewer than 2 tabs. */
void          ev_tab_manager_select_next (EvTabManager *manager);
void          ev_tab_manager_select_prev (EvTabManager *manager);

/* Move the active tab to a new position in the list.  Used by the
 * notebook's drag-to-reorder tab UI.  The active index is updated
 * to reflect the new position. */
void          ev_tab_manager_reorder_tab (EvTabManager *manager,
                                          guint         old_index,
                                          guint         new_index);

G_END_DECLS
/* "Reopen closed tab" stack.  When a tab is removed, its document
 * is moved to the reopen stack (along with the page + scroll state).
 * ev_tab_manager_reopen_last_closed_tab() pops the most recent
 * entry and re-creates a tab with the saved state.  The stack
 * is bounded (max 10 entries) to prevent unbounded growth. */
void          ev_tab_manager_reopen_last_closed_tab (EvTabManager *manager);
guint         ev_tab_manager_get_reopen_stack_size  (EvTabManager *manager);
void          ev_tab_manager_clear_reopen_stack     (EvTabManager *manager);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (EvTabManager, g_object_unref)

G_END_DECLS

#endif /* EV_TAB_MANAGER_H */
