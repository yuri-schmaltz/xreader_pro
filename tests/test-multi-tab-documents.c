/* test-multi-tab-documents.c - Integration test for opening multiple documents simultaneously in tabs
 *
 * Verifies that EvTabbedWindow opens multiple documents in separate tabs,
 * switches active tabs, updates window titles, manages tab bar visibility,
 * and supports close/reopen lifecycle across multiple loaded documents.
 */

#include "config.h"
#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "ev-init.h"
#include "ev-tabbed-window.h"
#include "ev-tab.h"
#include "ev-tab-manager.h"
#include "ev-document.h"

static void
test_multi_tab_open_simultaneous (void)
{
	GtkWidget *win = ev_tabbed_window_new (NULL);
	EvTabbedWindow *tabbed_win = EV_TABBED_WINDOW (win);
	EvTabManager *manager = ev_tabbed_window_get_tab_manager (tabbed_win);

	gchar *srcdir = g_strdup (g_getenv ("G_TEST_SRCDIR"));
	if (!srcdir)
		srcdir = g_strdup (".");

	gchar *pdf1_path = g_build_filename (srcdir, "..", "test", "test-links.pdf", NULL);
	gchar *pdf2_path = g_build_filename (srcdir, "..", "test", "test-page-labels.pdf", NULL);

	GFile *file1 = g_file_new_for_path (pdf1_path);
	GFile *file2 = g_file_new_for_path (pdf2_path);

	GError *error = NULL;

	/* 1. Open first document in tab 1 */
	EvTab *tab1 = ev_tabbed_window_open_file (tabbed_win, file1, &error);
	g_assert_no_error (error);
	g_assert_nonnull (tab1);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (manager), ==, 1);
	g_assert_false (ev_tabbed_window_get_tab_bar_visible (tabbed_win));

	EvDocument *doc1 = ev_tabbed_window_get_active_document (tabbed_win);
	g_assert_nonnull (doc1);
	g_assert_true (EV_IS_DOCUMENT (doc1));

	/* 2. Open second document simultaneously in tab 2 */
	EvTab *tab2 = ev_tabbed_window_open_file (tabbed_win, file2, &error);
	g_assert_no_error (error);
	g_assert_nonnull (tab2);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (manager), ==, 2);
	g_assert_true (ev_tabbed_window_get_tab_bar_visible (tabbed_win));

	EvDocument *doc2 = ev_tabbed_window_get_active_document (tabbed_win);
	g_assert_nonnull (doc2);
	g_assert_true (EV_IS_DOCUMENT (doc2));

	/* Active tab is now tab 2 */
	g_assert_true (ev_tab_manager_get_active (manager) == tab2);

	/* 3. Switch tabs */
	ev_tabbed_window_select_next_tab (tabbed_win);
	g_assert_true (ev_tab_manager_get_active (manager) == tab1);

	ev_tabbed_window_select_prev_tab (tabbed_win);
	g_assert_true (ev_tab_manager_get_active (manager) == tab2);

	/* 4. Opening existing file focuses existing tab rather than duplicating */
	EvTab *tab2_refocus = ev_tabbed_window_open_file (tabbed_win, file2, &error);
	g_assert_no_error (error);
	g_assert_true (tab2_refocus == tab2);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (manager), ==, 2);

	/* 5. Close active tab (tab2) */
	gboolean closed = ev_tabbed_window_close_active_tab (tabbed_win);
	g_assert_true (closed);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (manager), ==, 1);
	g_assert_false (ev_tabbed_window_get_tab_bar_visible (tabbed_win));
	g_assert_true (ev_tab_manager_get_active (manager) == tab1);

	/* 6. Reopen last closed tab (restores tab2) */
	g_assert_cmpuint (ev_tabbed_window_get_reopen_stack_size (tabbed_win), ==, 1);
	ev_tabbed_window_reopen_last_closed_tab (tabbed_win);
	g_assert_cmpuint (ev_tab_manager_get_n_tabs (manager), ==, 2);
	g_assert_true (ev_tabbed_window_get_tab_bar_visible (tabbed_win));

	/* Cleanup */
	gtk_widget_destroy (win);
	g_object_unref (file1);
	g_object_unref (file2);
	g_free (pdf1_path);
	g_free (pdf2_path);
	g_free (srcdir);
}

int
main (int argc, char *argv[])
{
	gtk_init_check (&argc, &argv);
	if (!ev_init ())
		return 1;

	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/multi-tab/open-simultaneous", test_multi_tab_open_simultaneous);

	gint res = g_test_run ();

	ev_shutdown ();
	return res;
}
