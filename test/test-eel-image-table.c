#include <stdlib.h>
#include <ctk/ctk.h>

#include <eel/eel-image-table.h>

#include "test.h"

static const char pixbuf_name[] = "/usr/share/pixmaps/cafe-about-logo.png";

#define BG_COLOR 0xFFFFFF
#define BG_COLOR_SPEC "white"

static const char *names[] =
{
	"Tomaso Albinoni",
	"Isaac Alb�niz",
	"Georges Bizet",
	"Luigi Boccherini",
	"Alexander Borodin",
	"Johannes Brahms",
	"Max Bruch",
	"Anton Bruckner",
	"Fr�d�ric Chopin",
	"Aaron Copland",
	"John Corigliano",
	"Claude Debussy",
	"L�o Delibes",
	"Anton�n Dvor�k",
	"Edward Elgar",
	"Manuel de Falla",
	"George Gershwin",
	"Alexander Glazunov",
	"Mikhail Glinka",
	"Enrique Granados",
	"Edvard Grieg",
	"Joseph Haydn",
	"Scott Joplin",
	"Franz Liszt",
	"Gustav Mahler",
	"Igor Markevitch",
	"Felix Mendelssohn",
	"Modest Mussorgsky",
	"Sergei Prokofiev",
	"Giacomo Puccini",
	"Maurice Ravel",
	"Ottorino Respighi",
	"Joaquin Rodrigo",
	"Gioachino Rossini",
	"Domenico Scarlatti",
	"Franz Schubert",
	"Robert Schumann",
	"Jean Sibelius",
	"Bedrich Smetana",
	"Johann Strauss",
	"Igor Stravinsky",
	"Giuseppe Verdi",
	"Antonio Vivaldi",
	"Richard Wagner",
};

static CtkWidget *
labeled_image_new (const char *text,
		   const char *icon_name)
{
	CtkWidget *image;
	GdkPixbuf *pixbuf = NULL;

	if (icon_name) {
		float sizes[] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
					1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0 };
		pixbuf = test_pixbuf_new_named (icon_name, sizes[random () % G_N_ELEMENTS (sizes)]);
	}

	image = eel_labeled_image_new (text, pixbuf);

	eel_gdk_pixbuf_unref_if_not_null (pixbuf);

	return image;
}


static void
image_table_child_enter_callback (CtkWidget *image_table G_GNUC_UNUSED,
				  CtkWidget *item G_GNUC_UNUSED,
				  gpointer   callback_data G_GNUC_UNUSED)
{
#if 0
	char *text;

	g_return_if_fail (EEL_IS_IMAGE_TABLE (image_table));
	g_return_if_fail (EEL_IS_LABELED_IMAGE (item));

	text = eel_labeled_image_get_text (EEL_LABELED_IMAGE (item));

	g_print ("%s(%s)\n", G_STRFUNC, text);
#endif
}

static void
image_table_child_leave_callback (CtkWidget *image_table G_GNUC_UNUSED,
				  CtkWidget *item G_GNUC_UNUSED,
				  gpointer   callback_data G_GNUC_UNUSED)
{
#if 0
	char *text;

	g_return_if_fail (EEL_IS_IMAGE_TABLE (image_table));
	g_return_if_fail (EEL_IS_LABELED_IMAGE (item));

	text = eel_labeled_image_get_text (EEL_LABELED_IMAGE (item));

	g_print ("%s(%s)\n", G_STRFUNC, text);
#endif
}

static void
image_table_child_pressed_callback (CtkWidget *image_table,
				    CtkWidget *item,
				    gpointer   callback_data G_GNUC_UNUSED)
{
	char *text;

	g_return_if_fail (EEL_IS_IMAGE_TABLE (image_table));
	g_return_if_fail (EEL_IS_LABELED_IMAGE (item));

	text = eel_labeled_image_get_text (EEL_LABELED_IMAGE (item));

	g_print ("%s(%s)\n", G_STRFUNC, text);
}

static void
image_table_child_released_callback (CtkWidget *image_table,
				     CtkWidget *item,
				     gpointer   callback_data G_GNUC_UNUSED)
{
	char *text;

	g_return_if_fail (EEL_IS_IMAGE_TABLE (image_table));
	g_return_if_fail (EEL_IS_LABELED_IMAGE (item));

	text = eel_labeled_image_get_text (EEL_LABELED_IMAGE (item));

	g_print ("%s(%s)\n", G_STRFUNC, text);
}

static void
image_table_child_clicked_callback (CtkWidget *image_table,
				    CtkWidget *item,
				    gpointer   callback_data G_GNUC_UNUSED)
{
	char *text;

	g_return_if_fail (EEL_IS_IMAGE_TABLE (image_table));
	g_return_if_fail (EEL_IS_LABELED_IMAGE (item));

	text = eel_labeled_image_get_text (EEL_LABELED_IMAGE (item));

	g_print ("%s(%s)\n", G_STRFUNC, text);
}

static int
foo_timeout (gpointer callback_data)
{
	static int recursion_count = 0;
	g_return_val_if_fail (CTK_IS_WINDOW (callback_data), FALSE);

	recursion_count++;

	g_print ("%s(%d)\n", G_STRFUNC, recursion_count);
	ctk_widget_queue_resize (CTK_WIDGET (callback_data));

	recursion_count--;

	return FALSE;
}

static void
image_table_size_allocate (CtkWidget *image_table,
			   CtkAllocation *allocation,
			   gpointer callback_data)
{
	static int recursion_count = 0;
	CtkAllocation w_allocation;

	g_return_if_fail (EEL_IS_IMAGE_TABLE (image_table));
	g_return_if_fail (allocation != NULL);
	g_return_if_fail (CTK_IS_WINDOW (callback_data));

	recursion_count++;

	if (0) g_timeout_add (0, foo_timeout, callback_data);

	/*ctk_widget_queue_resize (CTK_WIDGET (callback_data));*/

	ctk_widget_get_allocation (CTK_WIDGET (image_table), &w_allocation);
	if (0) ctk_widget_size_allocate (CTK_WIDGET (image_table),
					 &w_allocation);

	g_print ("%s(%d)\n", G_STRFUNC, recursion_count);

	recursion_count--;
}

static CtkWidget *
image_table_new_scrolled (void)
{
	CtkWidget *scrolled;
	CtkWidget *viewport;
	CtkWidget *window;
	CtkWidget *image_table;
	int i;

	window = test_window_new ("Image Table Test", 10);

	ctk_window_set_default_size (CTK_WINDOW (window), 400, 300);

	/* Scrolled window */
	scrolled = ctk_scrolled_window_new (NULL, NULL);
	ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled),
					CTK_POLICY_NEVER,
					CTK_POLICY_AUTOMATIC);
	ctk_container_add (CTK_CONTAINER (window), scrolled);

	/* Viewport */
 	viewport = ctk_viewport_new (NULL, NULL);
	ctk_viewport_set_shadow_type (CTK_VIEWPORT (viewport), CTK_SHADOW_OUT);
	ctk_container_add (CTK_CONTAINER (scrolled), viewport);

	image_table = eel_image_table_new (FALSE);

	if (0) g_signal_connect (image_table,
			    "size_allocate",
			    G_CALLBACK (image_table_size_allocate),
			    window);

	eel_wrap_table_set_x_justification (EEL_WRAP_TABLE (image_table),
						 EEL_JUSTIFICATION_MIDDLE);
	eel_wrap_table_set_y_justification (EEL_WRAP_TABLE (image_table),
						 EEL_JUSTIFICATION_END);

	ctk_container_add (CTK_CONTAINER (viewport), image_table);

	g_signal_connect (image_table,
			    "child_enter",
			    G_CALLBACK (image_table_child_enter_callback),
			    NULL);

	g_signal_connect (image_table,
			    "child_leave",
			    G_CALLBACK (image_table_child_leave_callback),
			    NULL);

	g_signal_connect (image_table,
			    "child_pressed",
			    G_CALLBACK (image_table_child_pressed_callback),
			    NULL);

	g_signal_connect (image_table,
			    "child_released",
			    G_CALLBACK (image_table_child_released_callback),
			    NULL);

	g_signal_connect (image_table,
			    "child_clicked",
			    G_CALLBACK (image_table_child_clicked_callback),
			    NULL);

	for (i = 0; i < 100; i++) {
		char *text;
		CtkWidget *image;

		text = g_strdup_printf ("%s %d",
					names[random () % G_N_ELEMENTS (names)],
					i);
		image = labeled_image_new (text, pixbuf_name);
		g_free (text);

		ctk_container_add (CTK_CONTAINER (image_table), image);
		ctk_widget_show (image);
	}

	ctk_widget_show (viewport);
	ctk_widget_show (scrolled);
	ctk_widget_show (image_table);

	return window;
}

int
main (int argc, char* argv[])
{
	CtkWidget *window = NULL;

	test_init (&argc, &argv);

	window = image_table_new_scrolled ();

	ctk_widget_show (window);

	ctk_main ();

	return 0;
}
