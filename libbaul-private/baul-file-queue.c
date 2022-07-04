/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   Copyright (C) 2001 Maciej Stachowiak

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Author: Maciej Stachowiak <mjs@noisehavoc.org>
*/

#include <config.h>
#include "baul-file-queue.h"

#include <glib.h>

struct BaulFileQueue
{
    GList *head;
    GList *tail;
    GHashTable *item_to_link_map;
};

BaulFileQueue *
baul_file_queue_new (void)
{
    BaulFileQueue *queue;

    queue = g_new0 (BaulFileQueue, 1);
    queue->item_to_link_map = g_hash_table_new (g_direct_hash, g_direct_equal);

    return queue;
}

void
baul_file_queue_destroy (BaulFileQueue *queue)
{
    g_hash_table_destroy (queue->item_to_link_map);
    baul_file_list_free (queue->head);
    g_free (queue);
}

void
baul_file_queue_enqueue (BaulFileQueue *queue,
                         BaulFile      *file)
{
    if (g_hash_table_lookup (queue->item_to_link_map, file) != NULL)
    {
        /* It's already on the queue. */
        return;
    }

    if (queue->tail == NULL)
    {
        queue->head = g_list_append (NULL, file);
        queue->tail = queue->head;
    }
    else
    {
        queue->tail = g_list_append (queue->tail, file);
        queue->tail = queue->tail->next;
    }

    baul_file_ref (file);
    g_hash_table_insert (queue->item_to_link_map, file, queue->tail);
}

BaulFile *
baul_file_queue_dequeue (BaulFileQueue *queue)
{
    BaulFile *file;

    file = baul_file_queue_head (queue);
    baul_file_queue_remove (queue, file);

    return file;
}


void
baul_file_queue_remove (BaulFileQueue *queue,
                        BaulFile *file)
{
    GList *link;

    link = g_hash_table_lookup (queue->item_to_link_map, file);

    if (link == NULL)
    {
        /* It's not on the queue */
        return;
    }

    if (link == queue->tail)
    {
        /* Need to special-case removing the tail. */
        queue->tail = queue->tail->prev;
    }

    queue->head =  g_list_remove_link (queue->head, link);
    g_list_free (link);
    g_hash_table_remove (queue->item_to_link_map, file);

    baul_file_unref (file);
}

BaulFile *
baul_file_queue_head (BaulFileQueue *queue)
{
    if (queue->head == NULL)
    {
        return NULL;
    }

    return BAUL_FILE (queue->head->data);
}

gboolean
baul_file_queue_is_empty (BaulFileQueue *queue)
{
    return (queue->head == NULL);
}
