#include <eel/eel-labeled-image.h>

#include "test.h"

static const char pixbuf_name[] = "/usr/share/pixmaps/cafe-globe.png";

static void
button_callback (CtkWidget *button,
		 gpointer callback_data)
{
	const char *info = callback_data;
	g_return_if_fail (CTK_IS_BUTTON (button));

	g_print ("%s(%p)\n", info, button);
}

static CtkWidget *
labeled_image_button_window_new (const char *title,
				 CdkPixbuf *pixbuf)
{
	CtkWidget *window;
	CtkWidget *vbox;
	CtkWidget *button;
	CtkWidget *toggle_button;
	CtkWidget *check_button;
	CtkWidget *plain;

	window = test_window_new (title, 20);
	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
	ctk_container_add (CTK_CONTAINER (window), vbox);

	if (1) button = eel_labeled_image_button_new ("CtkButton with LabeledImage", pixbuf);
	if (1) toggle_button = eel_labeled_image_toggle_button_new ("CtkToggleButton with LabeledImage", pixbuf);
	if (1) check_button = eel_labeled_image_check_button_new ("CtkCheckButton with LabeledImage", pixbuf);
	if (1) {
		plain = eel_labeled_image_new ("Plain LabeledImage", pixbuf);
		eel_labeled_image_set_can_focus (EEL_LABELED_IMAGE (plain), TRUE);
	}

	if (button) ctk_box_pack_start (CTK_BOX (vbox), button, TRUE, TRUE, 0);
	if (toggle_button) ctk_box_pack_start (CTK_BOX (vbox), toggle_button, TRUE, TRUE, 0);
	if (check_button) ctk_box_pack_start (CTK_BOX (vbox), check_button, TRUE, TRUE, 0);
	if (plain) ctk_box_pack_start (CTK_BOX (vbox), plain, TRUE, TRUE, 0);

	if (button) {
		g_signal_connect (button, "enter", G_CALLBACK (button_callback), "enter");
		g_signal_connect (button, "leave", G_CALLBACK (button_callback), "leave");
		g_signal_connect (button, "pressed", G_CALLBACK (button_callback), "pressed");
		g_signal_connect (button, "released", G_CALLBACK (button_callback), "released");
		g_signal_connect (button, "clicked", G_CALLBACK (button_callback), "clicked");
	}

	ctk_widget_show_all (vbox);

	return window;
}

int
main (int argc, char* argv[])
{
	CtkWidget *labeled_image_window = NULL;
	CtkWidget *labeled_image_button_window = NULL;
	CdkPixbuf *pixbuf = NULL;

	test_init (&argc, &argv);

	if (1) pixbuf = test_pixbuf_new_named (pixbuf_name, 1.0);
	if (1) labeled_image_button_window = labeled_image_button_window_new ("LabeledImage in CtkButton Test", pixbuf);

	eel_gdk_pixbuf_unref_if_not_null (pixbuf);

	if (labeled_image_window) ctk_widget_show (labeled_image_window);
	if (labeled_image_button_window) ctk_widget_show (labeled_image_button_window);

	ctk_main ();

	return test_quit (EXIT_SUCCESS);
}
