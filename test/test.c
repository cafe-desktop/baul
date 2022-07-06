#include "test.h"
#include <sys/types.h>
#include <unistd.h>

void
test_init (int *argc,
	   char ***argv)
{
	ctk_init (argc, argv);

	eel_make_warnings_and_criticals_stop_in_debugger ();
}

int
test_quit (int exit_code)
{
	if (ctk_main_level () > 0) {
		ctk_main_quit ();
	}

	return exit_code;
}

void
test_delete_event (CtkWidget *widget,
		   CdkEvent *event,
		   gpointer callback_data)
{
	test_quit (0);
}

CtkWidget *
test_window_new (const char *title, guint border_width)
{
	CtkWidget *window;

	window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

	if (title != NULL) {
		ctk_window_set_title (CTK_WINDOW (window), title);
	}

	g_signal_connect (window, "delete_event",
                          G_CALLBACK (test_delete_event), NULL);

	ctk_container_set_border_width (CTK_CONTAINER (window), border_width);

	return window;
}

void
test_ctk_widget_set_background_image (CtkWidget *widget,
				      const char *image_name)
{
	EelBackground *background;
	char *uri;

	g_return_if_fail (CTK_IS_WIDGET (widget));
	g_return_if_fail (image_name != NULL);

	background = eel_get_widget_background (widget);

	uri = g_strdup_printf ("file://%s/%s", BAUL_DATADIR, image_name);

	eel_background_set_image_uri (background, uri);

	g_free (uri);
}

void
test_ctk_widget_set_background_color (CtkWidget *widget,
				      const char *color_spec)
{
	EelBackground *background;

	g_return_if_fail (CTK_IS_WIDGET (widget));
	g_return_if_fail (color_spec != NULL);

	background = eel_get_widget_background (widget);

	eel_background_set_color (background, color_spec);
}

CdkPixbuf *
test_pixbuf_new_named (const char *name, float scale)
{
	CdkPixbuf *pixbuf;
	char *path;

	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (scale >= 0.0, NULL);

	if (name[0] == '/') {
		path = g_strdup (name);
	} else {
		path = g_strdup_printf ("%s/%s", BAUL_DATADIR, name);
	}

	pixbuf = cdk_pixbuf_new_from_file (path, NULL);

	g_free (path);

	g_return_val_if_fail (pixbuf != NULL, NULL);

	if (scale != 1.0) {
		CdkPixbuf *scaled;
		float width = cdk_pixbuf_get_width (pixbuf) * scale;
		float height = cdk_pixbuf_get_width (pixbuf) * scale;

		scaled = cdk_pixbuf_scale_simple (pixbuf, width, height, CDK_INTERP_BILINEAR);

		g_object_unref (pixbuf);

		g_return_val_if_fail (scaled != NULL, NULL);

		pixbuf = scaled;
	}

	return pixbuf;
}

CtkWidget *
test_label_new (const char *text,
		gboolean with_background,
		int num_sizes_larger)
{
	CtkWidget *label;

	if (text == NULL) {
		text = "Foo";
	}

	label = ctk_label_new (text);

	return label;
}

void
test_window_set_title_with_pid (CtkWindow *window,
				const char *title)
{
	char *tmp;

	g_return_if_fail (CTK_IS_WINDOW (window));

	tmp = g_strdup_printf ("%lu: %s", (gulong) getpid (), title);
	ctk_window_set_title (CTK_WINDOW (window), tmp);
	g_free (tmp);
}

