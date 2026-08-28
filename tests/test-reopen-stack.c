/* test-reopen-stack.c - Unit tests for the reopen-last-closed-tab feature
 *                      (added in PR #101).
 *
 * Copyright (C) 2026 Yuri Schmaltz / xreader fork
 *
 * The reopen stack lets the user restore a tab that was
 * recently closed (via Ctrl+Shift+T).  The stack is
 * bounded (max 10) to prevent unbounded growth.
 *
 * The tests cover:
 *   - Reopen on an empty stack is a no-op
 *   - Reopen restores the most recently closed tab
 *   - Reopen preserves the location, document, and page
 *   - The stack is bounded (max 10 entries; older entries
 *     are evicted first)
 *   - The stack can be cleared explicitly
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
make_fake_tab_with_location (const gchar *path)
{
	EvTab *tab = EV_TAB (g_object_ref_sink (ev_tab_new (make_fake_document ())));
	if (path) {
		GFile *file = g_file_new_for_path (path);
		ev_tab_set_location (tab, file);
		ev_tab_set_page (tab, 5);
		g_object_unref (file);
	}
	return tab;
}

static void
test_empty_reopen_noop (void)
{
	/* Reopen on an empty stack does nothing. */
	EvTabManager *m = ev_tab_manager_new ();
	ev_tab_manager_reopen_last_closed_tab (m);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (m), ==, 0);
	g_assert_cmpuint (ev_tab_manager_get_reopen_stack_size (m), ==, 0);
	g_object_unref (m);
}

static void
test_reopen_restores_tab (void)
{
	/* Close a tab, then reopen.  Expect: the tab is back, the
	 * location is preserved. */
	EvTabManager *m = ev_tab_manager_new ();
	g_autoptr(EvTab) t1 = make_fake_tab_with_location ("/tmp/foo.pdf");
	ev_tab_manager_append_tab (m, t1);

	ev_tab_manager_remove_tab (m, t1);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (m), ==, 0);
	g_assert_cmpuint (ev_tab_manager_get_reopen_stack_size (m), ==, 1);

	ev_tab_manager_reopen_last_closed_tab (m);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (m), ==, 1);
	g_assert_cmpuint (ev_tab_manager_get_reopen_stack_size (m), ==, 0);

	EvTab *restored = ev_tab_manager_get_active (m);
	g_assert_nonnull (restored);
	GFile *loc = ev_tab_get_location (restored);
	g_assert_nonnull (loc);
	gchar *path = g_file_get_path (loc);
	g_assert_cmpstr (path, ==, "/tmp/foo.pdf");
	g_free (path);
	g_object_unref (loc);

	g_object_unref (m);
}

static void
test_reopen_preserves_page (void)
{
	/* Reopen preserves the page number. */
	EvTabManager *m = ev_tab_manager_new ();
	g_autoptr(EvTab) t1 = make_fake_tab_with_location ("/tmp/bar.pdf");
	ev_tab_set_page (t1, 42);
	ev_tab_manager_append_tab (m, t1);
	ev_tab_manager_remove_tab (m, t1);
	ev_tab_manager_reopen_last_closed_tab (m);

	EvTab *restored = ev_tab_manager_get_active (m);
	g_assert_cmpint (ev_tab_get_page (restored), ==, 42);
	g_object_unref (m);
}

static void
test_reopen_lifo_order (void)
{
	/* Two tabs closed; reopen restores the most recent first. */
	EvTabManager *m = ev_tab_manager_new ();
	g_autoptr(EvTab) t1 = make_fake_tab_with_location ("/tmp/first.pdf");
	g_autoptr(EvTab) t2 = make_fake_tab_with_location ("/tmp/second.pdf");
	ev_tab_manager_append_tab (m, t1);
	ev_tab_manager_append_tab (m, t2);

	ev_tab_manager_remove_tab (m, t1);
	ev_tab_manager_remove_tab (m, t2);
	g_assert_cmpuint (ev_tab_manager_get_reopen_stack_size (m), ==, 2);

	/* First reopen: t2 (most recent). */
	ev_tab_manager_reopen_last_closed_tab (m);
	EvTab *r1 = ev_tab_manager_get_active (m);
	GFile *loc1 = ev_tab_get_location (r1);
	gchar *path1 = g_file_get_path (loc1);
	g_assert_cmpstr (path1, ==, "/tmp/second.pdf");
	g_free (path1);
	g_object_unref (loc1);

	/* Second reopen: t1. */
	ev_tab_manager_reopen_last_closed_tab (m);
	EvTab *r2 = ev_tab_manager_get_active (m);
	GFile *loc2 = ev_tab_get_location (r2);
	gchar *path2 = g_file_get_path (loc2);
	g_assert_cmpstr (path2, ==, "/tmp/first.pdf");
	g_free (path2);
	g_object_unref (loc2);

	g_object_unref (m);
}

static void
test_reopen_stack_bounded (void)
{
	/* Closing 15 tabs should leave only the most recent 10
	 * in the reopen stack. */
	EvTabManager *m = ev_tab_manager_new ();

	for (int i = 0; i < 15; i++) {
		gchar *path = g_strdup_printf ("/tmp/tab-%02d.pdf", i);
		g_autoptr(EvTab) t = make_fake_tab_with_location (path);
		ev_tab_manager_append_tab (m, t);
		ev_tab_manager_remove_tab (m, t);
		g_free (path);
	}

	g_assert_cmpuint (ev_tab_manager_get_reopen_stack_size (m), ==, 10);

	/* The oldest tab (i=0) should have been evicted; the
	 * newest (i=14) should be at the top. */
	ev_tab_manager_reopen_last_closed_tab (m);
	EvTab *top = ev_tab_manager_get_active (m);
	GFile *loc = ev_tab_get_location (top);
	gchar *path = g_file_get_path (loc);
	g_assert_cmpstr (path, ==, "/tmp/tab-14.pdf");
	g_free (path);
	g_object_unref (loc);

	g_object_unref (m);
}

static void
test_clear_reopen_stack (void)
{
	/* clear_reopen_stack empties the stack. */
	EvTabManager *m = ev_tab_manager_new ();
	g_autoptr(EvTab) t1 = make_fake_tab_with_location ("/tmp/foo.pdf");
	ev_tab_manager_append_tab (m, t1);
	ev_tab_manager_remove_tab (m, t1);
	g_assert_cmpuint (ev_tab_manager_get_reopen_stack_size (m), ==, 1);

	ev_tab_manager_clear_reopen_stack (m);
	g_assert_cmpuint (ev_tab_manager_get_reopen_stack_size (m), ==, 0);

	ev_tab_manager_reopen_last_closed_tab (m);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (m), ==, 0);
	g_object_unref (m);
}

int
main (int argc, char *argv[])
{
	gtk_test_init (&argc, &argv, NULL);

	g_test_add_func ("/reopen/empty-noop",          test_empty_reopen_noop);
	g_test_add_func ("/reopen/restores-tab",        test_reopen_restores_tab);
	g_test_add_func ("/reopen/preserves-page",      test_reopen_preserves_page);
	g_test_add_func ("/reopen/lifo-order",          test_reopen_lifo_order);
	g_test_add_func ("/reopen/stack-bounded",       test_reopen_stack_bounded);
	g_test_add_func ("/reopen/clear-stack",         test_clear_reopen_stack);

	return g_test_run ();
}
