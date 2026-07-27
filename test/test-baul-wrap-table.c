#include <eel/eel-wrap-table.h>
#include <eel/eel-labeled-image.h>
#include <eel/eel-vfs-extensions.h>
#include <libbaul-private/baul-customization-data.h>
#include <libbaul-private/baul-icon-info.h>

#include "test.h"

int
main (int argc, char* argv[])
{
	BaulCustomizationData *customization_data;
	CtkWidget *window;
	CtkWidget *emblems_table, *button, *scroller;
	char *emblem_name, *stripped_name;
	CdkPixbuf *pixbuf;
	char *label;

	test_init (&argc, &argv);

	window = test_window_new ("Wrap Table Test", 10);

	ctk_window_set_default_size (CTK_WINDOW (window), 400, 300);

	/* The emblems wrapped table */
	emblems_table = eel_wrap_table_new (TRUE);

	ctk_widget_show (emblems_table);
	ctk_container_set_border_width (CTK_CONTAINER (emblems_table), 8);

	scroller = ctk_scrolled_window_new (NULL, NULL);
	ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scroller),
					CTK_POLICY_NEVER,
					CTK_POLICY_AUTOMATIC);

	/* Viewport */
	ctk_container_add (CTK_CONTAINER (scroller),
					   emblems_table);

	ctk_container_add (CTK_CONTAINER (window), scroller);

	ctk_widget_show (scroller);

#if 0
	/* Get rid of default lowered shadow appearance.
	 * This must be done after the widget is realized, due to
	 * an apparent bug in ctk_viewport_set_shadow_type.
	 */
 	g_signal_connect (CTK_BIN (scroller->child),
			  "realize",
			  remove_default_viewport_shadow,
			  NULL);
#endif


	/* Use baul_customization to make the emblem widgets */
	customization_data = baul_customization_data_new ("emblems", TRUE,
							      BAUL_ICON_SIZE_SMALL,
							      BAUL_ICON_SIZE_SMALL);

	while (baul_customization_data_get_next_element_for_display (customization_data,
									 &emblem_name,
									 &pixbuf,
									 &label)) {

		stripped_name = eel_filename_strip_extension (emblem_name);
		g_free (emblem_name);

		if (strcmp (stripped_name, "erase") == 0) {
			g_object_unref (pixbuf);
			g_free (label);
			g_free (stripped_name);
			continue;
		}

		button = eel_labeled_image_check_button_new (label, pixbuf);
		g_free (label);
		g_object_unref (pixbuf);

		/* Attach parameters and signal handler. */
		g_object_set_data_full (G_OBJECT (button),
					"baul_property_name",
					stripped_name,
					(GDestroyNotify) g_free);

		ctk_container_add (CTK_CONTAINER (emblems_table), button);
	}

	ctk_widget_show_all (emblems_table);

	ctk_widget_show (window);

	ctk_main ();

	return 0;
}
