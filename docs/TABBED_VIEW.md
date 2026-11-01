# Tabbed view (C3 feature)

Xreader 4.8.0 introduces a tabbed document view (the C3 item in
ROADMAP.md).  Multiple documents can now be opened in a single
window, with each document in its own tab.

## Quick start

To enable tabbed mode:

  - **Command line**: pass `--tabbed` (or `-T`) when launching
    xreader: `xreader --tabbed file1.pdf file2.pdf`
  - **Persistent**: the choice is stored in GSettings under
    `org.x.reader.tabbed-mode`.  Set it with:
    `gsettings set org.x.reader tabbed-mode true`

When tabbed mode is on, opening a file adds a new tab to the
existing window.  When off (the default), opening a file opens
a new window (the 4.7.0 behavior).

## Keyboard shortcuts

| Shortcut           | Action                  |
|--------------------|-------------------------|
| `Ctrl+Tab`         | Switch to next tab      |
| `Ctrl+Shift+Tab`   | Switch to previous tab  |
| `Ctrl+PgDn`        | Switch to next tab      |
| `Ctrl+PgUp`        | Switch to previous tab  |
| `Ctrl+W`           | Close active tab        |
| `Ctrl+Shift+T`     | Reopen last closed tab  |

The reopen-last-closed-tab action has a bounded history of 10
tabs.  Pressing it more than 10 times after the same 10 closes
restores the tabs in reverse order (LIFO).

## Mouse interaction

  - **Click on tab label**: switch to that tab
  - **Click on tab close button (X)**: close that tab
  - **Middle-click on tab label**: close that tab (alternative
    to the X button)
  - **Drag a file onto a tab**: open the file in a new tab
  - **Drag a file onto the empty state**: open the file in a
    new tab

## Tab bar visibility

The tab bar is automatically shown when the window has 2 or
more tabs, and hidden when the window has 0 or 1 tab.  This
keeps single-document mode visually identical to the 4.7.0
behavior (no tab bar clutter).

## Architecture

The tabbed view is implemented in 4 classes:

  - `EvTab` (`shell/ev-tab.h`): per-tab state (location, page,
    modified, title, tooltip).  A simple GtkBox that holds
    the document view.
  - `EvTabManager` (`shell/ev-tab-manager.h`): owns the list
    of tabs + tracks the active one.  Emits 4 signals
    (tab-added, tab-removed, active-changed, tab-reordered).
  - `EvTabbedWindow` (`shell/ev-tabbed-window.h`): the top-level
    window.  Composes an EvTabManager + GtkNotebook.  Hosts the
    shared chrome (menubar, toolbar, thumbnail sidebar, status
    bar) and implements the keyboard shortcuts and drag-and-drop.
    Menu actions (zoom, rotate, page navigation, copy, fullscreen)
    operate on the active tab's view/model.
  - The `tabbed-mode` GSettings key (`org.x.reader`): persistent
    on/off state.

## What is NOT in 4.8.0 (planned for 4.9.0+)

The 4.8.0 release includes the foundation + the user-visible
surface (notebook, shortcuts, drag-and-drop, reopen stack).  The
window now also has shared chrome wired to the active tab: a full
menubar (File/Edit/View/Go/Tabs/Help), a thumbnail sidebar, and a
status bar showing the current page and zoom.  The following
features are still planned for 4.9.0:

  - **Find bar**: an in-window find bar (with next/prev/highlight)
    bound to the active tab's view.
  - **Session save/restore**: the list of open tabs +
    active tab index should be persisted across sessions.
  - **Multi-file drop**: drag multiple files from a file
    manager to open them all in tabs at once (4.8.0 opens
    only the first file in a multi-file drop).

These are all tracked in ROADMAP.md under C3.

## Testing

The tabbed view has 20+ unit tests:

  - `test-tab-manager.c` (14 cases): covers append, remove,
    active-changed, reorder, get-tab-by-location, etc.
  - `test-reopen-stack.c` (6 cases): covers the reopen-last-
    closed-tab stack.

Run with: `meson test -C builddir test-tab-manager
test-reopen-stack`.

## API documentation

See the gtk-doc blocks in:

  - `shell/ev-tab.h` (per-tab state)
  - `shell/ev-tab-manager.h` (list + active tracking)
  - `shell/ev-tabbed-window.h` (top-level window + actions)
