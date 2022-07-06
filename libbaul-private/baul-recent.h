

#ifndef __BAUL_RECENT_H__
#define __BAUL_RECENT_H__

#include <ctk/ctk.h>
#include <gio/gio.h>

#include "baul-file.h"

void baul_recent_add_file (BaulFile *file,
                           GAppInfo *application);

#endif
