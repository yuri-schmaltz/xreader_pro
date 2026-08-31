#ifndef __EV_PREFERENCES_DIALOG_H__
#define __EV_PREFERENCES_DIALOG_H__

#include <libxapp/xapp-preferences-window.h>

#include "ev-window.h"

G_BEGIN_DECLS

void
ev_preferences_dialog_show (EvWindow *parent);

/* Internal: create a new preferences dialog.  Forward-declared
 * here to satisfy -Wmissing-prototypes; the implementation is
 * in ev-preferences-dialog.c and is not part of the public API. */
GtkWidget *
ev_preferences_dialog_new (EvWindow *parent);

G_END_DECLS

#endif /* __EV_PREFERENCES_DIALOG_H__ */

