/* this file is part of xreader, a generic document viewer
 *
 *  Copyright (C) 2004 Martin Kretzschmar
 *  Copyright © 2010 Christian Persch
 *
 *  Author:
 *    Martin Kretzschmar <martink@gnome.org>
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
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <libxapp/xapp-dark-mode-manager.h>

#include "totem-scrsaver.h"

#include "eggsmclient.h"

#include "ev-application.h"
#include "ev-file-helpers.h"
#include "ev-stock-icons.h"
#include "ev-tabbed-window.h"

#ifdef ENABLE_DBUS
#include "ev-gdbus-generated.h"
#endif /* ENABLE_DBUS */

gchar **supported_mimetypes;

struct _EvApplication {
	GtkApplication         base_instance;

	gchar                 *uri;
	gchar                 *dot_dir;

#ifdef ENABLE_DBUS
    GDBusConnection       *connection;
    EvXreaderApplication  *skeleton;
    gboolean               doc_registered;
#endif

	TotemScrsaver         *scr_saver;
	EggSMClient           *smclient;

    XAppDarkModeManager *dark_mode_manager;
};

struct _EvApplicationClass {
	GtkApplicationClass   base_class;
};

G_DEFINE_TYPE (EvApplication, ev_application, GTK_TYPE_APPLICATION)

#ifdef ENABLE_DBUS
#define APPLICATION_DBUS_OBJECT_PATH  "/org/x/reader/Xreader"
#define APPLICATION_DBUS_INTERFACE    "org.x.reader.Application"

#define XREADER_DAEMON_SERVICE        "org.x.reader.Daemon"
#define XREADER_DAEMON_OBJECT_PATH    "/org/x/reader/Daemon"
#define XREADER_DAEMON_INTERFACE      "org.x.reader.Daemon"
#endif

static void _ev_application_open_uri_at_dest (EvApplication   *application,
                                              const gchar     *uri,
                                              GdkScreen       *screen,
                                              EvLinkDest      *dest,
                                              EvWindowRunMode  mode,
                                              const gchar     *search_string,
                                              guint            timestamp);
static void ev_application_open_uri_in_window (EvApplication  *application,
                                               const char     *uri,
                                               EvWindow       *ev_window,
                                               GdkScreen      *screen,
                                               EvLinkDest     *dest,
                                               EvWindowRunMode mode,
                                               const gchar    *search_string,
                                               guint           timestamp);

/**
 * ev_application_new:
 *
 * Creates a new #EvApplication instance.
 *
 * Returns: (transfer full): a newly created #EvApplication
 */
EvApplication *
ev_application_new (void)
{
  const GApplicationFlags flags = G_APPLICATION_NON_UNIQUE;

  return g_object_new (EV_TYPE_APPLICATION,
                       "application-id", NULL,
                       "flags", flags,
                       NULL);
}

/* Session */
gboolean
ev_application_load_session (EvApplication *application)
{
    GKeyFile *state_file;
    gchar    *uri;
    gchar    *uris;
    gchar   **uri_list;
    guint     active_index = 0;
    gint      i;

#ifdef WITH_SMCLIENT
    if (egg_sm_client_is_resumed (application->smclient)) {
        state_file = egg_sm_client_get_state_file (application->smclient);
        if (!state_file)
            return FALSE;
    } else
#endif /* WITH_SMCLIENT */
        return FALSE;

    /* New format (4.8.0+): 'uris' (CSV) + 'active-index' (uint).
     * Old format (4.7.0): single 'uri'.  Read both, prefer the new. */
    uris = g_key_file_get_string (state_file, "Xreader", "uris", NULL);
    if (uris) {
        uri_list = g_strsplit (uris, ",", 0);
        g_free (uris);
        if (g_key_file_has_key (state_file, "Xreader", "active-index", NULL))
            active_index = g_key_file_get_integer (
                state_file, "Xreader", "active-index", NULL);
    } else {
        uri = g_key_file_get_string (state_file, "Xreader", "uri", NULL);
        if (!uri) {
            g_key_file_free (state_file);
            return FALSE;
        }
        uri_list = g_new0 (gchar *, 2);
        uri_list[0] = uri;
    }

    /* Create the window once (so all tabs go into the same window
     * in tabbed mode).  In single-window mode, only the first
     * URI is opened (the rest are ignored, matching the 4.7.0
     * behavior). */
    GSettings *settings = g_settings_new ("org.x.reader");
    gboolean   tabbed = g_settings_get_boolean (settings, "tabbed-mode");
    g_object_unref (settings);

    if (tabbed && g_strv_length (uri_list) > 0) {
        GtkWidget *window = ev_application_create_window (application);
        for (i = 0; uri_list[i] != NULL; i++) {
            GFile *file = g_file_new_for_uri (uri_list[i]);
            ev_tabbed_window_open_file (EV_TABBED_WINDOW (window), file, NULL);
            g_object_unref (file);
        }
        EvTabManager *manager = ev_tabbed_window_get_tab_manager (EV_TABBED_WINDOW (window));
        if (manager) {
            EvTab *tab = ev_tab_manager_get_tab (manager, active_index);
            if (tab)
                ev_tab_manager_set_active (manager, tab);
        }
        gtk_widget_show (window);
    } else {
        /* Legacy path: open the first URI. */
        ev_application_open_uri_at_dest (application, uri_list[0],
                                         gdk_screen_get_default (),
                                         NULL, 0, NULL,
                                         GDK_CURRENT_TIME);
    }

    g_strfreev (uri_list);
    g_key_file_free (state_file);

    return TRUE;
}

#ifdef WITH_SMCLIENT

static void
smclient_save_state_cb (EggSMClient   *client,
                        GKeyFile      *state_file,
                        EvApplication *application)
{
    /* New format (4.8.0+): 'uris' (CSV) + 'active-index' (uint)
     * covers the tabbed view.  Old format (4.7.0): single 'uri'.
     * Write both: the new fields when the application has tabbed
     * windows, the old field as a fallback for the single-window
     * case. */
    if (application->uri) {
        g_key_file_set_string (state_file, "Xreader", "uri", application->uri);
    }

    /* Save the tabbed view state (if any tabbed windows are open). */
    GList *windows = gtk_application_get_windows (GTK_APPLICATION (application));
    GList *l;
    for (l = windows; l != NULL; l = l->next) {
        if (!EV_IS_TABBED_WINDOW (l->data))
            continue;

        EvTabbedWindow *tw = EV_TABBED_WINDOW (l->data);
        EvTabManager  *mgr = ev_tabbed_window_get_tab_manager (tw);
        guint n = ev_tab_manager_get_n_tabs (mgr);
        if (n == 0)
            continue;

        GString *uris_csv = g_string_new ("");
        guint i;
        for (i = 0; i < n; i++) {
            EvTab *tab = ev_tab_manager_get_tab (mgr, i);
            GFile *loc = ev_tab_get_location (tab);
            if (loc) {
                gchar *uri_str = g_file_get_uri (loc);
                if (i > 0)
                    g_string_append_c (uris_csv, ',');
                g_string_append (uris_csv, uri_str);
                g_free (uri_str);
                g_object_unref (loc);
            }
        }
        if (uris_csv->len > 0) {
            g_key_file_set_string (state_file, "Xreader", "uris",
                                  uris_csv->str);
            g_key_file_set_integer (state_file, "Xreader", "active-index",
                                    (gint) ev_tab_manager_get_tab_index (
                                        mgr, ev_tab_manager_get_active (mgr)));
        }
        g_string_free (uris_csv, TRUE);
        break;  /* Only the first tabbed window is restored. */
    }
}

static void
smclient_quit_cb (EggSMClient   *client,
                  GApplication *application)
{
    g_application_quit (application);
}
#endif /* WITH_SMCLIENT */

static void
ev_application_init_session (EvApplication *application)
{
#ifdef WITH_SMCLIENT
    application->smclient = egg_sm_client_get ();
    g_signal_connect (application->smclient, "save_state",
                      G_CALLBACK (smclient_save_state_cb),
                      application);
    g_signal_connect (application->smclient, "quit",
                      G_CALLBACK (smclient_quit_cb),
                      application);
#endif /* WITH_SMCLIENT */
}

#ifdef ENABLE_DBUS
/**
 * ev_display_open_if_needed:
 * @name: the name of the display to be open if it's needed.
 *
 * Search among all the open displays if any of them have the same name as the
 * passed name. If the display isn't found it tries the open it.
 *
 * Returns: a #GdkDisplay of the display with the passed name.
 */
static GdkDisplay *
ev_display_open_if_needed (const gchar *name)
{
    GSList     *displays;
    GSList     *l;
    GdkDisplay *display = NULL;

    displays = gdk_display_manager_list_displays (gdk_display_manager_get ());

    for (l = displays; l != NULL; l = l->next) {
        const gchar *display_name = gdk_display_get_name ((GdkDisplay *) l->data);

        if (g_ascii_strcasecmp (display_name, name) == 0) {
            display = l->data;
            break;
        }
    }

    g_slist_free (displays);

    return display != NULL ? display : gdk_display_open (name);
}
#endif /* ENABLE_DBUS */

static void
ev_spawn (const char     *uri,
          GdkScreen      *screen,
          EvLinkDest     *dest,
          EvWindowRunMode mode,
          const gchar    *search_string,
          guint           timestamp)
{
    /* The same fix as commit 50052ea / PR #1:  build argv[]
     * directly instead of routing through a command-line string
     * and g_app_info_create_from_commandline().  The arguments
     * below come from the document (page labels and named
     * destinations in PDF / DVI outlines, search strings
     * forwarded from a "find" link) so they are user-influenced
     * even if a separate process: a page label of
     * "' ; rm -rf ~ ; echo '" would previously have been
     * "shell-quoted" and passed through g_shell_quote()'s
     * converter, but g_app_info_create_from_commandline() re-runs
     * a parser on the result, and any mismatch between the
     * quoter and the parser is a path-injection bug.  Build the
     * array, spawn it, no shell.
     */
    gchar      **argv;
    gchar       *path;
    gint         argc = 0;
    GError      *error = NULL;
    gchar       *page_arg = NULL;
    gchar       *find_arg = NULL;
    const gchar *mode_arg = NULL;
    const gchar *display_name = NULL;
    gchar      **child_env = NULL;
    GdkDisplay  *display;

    path = g_build_filename (BINDIR, "xreader", NULL);
    if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
        g_free (path);
        path = g_file_read_link ("/proc/self/exe", NULL);
        if (!path)
            path = g_find_program_in_path ("xreader");
        if (!path)
            path = g_strdup ("xreader");
    }

    /* Page label or index */
    if (dest) {
        switch (ev_link_dest_get_dest_type (dest)) {
            case EV_LINK_DEST_TYPE_PAGE_LABEL: {
                const gchar *label = ev_link_dest_get_page_label (dest);
                if (label && *label)
                    page_arg = g_strconcat ("--page-label=", label, NULL);
                break;
            }
            case EV_LINK_DEST_TYPE_PAGE:
                page_arg = g_strdup_printf ("--page-index=%d",
                                            ev_link_dest_get_page (dest) + 1);
                break;
            case EV_LINK_DEST_TYPE_NAMED: {
                const gchar *named = ev_link_dest_get_named_dest (dest);
                if (named && *named)
                    page_arg = g_strconcat ("--named-dest=", named, NULL);
                break;
            }
            default:
                break;
        }
    }

    /* Find string */
    if (search_string && *search_string) {
        find_arg = g_strconcat ("--find=", search_string, NULL);
    }

    /* Mode */
    switch (mode) {
        case EV_WINDOW_MODE_FULLSCREEN:
            mode_arg = "-f";
            break;
        case EV_WINDOW_MODE_PRESENTATION:
            mode_arg = "-s";
            break;
        default:
            break;
    }

    /* argv[] = { path, page_arg?, find_arg?, mode_arg?, uri?, NULL }.
     * Worst case: 1 (path) + 1 (page) + 1 (find) + 1 (mode) + 1 (uri) + 1 (NULL) = 6. */
    argv = g_new0 (gchar *, 6);
    argv[argc++] = path;
    if (page_arg)  argv[argc++] = page_arg;
    if (find_arg)  argv[argc++] = find_arg;
    if (mode_arg)  argv[argc++] = g_strdup (mode_arg);
    if (uri)       argv[argc++] = g_strdup (uri);
    argv[argc] = NULL;

    /* The original code passed `screen` and `timestamp` through
     * gdk_app_launch_context.  The child picks its display from
     * $DISPLAY, so we override that env var when the caller asked
     * for a non-default screen; the timestamp goes through the
     * DESKTOP_STARTUP_ID env var which is what the freedesktop
     * startup-notification spec also uses.
     */
    if (screen) {
        display = gdk_screen_get_display (screen);
        if (display)
            display_name = gdk_display_get_name (display);
    }
    if (display_name) {
        child_env = g_new0 (gchar *, 3);
        child_env[0] = g_strdup_printf ("DISPLAY=%s", display_name);
    }
    if (timestamp != 0 && timestamp != GDK_CURRENT_TIME) {
        gchar *startup_id = g_strdup_printf ("XREADER_STARTUP_ID=%u", timestamp);
        if (!child_env) {
            child_env = g_new0 (gchar *, 2);
            child_env[0] = startup_id;
        } else {
            child_env[1] = startup_id;
        }
    }

    if (!g_spawn_async (NULL, argv, child_env,
                        G_SPAWN_SEARCH_PATH,
                        NULL, NULL, NULL, &error)) {
        g_printerr ("Error launching xreader %s: %s\n",
                    uri ? uri : "", error->message);
        g_error_free (error);
    }

    g_strfreev (argv);
    g_strfreev (child_env);
}


static EvWindow *
ev_application_get_empty_window (EvApplication *application,
                                 GdkScreen     *screen)
{
    EvWindow *empty_window = NULL;
	GList    *windows;
    GList    *l;

    windows = gtk_application_get_windows (GTK_APPLICATION (application));
    for (l = windows; l != NULL; l = l->next) {
		EvWindow *window;

        if (!EV_IS_WINDOW (l->data))
            continue;

        window = EV_WINDOW (l->data);

        if (ev_window_is_empty (window) &&
            gtk_window_get_screen (GTK_WINDOW (window)) == screen) {
            empty_window = window;
            break;
        }
    }

    return empty_window;
}


#ifdef ENABLE_DBUS
typedef struct {
    gchar          *uri;
    GdkScreen      *screen;
    EvLinkDest     *dest;
    EvWindowRunMode mode;
    gchar          *search_string;
    guint           timestamp;
} EvRegisterDocData;

static void
ev_register_doc_data_free (EvRegisterDocData *data)
{
    if (!data)
        return;

    g_free (data->uri);
    if (data->search_string)
        g_free (data->search_string);
    if (data->dest)
        g_object_unref (data->dest);

    g_free (data);
}

static void
on_reload_cb (GObject      *source_object,
              GAsyncResult *res,
              gpointer      user_data)
{
    GDBusConnection *connection = G_DBUS_CONNECTION (source_object);
    GVariant        *value;
    GError          *error = NULL;

    g_application_release (g_application_get_default ());

    value = g_dbus_connection_call_finish (connection, res, &error);
    if (value != NULL) {
        g_variant_unref (value);
    } else {
        g_printerr ("Failed to Reload: %s\n", error->message);
        g_error_free (error);
    }

    /* We did not open a window, so manually clear the startup
     * notification. */
    gdk_notify_startup_complete ();
}

static void
on_register_uri_cb (GObject      *source_object,
                    GAsyncResult *res,
                    gpointer      user_data)
{
    GDBusConnection   *connection = G_DBUS_CONNECTION (source_object);
    EvRegisterDocData *data = (EvRegisterDocData *)user_data;
    EvApplication     *application = EV_APP;
    GVariant          *value;
    const gchar       *owner;
    GVariantBuilder    builder;
    GError            *error = NULL;

    g_application_release (G_APPLICATION (application));

    value = g_dbus_connection_call_finish (connection, res, &error);
    if (!value) {
		g_printerr ("Error registering document: %s\n", error->message);
        g_error_free (error);

        _ev_application_open_uri_at_dest (application,
                                          data->uri,
                                          data->screen,
                                          data->dest,
                                          data->mode,
                                          data->search_string,
                                          data->timestamp);
        ev_register_doc_data_free (data);

        return;
    }

    g_variant_get (value, "(&s)", &owner);

    /* This means that the document wasn't already registered; go
     * ahead with opening it.
     */
    if (owner[0] == '\0') {
        g_variant_unref (value);

        application->doc_registered = TRUE;

        _ev_application_open_uri_at_dest (application,
                                          data->uri,
                                          data->screen,
                                          data->dest,
                                          data->mode,
                                          data->search_string,
                                          data->timestamp);
        ev_register_doc_data_free (data);

        return;
    }

    /* Already registered */
    g_variant_builder_init (&builder, G_VARIANT_TYPE ("(a{sv}u)"));
    g_variant_builder_open (&builder, G_VARIANT_TYPE ("a{sv}"));
    g_variant_builder_add (&builder, "{sv}",
                           "display",
                           g_variant_new_string (gdk_display_get_name (gdk_screen_get_display (data->screen))));
    g_variant_builder_add (&builder, "{sv}",
                           "screen",
                           g_variant_new_int32 (gdk_screen_get_number (data->screen)));
    if (data->dest) {
        switch (ev_link_dest_get_dest_type (data->dest)) {
            case EV_LINK_DEST_TYPE_PAGE_LABEL:
                g_variant_builder_add (&builder, "{sv}", "page-label",
                                       g_variant_new_string (ev_link_dest_get_page_label (data->dest)));
                break;
            case EV_LINK_DEST_TYPE_PAGE:
                g_variant_builder_add (&builder, "{sv}", "page-index",
                                       g_variant_new_uint32 (ev_link_dest_get_page (data->dest)));
                break;
            case EV_LINK_DEST_TYPE_NAMED:
                g_variant_builder_add (&builder, "{sv}", "named-dest",
                                       g_variant_new_string (ev_link_dest_get_named_dest (data->dest)));
                break;
            default:
                break;
        }
    }
    if (data->search_string) {
        g_variant_builder_add (&builder, "{sv}",
                               "find-string",
                               g_variant_new_string (data->search_string));
    }
    if (data->mode != EV_WINDOW_MODE_NORMAL) {
        g_variant_builder_add (&builder, "{sv}",
                               "mode",
                               g_variant_new_uint32 (data->mode));
    }
    g_variant_builder_close (&builder);

    g_variant_builder_add (&builder, "u", data->timestamp);

    g_dbus_connection_call (connection,
                            owner,
                            APPLICATION_DBUS_OBJECT_PATH,
                            APPLICATION_DBUS_INTERFACE,
                            "Reload",
                            g_variant_builder_end (&builder),
                            NULL,
                            G_DBUS_CALL_FLAGS_NONE,
                            -1,
                            NULL,
                            on_reload_cb,
                            NULL);
    g_application_hold (G_APPLICATION (application));
    g_variant_unref (value);
    ev_register_doc_data_free (data);
}

/*
 * ev_application_register_uri:
 * @application:
 * @uri:
 * @screen:
 * @dest:
 * @mode:
 * @search_string:
 * @timestamp:
 *
 * Registers @uri with xreader-daemon.
 *
 */
static void
ev_application_register_uri (EvApplication  *application,
                             const gchar    *uri,
                             GdkScreen      *screen,
                             EvLinkDest     *dest,
                             EvWindowRunMode mode,
                             const gchar    *search_string,
                             guint           timestamp)
{
    EvRegisterDocData *data;

    if (!application->skeleton)
        return;

    if (application->doc_registered) {
        /* Already registered, reload */
        GList *windows, *l;

        windows = gtk_application_get_windows (GTK_APPLICATION (application));
        for (l = windows; l != NULL; l = g_list_next (l)) {
            if (!EV_IS_WINDOW (l->data))
                continue;

            ev_application_open_uri_in_window (application, uri,
                                               EV_WINDOW (l->data),
                                               screen, dest, mode,
                                               search_string,
                                               timestamp);
        }

        return;
    }

    data = g_new (EvRegisterDocData, 1);
    data->uri = g_strdup (uri);
    data->screen = screen;
    data->dest = dest ? g_object_ref (dest) : NULL;
    data->mode = mode;
    data->search_string = search_string ? g_strdup (search_string) : NULL;
    data->timestamp = timestamp;

    g_dbus_connection_call (g_application_get_dbus_connection (G_APPLICATION (application)),
                            XREADER_DAEMON_SERVICE,
                            XREADER_DAEMON_OBJECT_PATH,
                            XREADER_DAEMON_INTERFACE,
                            "RegisterDocument",
                            g_variant_new ("(s)", uri),
                            G_VARIANT_TYPE ("(s)"),
                            G_DBUS_CALL_FLAGS_NONE,
                            -1,
                            NULL,
                            on_register_uri_cb,
                            data);

    g_application_hold (G_APPLICATION (application));
}

static void
ev_application_unregister_uri (EvApplication *application,
                               const gchar   *uri)
{
    GVariant *value;
    GError   *error = NULL;

    if (!application->doc_registered)
        return;

    /* This is called from ev_application_shutdown(),
     * so it's safe to use the sync api
     */
    value = g_dbus_connection_call_sync (g_application_get_dbus_connection (G_APPLICATION (application)),
                                         XREADER_DAEMON_SERVICE,
                                         XREADER_DAEMON_OBJECT_PATH,
                                         XREADER_DAEMON_INTERFACE,
                                         "UnregisterDocument",
                                         g_variant_new ("(s)", uri),
                                         NULL,
                                         G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                         -1,
                                         NULL,
                                         &error);
    if (value == NULL) {
		g_printerr ("Error unregistering document: %s\n", error->message);
        g_error_free (error);
    } else {
        g_variant_unref (value);
    }
}
#endif /* ENABLE_DBUS */

static void
ev_application_open_uri_in_window (EvApplication  *application,
                                   const char     *uri,
                                   EvWindow       *ev_window,
                                   GdkScreen      *screen,
                                   EvLinkDest     *dest,
                                   EvWindowRunMode mode,
                                   const gchar    *search_string,
                                   guint           timestamp)
{
    if (uri == NULL)
        uri = application->uri;

    if (screen) {
        ev_stock_icons_set_screen (screen);
        gtk_window_set_screen (GTK_WINDOW (ev_window), screen);
    }

    /* We need to load uri before showing the window, so
       we can restore window size without flickering */
    if (!gtk_widget_get_realized (GTK_WIDGET (ev_window))) {
        gtk_widget_hide(GTK_WIDGET (ev_window));
        gtk_widget_realize (GTK_WIDGET (ev_window));
    }
    ev_window_open_uri (ev_window, uri, dest, mode, search_string);
    gtk_widget_show(GTK_WIDGET (ev_window));

#ifdef GDK_WINDOWING_X11
    GdkWindow *gdk_window = gtk_widget_get_window (GTK_WIDGET (ev_window));
    if (GDK_IS_X11_WINDOW (gdk_window)) {
        if (timestamp <= 0)
            timestamp = gdk_x11_get_server_time (gdk_window);

        gdk_x11_window_set_user_time (gdk_window, timestamp);
        gtk_window_present (GTK_WINDOW (ev_window));
    } else
#endif /* GDK_WINDOWING_X11 */
    {
        gtk_window_present_with_time (GTK_WINDOW (ev_window), timestamp);
    }
}

static void
_ev_application_open_uri_at_dest (EvApplication  *application,
                                  const gchar    *uri,
                                  GdkScreen      *screen,
                                  EvLinkDest     *dest,
                                  EvWindowRunMode mode,
                                  const gchar    *search_string,
                                  guint           timestamp)
{
    EvWindow  *empty_window;
    GtkWidget *new_window;

    empty_window = ev_application_get_empty_window (application, screen);
    if (empty_window)
        new_window = GTK_WIDGET (empty_window);
    else
        new_window = ev_application_create_window (application);

    ev_application_open_uri_in_window (application, uri, EV_WINDOW (new_window),
                                       screen, dest, mode,
                                       search_string,
                                       timestamp);
}

/**
 * ev_application_open_uri_at_dest:
 * @application: The instance of the application.
 * @uri: The uri to be opened.
 * @screen: Thee screen where the link will be shown.
 * @dest: The #EvLinkDest of the document.
 * @mode: The run mode of the window.
 * @timestamp: Current time value.
 */
void
ev_application_open_uri_at_dest (EvApplication  *application,
                                 const char     *uri,
                                 GdkScreen      *screen,
                                 EvLinkDest     *dest,
                                 EvWindowRunMode mode,
                                 const gchar    *search_string,
                                 guint           timestamp)
{
    g_return_if_fail (uri != NULL);

    GSettings *settings = g_settings_new ("org.x.reader");
    gboolean tabbed_mode = g_settings_get_boolean (settings, "tabbed-mode");
    g_object_unref (settings);

    if (!tabbed_mode && application->uri && strcmp (application->uri, uri) != 0) {
        /* spawn a new xreader process */
        ev_spawn (uri, screen, dest, mode, search_string, timestamp);
        return;
    } else if (!application->uri) {
        application->uri = g_strdup (uri);
    }

#ifdef ENABLE_DBUS
    /* Register the uri or send Reload to
     * remote instance if already registered
     */
    ev_application_register_uri (application, uri, screen, dest, mode, search_string, timestamp);
#else
    _ev_application_open_uri_at_dest (application, uri, screen, dest, mode, search_string, timestamp);
#endif /* ENABLE_DBUS */
}

/**
 * ev_application_open_window:
 * @application: The instance of the application.
 * @timestamp: Current time value.
 *
 * Creates a new window
 */
void
ev_application_open_window (EvApplication *application,
                            GdkScreen     *screen,
                            guint32        timestamp)
{
    GtkWidget *new_window = ev_application_create_window (application);

    if (screen) {
        ev_stock_icons_set_screen (screen);
        gtk_window_set_screen (GTK_WINDOW (new_window), screen);
    }

    if (!gtk_widget_get_realized (new_window))
        gtk_widget_realize (new_window);

#ifdef GDK_WINDOWING_X11
    GdkWindow *gdk_window = gtk_widget_get_window (GTK_WIDGET (new_window));
    if (GDK_IS_X11_WINDOW (gdk_window)) {
        if (timestamp <= 0)
            timestamp = gdk_x11_get_server_time (gdk_window);
        gdk_x11_window_set_user_time (gdk_window, timestamp);

        gtk_window_present (GTK_WINDOW (new_window));
    } else
#endif /* GDK_WINDOWING_X11 */
    {
        gtk_window_present_with_time (GTK_WINDOW (new_window), timestamp);
    }
}

#ifdef ENABLE_DBUS
static gboolean
handle_get_window_list_cb (EvXreaderApplication   *object,
                           GDBusMethodInvocation *invocation,
                           EvApplication         *application)
{
        GList     *windows, *l;
        GPtrArray *paths;

        paths = g_ptr_array_new ();

        windows = gtk_application_get_windows (GTK_APPLICATION (application));
        for (l = windows; l; l = g_list_next (l)) {
            if (!EV_IS_WINDOW (l->data))
                continue;

            g_ptr_array_add (paths, (gpointer) ev_window_get_dbus_object_path (EV_WINDOW (l->data)));
        }

        g_ptr_array_add (paths, NULL);
        ev_xreader_application_complete_get_window_list (object, invocation,
                                                        (const char * const *) paths->pdata);

        g_ptr_array_free (paths, TRUE);

        return TRUE;
}

static gboolean
handle_reload_cb (EvXreaderApplication   *object,
                  GDBusMethodInvocation *invocation,
                  GVariant              *args,
                  guint                  timestamp,
                  EvApplication         *application)
{
    GList           *windows, *l;
    GVariantIter     iter;
    const gchar     *key;
    GVariant        *value;
    GdkDisplay      *display = NULL;
    int              screen_number = 0;
    EvLinkDest      *dest = NULL;
    EvWindowRunMode  mode = EV_WINDOW_MODE_NORMAL;
    const gchar     *search_string = NULL;
    GdkScreen       *screen = NULL;

    g_variant_iter_init (&iter, args);

    while (g_variant_iter_loop (&iter, "{&sv}", &key, &value)) {
        if (strcmp (key, "display") == 0 && g_variant_classify (value) == G_VARIANT_CLASS_STRING) {
            display = ev_display_open_if_needed (g_variant_get_string (value, NULL));
        } else if (strcmp (key, "screen") == 0 && g_variant_classify (value) == G_VARIANT_CLASS_INT32) {
            screen_number = g_variant_get_int32 (value);
        } else if (strcmp (key, "mode") == 0 && g_variant_classify (value) == G_VARIANT_CLASS_UINT32) {
            mode = g_variant_get_uint32 (value);
        } else if (strcmp (key, "page-label") == 0 && g_variant_classify (value) == G_VARIANT_CLASS_STRING) {
            dest = ev_link_dest_new_page_label (g_variant_get_string (value, NULL));
        } else if (strcmp (key, "named-dest") == 0 && g_variant_classify (value) == G_VARIANT_CLASS_STRING) {
            dest = ev_link_dest_new_named (g_variant_get_string (value, NULL));
        } else if (strcmp (key, "page-index") == 0 && g_variant_classify (value) == G_VARIANT_CLASS_UINT32) {
            dest = ev_link_dest_new_page (g_variant_get_uint32 (value));
        } else if (strcmp (key, "find-string") == 0 && g_variant_classify (value) == G_VARIANT_CLASS_STRING) {
            search_string = g_variant_get_string (value, NULL);
        }
    }

    if (display != NULL &&
        screen_number >= 0 &&
        screen_number < gdk_display_get_n_screens (display))
        screen = gdk_display_get_screen (display, screen_number);
    else
        screen = gdk_screen_get_default ();

    windows = gtk_application_get_windows (GTK_APPLICATION ((application)));
    for (l = windows; l != NULL; l = g_list_next (l)) {
         if (!EV_IS_WINDOW (l->data))
             continue;

        ev_application_open_uri_in_window (application, NULL,
                                           EV_WINDOW (l->data),
                                           screen, dest, mode,
                                           search_string,
                                           timestamp);
    }

    if (dest)
        g_object_unref (dest);

    ev_xreader_application_complete_reload (object, invocation);

    return TRUE;
}
#endif /* ENABLE_DBUS */

void
ev_application_open_uri_list (EvApplication *application,
                              GSList        *uri_list,
                              GdkScreen     *screen,
                              guint          timestamp)
{
    GSList *l;

    for (l = uri_list; l != NULL; l = l->next) {
        ev_application_open_uri_at_dest (application, (char *)l->data,
                                         screen, NULL, 0, NULL,
                                         timestamp);
    }
}

static void ev_application_accel_map_save(EvApplication* application)
{
    gchar* accel_map_file;
    gchar* tmp_filename;
    gint fd;

    accel_map_file = g_build_filename (application->dot_dir, "accels", NULL);
    tmp_filename = g_strdup_printf("%s.XXXXXX", accel_map_file);

    fd = g_mkstemp(tmp_filename);

    if (fd == -1)
    {
        g_free(accel_map_file);
        g_free(tmp_filename);

        return;
    }

    gtk_accel_map_save_fd(fd);
    close(fd);

    g_mkdir_with_parents (application->dot_dir, 0700);
    if (g_rename(tmp_filename, accel_map_file) == -1)
    {
        g_unlink(tmp_filename);
    }

    g_free(accel_map_file);
    g_free(tmp_filename);
}

static void ev_application_accel_map_load(EvApplication* application)
{
    gchar* accel_map_file;

    accel_map_file = g_build_filename (application->dot_dir, "accels", NULL);
    gtk_accel_map_load(accel_map_file);
    g_free(accel_map_file);
}

static void
ev_application_shutdown (GApplication *gapplication)
{
    EvApplication *application = EV_APPLICATION (gapplication);

    if (application->uri) {
#ifdef ENABLE_DBUS
        ev_application_unregister_uri (application,
                                       application->uri);
#endif
        g_free (application->uri);
        application->uri = NULL;
    }

    ev_application_accel_map_save (application);

    g_clear_object (&application->scr_saver);

    g_free (application->dot_dir);
    application->dot_dir = NULL;

    if (application->dark_mode_manager) {
        g_clear_object (&application->dark_mode_manager);
    }

    g_clear_pointer (&supported_mimetypes, g_strfreev);

    G_APPLICATION_CLASS (ev_application_parent_class)->shutdown (gapplication);
}

static void
ev_application_activate (GApplication *gapplication)
{
        EvApplication *application = EV_APPLICATION (gapplication);
        GList *windows, *l;

        windows = gtk_application_get_windows (GTK_APPLICATION (application));
        for (l = windows; l != NULL; l = l->next) {
                if (!EV_IS_WINDOW (l->data))
                        continue;

                gtk_window_present (GTK_WINDOW (l->data));
    }
    }

#ifdef ENABLE_DBUS
static gboolean
ev_application_dbus_register (GApplication    *gapplication,
                              GDBusConnection *connection,
                              const gchar     *object_path,
                              GError         **error)
{
        EvApplication *application = EV_APPLICATION (gapplication);
        EvXreaderApplication *skeleton;

        if (!G_APPLICATION_CLASS (ev_application_parent_class)->dbus_register (gapplication,
                                                                               connection,
                                                                               object_path,
                                                                               error))
                return FALSE;

        skeleton = ev_xreader_application_skeleton_new ();
        if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (skeleton),
                                               connection,
                                              APPLICATION_DBUS_OBJECT_PATH,
                                               error)) {
                g_object_unref (skeleton);

                return FALSE;
        }

        application->skeleton = skeleton;
                g_signal_connect (skeleton, "handle-get-window-list",
                                  G_CALLBACK (handle_get_window_list_cb),
                                  application);
                g_signal_connect (skeleton, "handle-reload",
                                  G_CALLBACK (handle_reload_cb),
                                  application);

        return TRUE;
        }

static void
ev_application_dbus_unregister (GApplication    *gapplication,
                                GDBusConnection *connection,
                                const gchar     *object_path)
{
        EvApplication *application = EV_APPLICATION (gapplication);

        if (application->skeleton != NULL) {
                g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (application->skeleton));
                g_object_unref (application->skeleton);
                application->skeleton = NULL;
        }

        G_APPLICATION_CLASS (ev_application_parent_class)->dbus_unregister (gapplication,
                                                                            connection,
                                                                            object_path);
    }

#endif /* ENABLE_DBUS */
static void
open_location_action_activate (GSimpleAction *action,
                                GVariant      *parameter,
                                gpointer       user_data)
{
	EvApplication *application = EV_APPLICATION (user_data);
	const gchar *uri = NULL;

	if (parameter != NULL) {
		g_variant_get (parameter, "&s", &uri);
	}

	ev_application_open_uri_at_dest (application, uri, NULL, NULL,
	                                   EV_WINDOW_MODE_NORMAL, NULL, 0);
}

static void
quit_action_activate (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
	EvApplication *application = EV_APPLICATION (user_data);

	g_application_quit (G_APPLICATION (application));
}

static void
help_action_activate (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
	GError *error = NULL;
	gtk_show_uri (NULL, "help:xreader", GDK_CURRENT_TIME, &error);
	if (error) {
		g_warning ("Error showing help: %s", error->message);
		g_error_free (error);
	}
}

static void
about_action_activate (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       user_data)
{
	const gchar *authors[] = {
		"Mavis",
		NULL
	};

	gtk_show_about_dialog (NULL,
			      "program-name", "Xreader",
			      "version", VERSION,
			      "copyright", "Copyright (C) 2002-2026 The Xreader Authors",
			      "license-type", GTK_LICENSE_GPL_2_0,
			      "authors", authors,
			      "website", "https://github.com/yuri-schmaltz/xreader",
			      NULL);
}

static void
print_action_activate (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       user_data)
{
	/* TODO: dispatch to ev_window_print_range (PR #91 will
	 * wire this up).  For now the action is registered
	 * but inert. */
}

static void
save_action_activate (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
	/* TODO: dispatch to ev_window_save_as (PR #91). */
}

static void
find_action_activate (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
	/* TODO: dispatch to ev_window_show_find_bar (PR #91). */
}

static const GActionEntry app_actions[] = {
	{ "open-location", open_location_action_activate, "s" },
	{ "quit",         quit_action_activate,           NULL },
	{ "help",         help_action_activate,           NULL },
	{ "about",        about_action_activate,          NULL },
	{ "print",        print_action_activate,           NULL },
	{ "save",         save_action_activate,            NULL },
	{ "find",         find_action_activate,            NULL },
};

static void
ev_application_class_init (EvApplicationClass *ev_application_class)
{
        GApplicationClass *g_application_class = G_APPLICATION_CLASS (ev_application_class);

        g_application_class->activate = ev_application_activate;
        g_application_class->shutdown = ev_application_shutdown;

#ifdef ENABLE_DBUS
        g_application_class->dbus_register = ev_application_dbus_register;
        g_application_class->dbus_unregister = ev_application_dbus_unregister;
#endif
        }

static void
parse_mimetypes (void)
{
    supported_mimetypes = g_strsplit (SUPPORTED_MIMETYPES, ";", -1);
}

static void
ev_application_init (EvApplication *ev_application)
{
    ev_application->dot_dir = g_build_filename (g_get_user_config_dir (), "xreader", NULL);

    ev_application_init_session (ev_application);

	ev_application_accel_map_load (ev_application);

	/* Register the GAction namespace.  These actions are
	 * parallel to the old GtkAction paths (which are still
	 * active) and provide a forward-compatible API for new
	 * code.  The eventual ev-window.c GAction migration
	 * (B1) will switch the menu definitions one-by-one to
	 * use these namespaced actions. */
	g_action_map_add_action_entries (G_ACTION_MAP (ev_application),
	                                 app_actions,
	                                 G_N_ELEMENTS (app_actions),
	                                 ev_application);

    parse_mimetypes ();

    ev_application->scr_saver = totem_scrsaver_new ();
    g_object_set (ev_application->scr_saver,
                  "reason", _("Running in presentation mode"),
                  NULL);

    if (g_strcmp0 (g_getenv ("XDG_CURRENT_DESKTOP"), "XFCE") != 0) {
        ev_application->dark_mode_manager = xapp_dark_mode_manager_new (FALSE);
    }
    else {
        ev_application->dark_mode_manager = NULL;
    }
}

gboolean
ev_application_has_window (EvApplication *application)
{
    GList *l, *windows;

    windows = gtk_application_get_windows (GTK_APPLICATION (application));
    for (l = windows; l != NULL; l = l->next) {
        if (!EV_IS_WINDOW (l->data))
            continue;

        return TRUE;
    }

    return FALSE;
}

guint
ev_application_get_n_windows (EvApplication *application)
{
    GList *l, *windows;
    guint  retval = 0;

        windows = gtk_application_get_windows (GTK_APPLICATION (application));
        for (l = windows; l != NULL && !retval; l = l->next) {
                if (!EV_IS_WINDOW (l->data))
                        continue;

            retval++;
    }

    return retval;
}

const gchar *
ev_application_get_uri (EvApplication *application)
{
    return application->uri;
}

void
ev_application_screensaver_enable (EvApplication *application)
{
    totem_scrsaver_enable (application->scr_saver);
}

void
ev_application_screensaver_disable (EvApplication *application)
{
    totem_scrsaver_disable (application->scr_saver);
}

const gchar *
ev_application_get_dot_dir (EvApplication *application,
                            gboolean create)
{
    if (create)
        g_mkdir_with_parents (application->dot_dir, 0700);

    return application->dot_dir;
}


/**
 * ev_application_create_window:
 * @application: an #EvApplication
 *
 * Creates a new top-level window.  When the 'tabbed-mode'
 * GSettings key is true, the new window is an #EvTabbedWindow
 * (which hosts multiple documents in tabs); otherwise it
 * is a legacy #EvWindow (single document per window).
 *
 * This is the single point where the tabbed-mode choice
 * is reflected.  All call sites that previously did
 * `ev_window_new()` should be updated to call this
 * helper instead.
 *
 * Returns: (transfer full): a new top-level window
 */
GtkWidget *
ev_application_create_window (EvApplication *application)
{
	GSettings *settings;
	gboolean   tabbed;

	g_return_val_if_fail (EV_IS_APPLICATION (application), NULL);

	settings = g_settings_new ("org.x.reader");
	tabbed = g_settings_get_boolean (settings, "tabbed-mode");
	g_object_unref (settings);

	if (tabbed) {
		return ev_tabbed_window_new (GTK_APPLICATION (application));
	}

	return ev_window_new ();
}
