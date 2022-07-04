/*
 *  Copyright © 2002 Christophe Fergeau
 *  Copyright © 2003 Marco Pesenti Gritti
 *  Copyright © 2003, 2004 Christian Persch
 *    (ephy-notebook.c)
 *
 *  Copyright © 2008 Free Software Foundation, Inc.
 *    (baul-notebook.c)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  $Id: baul-notebook.h 8210 2008-04-11 20:05:25Z chpe $
 */

#ifndef BAUL_NOTEBOOK_H
#define BAUL_NOTEBOOK_H

#include <glib.h>

#include <gtk/gtk.h>
#include "baul-window-slot.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BAUL_TYPE_NOTEBOOK		(baul_notebook_get_type ())
#define BAUL_NOTEBOOK(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), BAUL_TYPE_NOTEBOOK, CajaNotebook))
#define BAUL_NOTEBOOK_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), BAUL_TYPE_NOTEBOOK, CajaNotebookClass))
#define BAUL_IS_NOTEBOOK(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), BAUL_TYPE_NOTEBOOK))
#define BAUL_IS_NOTEBOOK_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), BAUL_TYPE_NOTEBOOK))
#define BAUL_NOTEBOOK_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), BAUL_TYPE_NOTEBOOK, CajaNotebookClass))

    typedef struct _CajaNotebookClass	CajaNotebookClass;
    typedef struct _CajaNotebook		CajaNotebook;
    typedef struct _CajaNotebookPrivate	CajaNotebookPrivate;

    struct _CajaNotebook
    {
        GtkNotebook parent;

        /*< private >*/
        CajaNotebookPrivate *priv;
    };

    struct _CajaNotebookClass
    {
        GtkNotebookClass parent_class;

        /* Signals */
        void	 (* tab_close_request)  (CajaNotebook *notebook,
                                         CajaWindowSlot *slot);
    };

    GType		baul_notebook_get_type		(void);

    int		baul_notebook_add_tab	(CajaNotebook *nb,
                                     CajaWindowSlot *slot,
                                     int position,
                                     gboolean jump_to);

    void		baul_notebook_set_show_tabs	(CajaNotebook *nb,
            gboolean show_tabs);

    void		baul_notebook_set_dnd_enabled (CajaNotebook *nb,
            gboolean enabled);
    void		baul_notebook_sync_tab_label (CajaNotebook *nb,
            CajaWindowSlot *slot);
    void		baul_notebook_sync_loading   (CajaNotebook *nb,
            CajaWindowSlot *slot);

    void		baul_notebook_reorder_current_child_relative (CajaNotebook *notebook,
            int offset);
    void		baul_notebook_set_current_page_relative (CajaNotebook *notebook,
            int offset);

    gboolean        baul_notebook_can_reorder_current_child_relative (CajaNotebook *notebook,
            int offset);
    gboolean        baul_notebook_can_set_current_page_relative (CajaNotebook *notebook,
            int offset);

#ifdef __cplusplus
}
#endif

#endif /* BAUL_NOTEBOOK_H */

