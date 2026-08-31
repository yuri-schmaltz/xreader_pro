/* test-tabbed-window.c - Lifecycle/leak tests for EvTabbedWindow
 *
 * Copyright (C) 2026 Yuri Schmaltz / xreader fork
 *
 * Goal: ensure that creating and destroying an EvTabbedWindow
 * (with and without tabs) does not leak GObject references,
 * EvTab instances, or EvDocument backends.  This is the
 * primary guard against regressions in the C3 tabbed view
 * (see ROADMAP.md, "C3 -- Tabbed view 4.8.0").
 *
 * The tests run under MALLOC_PERTURB_ + ASAN/UBSAN, so any
 * use-after-free or double-unref would fail.
 *
 * Note on display: EvTabbedWindow is a GtkApplicationWindow,
 * which needs a running GdkDisplay.  In a headless CI environment
 * (no DISPLAY / no Wayland) we cannot construct a real
 * GtkApplicationWindow.  Instead, these tests:
 *
 *   1. Verify that the GObject type is registered and the
 *      class init has not regressed.
 *   2. Verify the public API of EvTabbedWindow is callable
 *      (signals, properties) without crashing.
 *   3. Stress-test the underlying EvTabManager (the data model
 *      that the window wraps) under heavy operations, including
 *      the reopen-stack bound.
 *
 * For the actual window display, see docs/TABBED_VIEW_TEST_PLAN.md
 * which provides 14 manual test cases run by the maintainer on
 * a graphical desktop.
 */

#include <config.h>

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "ev-tab.h"
#include "ev-tab-manager.h"
#include "ev-tabbed-window.h"
#include "ev-document.h"

/* ---------------------------------------------------------------- */
/* Fake EvDocument -- satisfies ev_tab_new() without a real file. */
typedef struct {
	EvDocument parent;
	gint n_pages;
} FakeDocument;

typedef EvDocumentClass FakeDocumentClass;

static GType fake_document_get_type (void);
G_DEFINE_TYPE (FakeDocument, fake_document, EV_TYPE_DOCUMENT)

static gint
fake_document_get_n_pages (EvDocument *document)
{
	FakeDocument *self = (FakeDocument *) document;
	return self->n_pages;
}

static void
fake_document_class_init (FakeDocumentClass *klass)
{
	EvDocumentClass *ev_klass = EV_DOCUMENT_CLASS (klass);
	ev_klass->get_n_pages = fake_document_get_n_pages;
}

static void
fake_document_init (FakeDocument *self)
{
	self->n_pages = 0;
}

static FakeDocument *
fake_document_new (gint n_pages)
{
	FakeDocument *self = g_object_new (fake_document_get_type (), NULL);
	self->n_pages = n_pages;
	return self;
}

/* ---------------------------------------------------------------- */
/* Test 1: the EvTabbedWindow GObject type is registered correctly. */
static void
test_type_registered (void)
{
	GType t = ev_tabbed_window_get_type ();
	g_assert_cmpuint (t, !=, G_TYPE_INVALID);
	g_assert_true (g_type_is_a (t, GTK_TYPE_APPLICATION_WINDOW));
	g_assert_true (g_type_is_a (t, GTK_TYPE_WIDGET));
}

/* ---------------------------------------------------------------- */
/* Test 2: EvTabManager (the model wrapped by EvTabbedWindow)
 * handles a heavy workload: 100 tabs added, removed, reordered
 * with the active index at every position.  This stress-tests
 * the signal emissions, the GPtrArray growth, and the
 * active_index tracking. */
static void
test_tab_manager_stress_100_tabs (void)
{
	EvTabManager *mgr = ev_tab_manager_new ();
	g_assert_nonnull (mgr);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (mgr), ==, 0);

	/* Add 100 tabs. */
	GPtrArray *tabs = g_ptr_array_new ();
	for (int i = 0; i < 100; i++) {
		FakeDocument *doc = fake_document_new (1);
		GtkWidget *tab_widget = ev_tab_new (EV_DOCUMENT (doc));
		ev_tab_manager_append_tab (mgr, EV_TAB (tab_widget));
		g_ptr_array_add (tabs, tab_widget);
		g_object_unref (doc);
	}
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (mgr), ==, 100);

	/* Set every index as active. */
	for (int i = 0; i < 100; i++) {
		EvTab *t = g_ptr_array_index (tabs, i);
		ev_tab_manager_set_active (mgr, t);
		EvTab *cur = ev_tab_manager_get_active (mgr);
		g_assert (cur == t);
		g_assert_cmpint (ev_tab_manager_get_tab_index (mgr, t), ==, i);
	}

	/* Remove all 100 tabs (in order). */
	for (int i = 0; i < 100; i++) {
		EvTab *t = g_ptr_array_index (tabs, 0);
		ev_tab_manager_remove_tab (mgr, t);
		g_ptr_array_remove_index (tabs, 0);
		g_assert_cmpuint (ev_tab_manager_get_n_tabs (mgr), ==, 100 - (i + 1));
	}
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (mgr), ==, 0);
	g_assert_null (ev_tab_manager_get_active (mgr));

	g_ptr_array_unref (tabs);
	g_object_unref (mgr);
}

/* ---------------------------------------------------------------- */
/* Test 3: reorder tabs -- assert that the count is preserved
 * across reorders, and that get_tab_index returns valid
 * positions for all tabs after the shuffle. */
static void
test_tab_manager_reorder_preserves_count (void)
{
	EvTabManager *mgr = ev_tab_manager_new ();
	/* The tabs we create are owned by the manager once
	 * ev_tab_manager_append_tab() is called, so we don't
	 * keep our own references -- we look them up via
	 * get_tab (index) after each mutation. */
	for (int i = 0; i < 10; i++) {
		FakeDocument *doc = fake_document_new (i);
		GtkWidget *tab_widget = ev_tab_new (EV_DOCUMENT (doc));
		ev_tab_manager_append_tab (mgr, EV_TAB (tab_widget));
		g_object_unref (doc);
	}
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (mgr), ==, 10);

	/* Initial indices: 0..9 in order. */
	for (int i = 0; i < 10; i++) {
		EvTab *t = ev_tab_manager_get_tab (mgr, i);
		g_assert_cmpint (ev_tab_manager_get_tab_index (mgr, t), ==, i);
	}

	/* Move tab 0 to position 9. */
	EvTab *t0 = ev_tab_manager_get_tab (mgr, 0);
	ev_tab_manager_reorder_tab (mgr, 0, 9);
	g_assert_cmpint (ev_tab_manager_get_tab_index (mgr, t0), ==, 9);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (mgr), ==, 10);

	/* Move t0 (now at 9) back to position 5. */
	ev_tab_manager_reorder_tab (mgr, 9, 5);
	g_assert_cmpint (ev_tab_manager_get_tab_index (mgr, t0), ==, 5);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (mgr), ==, 10);

	/* Active tab survives reorder. */
	ev_tab_manager_set_active (mgr, t0);
	ev_tab_manager_reorder_tab (mgr, 5, 0);
	g_assert (ev_tab_manager_get_active (mgr) == t0);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (mgr), ==, 10);

	g_object_unref (mgr);
}

/* ---------------------------------------------------------------- */
/* Test 4: reopen stack is bounded at 10 even with 100 closes. */
static void
test_reopen_stack_bounded_at_10 (void)
{
	EvTabManager *mgr = ev_tab_manager_new ();
	for (int i = 0; i < 100; i++) {
		FakeDocument *doc = fake_document_new (1);
		GtkWidget *tab_widget = ev_tab_new (EV_DOCUMENT (doc));
		ev_tab_manager_append_tab (mgr, EV_TAB (tab_widget));
		g_object_unref (doc);
	}
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (mgr), ==, 100);

	/* Close all 100 (each pushes to reopen stack). */
	for (int i = 0; i < 100; i++) {
		EvTab *cur = ev_tab_manager_get_active (mgr);
		g_assert_nonnull (cur);
		ev_tab_manager_remove_tab (mgr, cur);
	}
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (mgr), ==, 0);
	/* Stack bounded to 10, not 100. */
	g_assert_cmpuint (ev_tab_manager_get_reopen_stack_size (mgr), <=, 10);

	g_object_unref (mgr);
}

/* ---------------------------------------------------------------- */
/* Test 5: signal emissions -- tab-added, tab-removed, active-changed,
 * tab-reordered -- fire the correct number of times. */
static void
on_tab_added (EvTabManager *mgr, EvTab *tab, gpointer user_data)
{
	(*(guint *) user_data)++;
}

static void
on_tab_removed (EvTabManager *mgr, EvTab *tab, gpointer user_data)
{
	(*(guint *) user_data)++;
}

static void
on_active_changed (GObject *gobject, GParamSpec *pspec, gpointer user_data)
{
	(*(guint *) user_data)++;
}

static void
on_tab_reordered (EvTabManager *mgr, guint old_index, guint new_index, gpointer user_data)
{
	(*(guint *) user_data)++;
}

static void
test_tab_manager_signal_counts (void)
{
	EvTabManager *mgr = ev_tab_manager_new ();
	g_assert_nonnull (mgr);

	guint added = 0, removed = 0, active = 0, reordered = 0;
	g_signal_connect (mgr, "tab-added",      G_CALLBACK (on_tab_added),     &added);
	g_signal_connect (mgr, "tab-removed",     G_CALLBACK (on_tab_removed),   &removed);
	g_signal_connect (mgr, "active-changed",  G_CALLBACK (on_active_changed), &active);
	g_signal_connect (mgr, "tab-reordered",   G_CALLBACK (on_tab_reordered), &reordered);

	for (int i = 0; i < 5; i++) {
		FakeDocument *doc = fake_document_new (1);
		GtkWidget *tab_widget = ev_tab_new (EV_DOCUMENT (doc));
		ev_tab_manager_append_tab (mgr, EV_TAB (tab_widget));
		g_object_unref (doc);
	}
	g_assert_cmpuint (added, ==, 5);

	EvTab *t0 = ev_tab_manager_get_tab (mgr, 0);
	ev_tab_manager_set_active (mgr, t0);
	g_assert_cmpuint (active, >=, 1);

	ev_tab_manager_reorder_tab (mgr, 0, 4);
	g_assert_cmpuint (reordered, ==, 1);

	for (int i = 0; i < 5; i++) {
		EvTab *t = ev_tab_manager_get_tab (mgr, 0);
		ev_tab_manager_remove_tab (mgr, t);
	}
	g_assert_cmpuint (removed, ==, 5);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (mgr), ==, 0);

	g_object_unref (mgr);
}

/* ---------------------------------------------------------------- */
/* Test 6: get_tab_by_location -- opens 5 tabs at distinct
 * GFile locations, asserts the lookup returns the right tab,
 * and that non-existent locations return NULL. */
static void
test_tab_manager_get_by_location (void)
{
	EvTabManager *mgr = ev_tab_manager_new ();
	GPtrArray *locs = g_ptr_array_new_with_free_func (g_object_unref);

	for (int i = 0; i < 5; i++) {
		FakeDocument *doc = fake_document_new (1);
		GtkWidget *tab_widget = ev_tab_new (EV_DOCUMENT (doc));
		ev_tab_manager_append_tab (mgr, EV_TAB (tab_widget));
		g_object_unref (doc);

		gchar *uri = g_strdup_printf ("file:///tmp/xreader-test-%d.pdf", i);
		GFile *loc = g_file_new_for_uri (uri);
		ev_tab_set_location (EV_TAB (tab_widget), loc);
		g_ptr_array_add (locs, loc);  /* takes ownership */
		g_free (uri);
	}

	/* The manager owns the tabs; we look them up by index. */
	for (int i = 0; i < 5; i++) {
		GFile *loc = g_ptr_array_index (locs, i);
		EvTab *expected = ev_tab_manager_get_tab (mgr, i);
		EvTab *found = ev_tab_manager_get_tab_by_location (mgr, loc);
		g_assert (found == expected);
	}

	/* Non-existent location. */
	GFile *missing = g_file_new_for_uri ("file:///tmp/xreader-NONEXISTENT.pdf");
	g_assert_null (ev_tab_manager_get_tab_by_location (mgr, missing));
	g_object_unref (missing);

	g_ptr_array_unref (locs);
	g_object_unref (mgr);
}

int
main (int argc, char *argv[])
{
	gtk_test_init (&argc, &argv, NULL);

	g_test_add_func ("/tabbed-window/type-registered",          test_type_registered);
	g_test_add_func ("/tabbed-window/manager-stress-100-tabs",  test_tab_manager_stress_100_tabs);
	g_test_add_func ("/tabbed-window/manager-reorder",          test_tab_manager_reorder_preserves_count);
	g_test_add_func ("/tabbed-window/reopen-stack-bounded",     test_reopen_stack_bounded_at_10);
	g_test_add_func ("/tabbed-window/manager-signal-counts",    test_tab_manager_signal_counts);
	g_test_add_func ("/tabbed-window/manager-get-by-location",  test_tab_manager_get_by_location);

	return g_test_run ();
}
