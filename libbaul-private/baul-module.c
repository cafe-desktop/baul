/*
 *  baul-module.h - Interface to baul extensions
 *
 *  Copyright (C) 2003 Novell, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Author: Dave Camp <dave@ximian.com>
 *
 */

#include <config.h>
#include <gmodule.h>

#include <eel/eel-ctk-macros.h>
#include <eel/eel-debug.h>

#include "baul-module.h"
#include "baul-extensions.h"

#define BAUL_TYPE_MODULE    	(baul_module_get_type ())
#define BAUL_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_MODULE, BaulModule))
#define BAUL_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_MODULE, BaulModule))
#define BAUL_IS_MODULE(obj)		(G_TYPE_INSTANCE_CHECK_TYPE ((obj), BAUL_TYPE_MODULE))
#define BAUL_IS_MODULE_CLASS(klass)	(G_TYPE_CLASS_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_MODULE))

typedef struct _BaulModule        BaulModule;
typedef struct _BaulModuleClass   BaulModuleClass;

struct _BaulModule
{
    GTypeModule parent;

    GModule *library;

    char *path;

    void (*initialize) (GTypeModule  *module);
    void (*shutdown)   (void);

    void (*list_types) (const GType **types,
                        int          *num_types);
    void (*list_pyfiles) (GList     **pyfiles);

};

struct _BaulModuleClass
{
    GTypeModuleClass parent;
};

static GList *module_objects = NULL;

static GType baul_module_get_type (void);

G_DEFINE_TYPE (BaulModule, baul_module, G_TYPE_TYPE_MODULE);
#define parent_class baul_module_parent_class

static gboolean
baul_module_load (GTypeModule *gmodule)
{
    BaulModule *module;

    module = BAUL_MODULE (gmodule);

    module->library = g_module_open (module->path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);

    if (!module->library)
    {
        g_warning ("%s", g_module_error ());
        return FALSE;
    }

    if (!g_module_symbol (module->library,
                          "baul_module_initialize",
                          (gpointer *)&module->initialize) ||
            !g_module_symbol (module->library,
                              "baul_module_shutdown",
                              (gpointer *)&module->shutdown) ||
            !g_module_symbol (module->library,
                              "baul_module_list_types",
                              (gpointer *)&module->list_types))
    {

        g_warning ("%s", g_module_error ());
        g_module_close (module->library);

        return FALSE;
    }

    g_module_symbol (module->library,
                     "baul_module_list_pyfiles",
                     (gpointer *)&module->list_pyfiles);

    module->initialize (gmodule);

    return TRUE;
}

static void
baul_module_unload (GTypeModule *gmodule)
{
    BaulModule *module;

    module = BAUL_MODULE (gmodule);

    module->shutdown ();

    g_module_close (module->library);

    module->initialize = NULL;
    module->shutdown = NULL;
    module->list_types = NULL;
}

static void
baul_module_finalize (GObject *object)
{
    BaulModule *module;

    module = BAUL_MODULE (object);

    g_free (module->path);

    EEL_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
baul_module_init (BaulModule *module G_GNUC_UNUSED)
{
}

static void
baul_module_class_init (BaulModuleClass *class)
{
    G_OBJECT_CLASS (class)->finalize = baul_module_finalize;
    G_TYPE_MODULE_CLASS (class)->load = baul_module_load;
    G_TYPE_MODULE_CLASS (class)->unload = baul_module_unload;
}

static void
module_object_weak_notify (gpointer user_data G_GNUC_UNUSED,
			   GObject *object)
{
    module_objects = g_list_remove (module_objects, object);
}

static void
add_module_objects (BaulModule *module)
{
    GObject *object = NULL;
    GList *pyfiles = NULL;
    gchar *filename = NULL;
    const GType *types = NULL;
    int num_types = 0;
    int i;

    module->list_types (&types, &num_types);
    filename = g_path_get_basename (module->path);

    /* fetch extensions details loaded through python-baul module */
    if (module->list_pyfiles)
    {
        module->list_pyfiles(&pyfiles);
    }

    for (i = 0; i < num_types; i++)
    {
        if (types[i] == 0)   /* Work around broken extensions */
        {
            break;
        }

        if (module->list_pyfiles)
        {
            filename = g_strconcat(g_list_nth_data(pyfiles, i), ".py", NULL);
        }

        object = baul_module_add_type (types[i]);
        baul_extension_register (filename, object);
    }
}

static BaulModule *
baul_module_load_file (const char *filename)
{
    BaulModule *module;

    module = g_object_new (BAUL_TYPE_MODULE, NULL);
    module->path = g_strdup (filename);

    if (g_type_module_use (G_TYPE_MODULE (module)))
    {
        add_module_objects (module);
        g_type_module_unuse (G_TYPE_MODULE (module));
        return module;
    }
    else
    {
        g_object_unref (module);
        return NULL;
    }
}

static void
load_module_dir (const char *dirname)
{
    GDir *dir;

    dir = g_dir_open (dirname, 0, NULL);

    if (dir)
    {
        const char *name;

        while ((name = g_dir_read_name (dir)))
        {
            if (g_str_has_suffix (name, "." G_MODULE_SUFFIX))
            {
                char *filename;

                filename = g_build_filename (dirname,
                                             name,
                                             NULL);
                baul_module_load_file (filename);
            }
        }
        g_dir_close (dir);
    }
}

static void
free_module_objects (void)
{
    GList *l, *next;

    for (l = module_objects; l != NULL; l = next)
    {
        next = l->next;
        g_object_unref (l->data);
    }

    g_list_free (module_objects);
}

void
baul_module_setup (void)
{
    static gboolean initialized = FALSE;

    if (!initialized)
    {
        initialized = TRUE;

        load_module_dir (BAUL_EXTENSIONDIR);

        eel_debug_call_at_shutdown (free_module_objects);
    }
}

GList *
baul_module_get_extensions_for_type (GType type)
{
    GList *l;
    GList *ret = NULL;

    for (l = module_objects; l != NULL; l = l->next)
    {
        if (G_TYPE_CHECK_INSTANCE_TYPE (G_OBJECT (l->data),
                                        type))
        {
            g_object_ref (l->data);
            ret = g_list_prepend (ret, l->data);
        }
    }

    return ret;
}

void
baul_module_extension_list_free (GList *extensions)
{
    GList *l, *next;

    for (l = extensions; l != NULL; l = next)
    {
        next = l->next;
        g_object_unref (l->data);
    }
    g_list_free (extensions);
}

GObject *
baul_module_add_type (GType type)
{
    GObject *object;

    object = g_object_new (type, NULL);
    g_object_weak_ref (object,
                       (GWeakNotify)module_object_weak_notify,
                       NULL);

    module_objects = g_list_prepend (module_objects, object);
    return object;
}
