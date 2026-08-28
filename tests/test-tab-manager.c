/* test-tab-manager.c - Unit tests for EvTabManager
 *                     (added in PR #97).
 *
 * Copyright (C) 2026 Yuri Schmaltz / xreader fork
 *
 * EvTabManager owns a GPtrArray<EvTab> and tracks the
 * active tab index.  The tests cover the API contract
 * and the edge cases (remove active tab, remove the
 * only tab, reorder the active tab, etc.).
 *
 * The tests use the public EvTab API (no private
 * struct access), so they double as a "the public API
 * behaves as documented" regression guard.
 *
 * Note: ev_tab_new() requires an EvDocument.  The tests
 * create a real EvDocument with a fake backend (a
 * "text/plain" DocumentInfo with no pages), which is
 * enough to satisfy the constructor.
 */

#include <config.h>
#include <glib.h>
#include <gio/gio.h>

#include "ev-tab-manager.h"
#include "ev-tab.h"
#include "ev-document.h"

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
	return ((FakeDocument *) document)->n_pages;
}

static void
fake_document_class_init (FakeDocumentClass *klass)
{
	EvDocumentClass *ev_document_class = EV_DOCUMENT_CLASS (klass);
	ev_document_class->get_n_pages = fake_document_get_n_pages;
}

static void
fake_document_init (FakeDocument *doc)
{
	doc->n_pages = 1;
}

static EvDocument *
make_fake_document (void)
{
	return g_object_new (fake_document_get_type (), NULL);
}

static EvTab *
make_fake_tab (void)
{
	return EV_TAB (g_object_ref_sink (ev_tab_new (make_fake_document ())));
}

/* --- tests --- */

static void
test_new_empty (void)
{
	EvTabManager *m = ev_tab_manager_new ();
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (m), ==, 0);
	g_assert_null (ev_tab_manager_get_active (m));
	g_object_unref (m);
}

static void
test_append_tab (void)
{
	EvTabManager *m = ev_tab_manager_new ();
	g_autoptr(EvTab) t1 = make_fake_tab ();

	ev_tab_manager_append_tab (m, t1);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (m), ==, 1);
	g_assert (ev_tab_manager_get_active (m) == t1);
	g_object_unref (m);
}

static void
test_append_multiple (void)
{
	EvTabManager *m = ev_tab_manager_new ();
	g_autoptr(EvTab) t1 = make_fake_tab ();
	g_autoptr(EvTab) t2 = make_fake_tab ();
	g_autoptr(EvTab) t3 = make_fake_tab ();

	ev_tab_manager_append_tab (m, t1);
	ev_tab_manager_append_tab (m, t2);
	ev_tab_manager_append_tab (m, t3);

	g_assert_cmpuint (ev_tab_manager_get_n_tabs (m), ==, 3);
	/* New tab becomes active. */
	g_assert (ev_tab_manager_get_active (m) == t3);
	g_object_unref (m);
}

static void
test_set_active (void)
{
	EvTabManager *m = ev_tab_manager_new ();
	g_autoptr(EvTab) t1 = make_fake_tab ();
	g_autoptr(EvTab) t2 = make_fake_tab ();

	ev_tab_manager_append_tab (m, t1);
	ev_tab_manager_append_tab (m, t2);

	ev_tab_manager_set_active (m, t1);
	g_assert (ev_tab_manager_get_active (m) == t1);

	ev_tab_manager_set_active (m, t2);
	g_assert (ev_tab_manager_get_active (m) == t2);
	g_object_unref (m);
}

static void
test_get_tab_index (void)
{
	EvTabManager *m = ev_tab_manager_new ();
	g_autoptr(EvTab) t1 = make_fake_tab ();
	g_autoptr(EvTab) t2 = make_fake_tab ();

	ev_tab_manager_append_tab (m, t1);
	ev_tab_manager_append_tab (m, t2);

	g_assert_cmpint (ev_tab_manager_get_tab_index (m, t1), ==, 0);
	g_assert_cmpint (ev_tab_manager_get_tab_index (m, t2), ==, 1);

	g_autoptr(EvTab) outside = make_fake_tab ();
	g_assert_cmpint (ev_tab_manager_get_tab_index (m, outside), ==, -1);
	g_object_unref (m);
}

static void
test_remove_active_first (void)
{
	/* Remove the active tab when there are 2 tabs and the
	 * active one is first.  Expected: the other tab becomes
	 * active. */
	EvTabManager *m = ev_tab_manager_new ();
	g_autoptr(EvTab) t1 = make_fake_tab ();
	g_autoptr(EvTab) t2 = make_fake_tab ();

	ev_tab_manager_append_tab (m, t1);
	ev_tab_manager_append_tab (m, t2);
	ev_tab_manager_set_active (m, t1);

	ev_tab_manager_remove_tab (m, t1);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (m), ==, 1);
	g_assert (ev_tab_manager_get_active (m) == t2);
	g_object_unref (m);
}

static void
test_remove_active_last (void)
{
	/* Remove the active tab when it is the LAST tab.  Expected:
	 * the previous tab (which becomes the only one) becomes
	 * active. */
	EvTabManager *m = ev_tab_manager_new ();
	g_autoptr(EvTab) t1 = make_fake_tab ();
	g_autoptr(EvTab) t2 = make_fake_tab ();
	g_autoptr(EvTab) t3 = make_fake_tab ();

	ev_tab_manager_append_tab (m, t1);
	ev_tab_manager_append_tab (m, t2);
	ev_tab_manager_append_tab (m, t3);

	ev_tab_manager_remove_tab (m, t3);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (m), ==, 2);
	/* Active is now t2 (previous). */
	g_assert (ev_tab_manager_get_active (m) == t2);
	g_object_unref (m);
}

static void
test_remove_active_only (void)
{
	/* Remove the only tab.  Expected: active becomes NULL. */
	EvTabManager *m = ev_tab_manager_new ();
	g_autoptr(EvTab) t1 = make_fake_tab ();

	ev_tab_manager_append_tab (m, t1);
	ev_tab_manager_remove_tab (m, t1);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (m), ==, 0);
	g_assert_null (ev_tab_manager_get_active (m));
	g_object_unref (m);
}

static void
test_remove_non_active (void)
{
	/* Remove a tab that is NOT the active one.  Expected: the
	 * active tab remains the same; if the removed tab was before
	 * the active one, the active index decrements. */
	EvTabManager *m = ev_tab_manager_new ();
	g_autoptr(EvTab) t1 = make_fake_tab ();
	g_autoptr(EvTab) t2 = make_fake_tab ();
	g_autoptr(EvTab) t3 = make_fake_tab ();

	ev_tab_manager_append_tab (m, t1);
	ev_tab_manager_append_tab (m, t2);
	ev_tab_manager_append_tab (m, t3);
	ev_tab_manager_set_active (m, t3);

	/* Remove t1 (before t3).  Active was t3, now should still
	 * be t3, but at index 1 instead of 2. */
	ev_tab_manager_remove_tab (m, t1);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (m), ==, 2);
	g_assert (ev_tab_manager_get_active (m) == t3);
	g_assert_cmpint (ev_tab_manager_get_tab_index (m, t3), ==, 1);
	g_object_unref (m);
}

static void
test_select_next_wraps (void)
{
	/* select_next wraps from the last tab back to the first. */
	EvTabManager *m = ev_tab_manager_new ();
	g_autoptr(EvTab) t1 = make_fake_tab ();
	g_autoptr(EvTab) t2 = make_fake_tab ();
	g_autoptr(EvTab) t3 = make_fake_tab ();

	ev_tab_manager_append_tab (m, t1);
	ev_tab_manager_append_tab (m, t2);
	ev_tab_manager_append_tab (m, t3);

	ev_tab_manager_select_next (m);
	g_assert (ev_tab_manager_get_active (m) == t1);

	ev_tab_manager_select_next (m);
	g_assert (ev_tab_manager_get_active (m) == t2);

	ev_tab_manager_select_next (m);
	g_assert (ev_tab_manager_get_active (m) == t3);

	ev_tab_manager_select_next (m);
	g_assert (ev_tab_manager_get_active (m) == t1);
	g_object_unref (m);
}

static void
test_select_prev_wraps (void)
{
	/* select_prev wraps from the first tab back to the last. */
	EvTabManager *m = ev_tab_manager_new ();
	g_autoptr(EvTab) t1 = make_fake_tab ();
	g_autoptr(EvTab) t2 = make_fake_tab ();
	g_autoptr(EvTab) t3 = make_fake_tab ();

	ev_tab_manager_append_tab (m, t1);
	ev_tab_manager_append_tab (m, t2);
	ev_tab_manager_append_tab (m, t3);
	ev_tab_manager_set_active (m, t1);

	ev_tab_manager_select_prev (m);
	g_assert (ev_tab_manager_get_active (m) == t3);

	ev_tab_manager_select_prev (m);
	g_assert (ev_tab_manager_get_active (m) == t2);
	g_object_unref (m);
}

static void
test_select_next_no_op_single (void)
{
	/* select_next is a no-op when there's only 1 tab. */
	EvTabManager *m = ev_tab_manager_new ();
	g_autoptr(EvTab) t1 = make_fake_tab ();

	ev_tab_manager_append_tab (m, t1);
	ev_tab_manager_select_next (m);
	g_assert (ev_tab_manager_get_active (m) == t1);
	g_object_unref (m);
}

static void
test_reorder (void)
{
	/* Reorder t1 (index 0) to index 2.  After: t2, t3, t1. */
	EvTabManager *m = ev_tab_manager_new ();
	g_autoptr(EvTab) t1 = make_fake_tab ();
	g_autoptr(EvTab) t2 = make_fake_tab ();
	g_autoptr(EvTab) t3 = make_fake_tab ();

	ev_tab_manager_append_tab (m, t1);
	ev_tab_manager_append_tab (m, t2);
	ev_tab_manager_append_tab (m, t3);
	ev_tab_manager_set_active (m, t2);

	ev_tab_manager_reorder_tab (m, 0, 2);
	g_assert (ev_tab_manager_get_tab (m, 0) == t2);
	g_assert (ev_tab_manager_get_tab (m, 1) == t3);
	g_assert (ev_tab_manager_get_tab (m, 2) == t1);
	/* Active was t2 (index 1), now at index 0. */
	g_assert (ev_tab_manager_get_active (m) == t2);
	g_assert_cmpint (ev_tab_manager_get_tab_index (m, t2), ==, 0);
	g_object_unref (m);
}

static void
test_get_tab_by_location (void)
{
	/* Two tabs with different locations.  get_tab_by_location
	 * should return the matching one. */
	EvTabManager *m = ev_tab_manager_new ();
	g_autoptr(EvTab) t1 = make_fake_tab ();
	g_autoptr(EvTab) t2 = make_fake_tab ();

	GFile *loc1 = g_file_new_for_path ("/tmp/foo.pdf");
	GFile *loc2 = g_file_new_for_path ("/tmp/bar.pdf");

	ev_tab_set_location (t1, loc1);
	ev_tab_set_location (t2, loc2);

	ev_tab_manager_append_tab (m, t1);
	ev_tab_manager_append_tab (m, t2);

	g_assert (ev_tab_manager_get_tab_by_location (m, loc1) == t1);
	g_assert (ev_tab_manager_get_tab_by_location (m, loc2) == t2);

	GFile *loc3 = g_file_new_for_path ("/tmp/nonexistent.pdf");
	g_assert_null (ev_tab_manager_get_tab_by_location (m, loc3));

	g_object_unref (loc1);
	g_object_unref (loc2);
	g_object_unref (loc3);
	g_object_unref (m);
}

int
main (int argc, char *argv[])
{
	gtk_test_init (&argc, &argv, NULL);

	g_test_add_func ("/tab-manager/new-empty",           test_new_empty);
	g_test_add_func ("/tab-manager/append-tab",          test_append_tab);
	g_test_add_func ("/tab-manager/append-multiple",     test_append_multiple);
	g_test_add_func ("/tab-manager/set-active",          test_set_active);
	g_test_add_func ("/tab-manager/get-tab-index",       test_get_tab_index);
	g_test_add_func ("/tab-manager/remove-active-first", test_remove_active_first);
	g_test_add_func ("/tab-manager/remove-active-last",  test_remove_active_last);
	g_test_add_func ("/tab-manager/remove-active-only",  test_remove_active_only);
	g_test_add_func ("/tab-manager/remove-non-active",   test_remove_non_active);
	g_test_add_func ("/tab-manager/select-next-wraps",   test_select_next_wraps);
	g_test_add_func ("/tab-manager/select-prev-wraps",   test_select_prev_wraps);
	g_test_add_func ("/tab-manager/select-next-no-op",   test_select_next_no_op_single);
	g_test_add_func ("/tab-manager/reorder",             test_reorder);
	g_test_add_func ("/tab-manager/get-tab-by-location", test_get_tab_by_location);

	return g_test_run ();
}
