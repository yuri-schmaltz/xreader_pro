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
#include "ev-tab-manager.h"

#define EV_TAB_MANAGER_REOPEN_STACK_MAX 10

typedef struct {
	EvDocument *document;
	GFile      *location;
	gint        page;
} EvTabClosedEntry;

struct _EvTabManagerPrivate
{
	GPtrArray  *tabs;       /* GPtrArray<EvTab>, owned, with ref */
	gint        active_index; /* -1 = no active tab */
	GPtrArray  *reopen_stack; /* GPtrArray<EvTabClosedEntry>, bounded */
};

enum {
	SIGNAL_TAB_ADDED,
	SIGNAL_TAB_REMOVED,
	SIGNAL_ACTIVE_CHANGED,
	SIGNAL_TAB_REORDERED,
	N_SIGNALS,
};

static guint signals[N_SIGNALS];

static void
ev_tab_closed_entry_free (EvTabClosedEntry *entry)
{
	g_clear_object (&entry->document);
	g_clear_object (&entry->location);
	g_free (entry);
}

G_DEFINE_TYPE_WITH_PRIVATE (EvTabManager, ev_tab_manager, G_TYPE_OBJECT)

static void
ev_tab_manager_finalize (GObject *object)
{
	EvTabManager *manager = EV_TAB_MANAGER (object);

	if (manager->priv->tabs) {
		g_ptr_array_unref (manager->priv->tabs);
		manager->priv->tabs = NULL;
	}
	if (manager->priv->reopen_stack) {
		g_ptr_array_unref (manager->priv->reopen_stack);
		manager->priv->reopen_stack = NULL;
	}

	G_OBJECT_CLASS (ev_tab_manager_parent_class)->finalize (object);
}

static void
ev_tab_manager_init (EvTabManager *manager)
{
	manager->priv = ev_tab_manager_get_instance_private (manager);

	manager->priv->tabs = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
	manager->priv->active_index = -1;
	manager->priv->reopen_stack = g_ptr_array_new_with_free_func ((GDestroyNotify) ev_tab_closed_entry_free);
}

static void
ev_tab_manager_class_init (EvTabManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = ev_tab_manager_finalize;

	signals[SIGNAL_TAB_ADDED] = g_signal_new ("tab-added",
	                                          G_TYPE_FROM_CLASS (object_class),
	                                          G_SIGNAL_RUN_LAST,
	                                          0, NULL, NULL, NULL,
	                                          G_TYPE_NONE, 1,
	                                          EV_TYPE_TAB);

	signals[SIGNAL_TAB_REMOVED] = g_signal_new ("tab-removed",
	                                            G_TYPE_FROM_CLASS (object_class),
	                                            G_SIGNAL_RUN_LAST,
	                                            0, NULL, NULL, NULL,
	                                            G_TYPE_NONE, 1,
	                                            EV_TYPE_TAB);

	signals[SIGNAL_ACTIVE_CHANGED] = g_signal_new ("active-changed",
	                                              G_TYPE_FROM_CLASS (object_class),
	                                              G_SIGNAL_RUN_LAST,
	                                              0, NULL, NULL, NULL,
	                                              G_TYPE_NONE, 1,
	                                              EV_TYPE_TAB);

	signals[SIGNAL_TAB_REORDERED] = g_signal_new ("tab-reordered",
	                                              G_TYPE_FROM_CLASS (object_class),
	                                              G_SIGNAL_RUN_LAST,
	                                              0, NULL, NULL, NULL,
	                                              G_TYPE_NONE, 2,
	                                              G_TYPE_UINT, G_TYPE_UINT);
}

EvTabManager *
ev_tab_manager_new (void)
{
	return g_object_new (EV_TYPE_TAB_MANAGER, NULL);
}

guint
ev_tab_manager_get_n_tabs (EvTabManager *manager)
{
	g_return_val_if_fail (EV_IS_TAB_MANAGER (manager), 0);
	return manager->priv->tabs->len;
}

EvTab *
ev_tab_manager_get_active (EvTabManager *manager)
{
	g_return_val_if_fail (EV_IS_TAB_MANAGER (manager), NULL);

	if (manager->priv->active_index < 0 ||
	    (guint) manager->priv->active_index >= manager->priv->tabs->len)
		return NULL;

	return g_ptr_array_index (manager->priv->tabs, manager->priv->active_index);
}

void
ev_tab_manager_set_active (EvTabManager *manager,
                           EvTab        *tab)
{
	g_return_if_fail (EV_IS_TAB_MANAGER (manager));
	g_return_if_fail (EV_IS_TAB (tab));

	gint index = ev_tab_manager_get_tab_index (manager, tab);
	if (index < 0) {
		g_warning ("set_active: tab is not in the manager");
		return;
	}

	if (manager->priv->active_index == index)
		return;

	manager->priv->active_index = index;
	g_signal_emit (manager, signals[SIGNAL_ACTIVE_CHANGED], 0, tab);
}

EvTab *
ev_tab_manager_get_tab (EvTabManager *manager,
                        guint         index)
{
	g_return_val_if_fail (EV_IS_TAB_MANAGER (manager), NULL);

	if (index >= manager->priv->tabs->len)
		return NULL;
	return g_ptr_array_index (manager->priv->tabs, index);
}

gint
ev_tab_manager_get_tab_index (EvTabManager *manager,
                             EvTab        *tab)
{
	g_return_val_if_fail (EV_IS_TAB_MANAGER (manager), -1);
	g_return_val_if_fail (EV_IS_TAB (tab), -1);

	guint i;
	for (i = 0; i < manager->priv->tabs->len; i++) {
		if (g_ptr_array_index (manager->priv->tabs, i) == tab)
			return (gint) i;
	}
	return -1;
}

EvTab *
ev_tab_manager_get_tab_by_location (EvTabManager *manager,
                                    GFile        *location)
{
	g_return_val_if_fail (EV_IS_TAB_MANAGER (manager), NULL);
	g_return_val_if_fail (G_IS_FILE (location), NULL);

	guint i;
	for (i = 0; i < manager->priv->tabs->len; i++) {
		EvTab *t = g_ptr_array_index (manager->priv->tabs, i);
		GFile *tab_location = ev_tab_get_location (t);
		if (tab_location && g_file_equal (tab_location, location)) {
			g_object_unref (tab_location);
			return t;
		}
		g_clear_object (&tab_location);
	}
	return NULL;
}

EvTab *
ev_tab_manager_append_tab (EvTabManager *manager,
                           EvTab        *tab)
{
	g_return_val_if_fail (EV_IS_TAB_MANAGER (manager), NULL);
	g_return_val_if_fail (EV_IS_TAB (tab), NULL);

	g_object_ref_sink (tab);
	g_ptr_array_add (manager->priv->tabs, tab);

	/* New tab becomes the active one. */
	manager->priv->active_index = manager->priv->tabs->len - 1;

	g_signal_emit (manager, signals[SIGNAL_TAB_ADDED], 0, tab);
	g_signal_emit (manager, signals[SIGNAL_ACTIVE_CHANGED], 0, tab);

	return tab;
}

void
ev_tab_manager_remove_tab (EvTabManager *manager,
                           EvTab        *tab)
{
	g_return_if_fail (EV_IS_TAB_MANAGER (manager));
	g_return_if_fail (EV_IS_TAB (tab));

	gint index = ev_tab_manager_get_tab_index (manager, tab);
	if (index < 0) {
		g_warning ("remove_tab: tab is not in the manager");
		return;
	}

	guint n = manager->priv->tabs->len;
	gboolean was_active = (manager->priv->active_index == index);

	/* Save to the reopen stack before removing. */
	if (ev_tab_get_document (tab)) {
		EvTabClosedEntry *entry = g_new0 (EvTabClosedEntry, 1);
		entry->document = g_object_ref (ev_tab_get_document (tab));
		entry->location = ev_tab_get_location (tab);
		entry->page = ev_tab_get_page (tab);
		g_ptr_array_add (manager->priv->reopen_stack, entry);
		/* Bounded growth. */
		while (manager->priv->reopen_stack->len > EV_TAB_MANAGER_REOPEN_STACK_MAX) {
			g_ptr_array_remove_index (manager->priv->reopen_stack, 0);
		}
	}

	g_object_ref (tab);
	g_ptr_array_remove_index (manager->priv->tabs, index);
	g_signal_emit (manager, signals[SIGNAL_TAB_REMOVED], 0, tab);

	if (n == 1) {
		/* The removed tab was the only one. */
		manager->priv->active_index = -1;
		if (was_active) {
			g_signal_emit (manager, signals[SIGNAL_ACTIVE_CHANGED], 0,
			               (EvTab *) NULL);
		}
	} else if (was_active) {
		/* The removed tab was the active one.  Pick the previous
		 * tab; if there is no previous, pick the new "next" tab
		 * (which is at the same index, since we removed before
		 * picking). */
		guint new_active = (index > 0) ? (guint) (index - 1) : 0;
		if (new_active >= manager->priv->tabs->len)
			new_active = manager->priv->tabs->len - 1;
		manager->priv->active_index = (gint) new_active;
		EvTab *new_tab = g_ptr_array_index (manager->priv->tabs, new_active);
		g_signal_emit (manager, signals[SIGNAL_ACTIVE_CHANGED], 0, new_tab);
	} else if (manager->priv->active_index > index) {
		/* The active tab was after the removed one.  Decrement. */
		manager->priv->active_index--;
	}

	g_object_unref (tab);
}

void
ev_tab_manager_select_next (EvTabManager *manager)
{
	g_return_if_fail (EV_IS_TAB_MANAGER (manager));

	if (manager->priv->tabs->len < 2)
		return;

	guint next = ((guint) manager->priv->active_index + 1) % manager->priv->tabs->len;
	manager->priv->active_index = (gint) next;
	EvTab *tab = g_ptr_array_index (manager->priv->tabs, next);
	g_signal_emit (manager, signals[SIGNAL_ACTIVE_CHANGED], 0, tab);
}

void
ev_tab_manager_select_prev (EvTabManager *manager)
{
	g_return_if_fail (EV_IS_TAB_MANAGER (manager));

	if (manager->priv->tabs->len < 2)
		return;

	guint n = manager->priv->tabs->len;
	guint prev;
	if (manager->priv->active_index <= 0)
		prev = n - 1;
	else
		prev = (guint) manager->priv->active_index - 1;
	manager->priv->active_index = (gint) prev;
	EvTab *tab = g_ptr_array_index (manager->priv->tabs, prev);
	g_signal_emit (manager, signals[SIGNAL_ACTIVE_CHANGED], 0, tab);
}

void
ev_tab_manager_reorder_tab (EvTabManager *manager,
                            guint         old_index,
                            guint         new_index)
{
	g_return_if_fail (EV_IS_TAB_MANAGER (manager));

	if (old_index >= manager->priv->tabs->len)
		return;
	if (new_index >= manager->priv->tabs->len)
		return;
	if (old_index == new_index)
		return;

	gpointer tab = g_ptr_array_steal_index (manager->priv->tabs, old_index);
	g_ptr_array_insert (manager->priv->tabs, new_index, tab);

	/* Update the active index to reflect the new position. */
	if (manager->priv->active_index == (gint) old_index) {
		manager->priv->active_index = (gint) new_index;
	} else if (manager->priv->active_index > (gint) old_index &&
	           manager->priv->active_index <= (gint) new_index) {
		manager->priv->active_index--;
	} else if (manager->priv->active_index < (gint) old_index &&
	           manager->priv->active_index >= (gint) new_index) {
		manager->priv->active_index++;
	}

	g_signal_emit (manager, signals[SIGNAL_TAB_REORDERED], 0,
	               old_index, new_index);
}

void
ev_tab_manager_reopen_last_closed_tab (EvTabManager *manager)
{
	g_return_if_fail (EV_IS_TAB_MANAGER (manager));

	if (manager->priv->reopen_stack->len == 0)
		return;

	EvTabClosedEntry *entry = g_ptr_array_index (
		manager->priv->reopen_stack,
		manager->priv->reopen_stack->len - 1);

	EvTab *tab = EV_TAB (ev_tab_new (entry->document));
	if (entry->location)
		ev_tab_set_location (tab, entry->location);
	if (entry->page >= 0)
		ev_tab_set_page (tab, entry->page);

	ev_tab_manager_append_tab (manager, tab);

	g_ptr_array_remove_index (manager->priv->reopen_stack,
	                          manager->priv->reopen_stack->len - 1);
}

guint
ev_tab_manager_get_reopen_stack_size (EvTabManager *manager)
{
	g_return_val_if_fail (EV_IS_TAB_MANAGER (manager), 0);
	return manager->priv->reopen_stack->len;
}

void
ev_tab_manager_clear_reopen_stack (EvTabManager *manager)
{
	g_return_if_fail (EV_IS_TAB_MANAGER (manager));
	g_ptr_array_set_size (manager->priv->reopen_stack, 0);
}
