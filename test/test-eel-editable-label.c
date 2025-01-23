/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

#include <config.h>

#include <ctk/ctk.h>

#include <eel/eel-editable-label.h>

static void
quit (CtkWidget *widget G_GNUC_UNUSED,
      gpointer   data G_GNUC_UNUSED)
{
	ctk_main_quit ();
}

int
main (int argc, char* argv[])
{
	CtkWidget *window;
	CtkWidget *label;
	CtkWidget *vbox;

	ctk_init (&argc, &argv);

	window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
	g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (quit), NULL);

	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

	ctk_container_add (CTK_CONTAINER (window), vbox);

	label = eel_editable_label_new ("Centered dsau dsfgsdfgoydsfiugy oiusdyfg iouysdf goiuys dfioguy siodufgy iusdyfgiu ydsf giusydf gouiysdfgoiuysdfg oiudyfsg Label");

	ctk_widget_set_size_request (label, 200, -1);
	eel_editable_label_set_line_wrap (EEL_EDITABLE_LABEL (label), TRUE);

	ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 4);

	label = eel_editable_label_new ("Left aligned label");

	ctk_label_set_xalign (CTK_LABEL (label), 0.0);

	ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 4);

	label = eel_editable_label_new ("Right aligned label");

	ctk_label_set_xalign (CTK_LABEL (label), 1.0);

	ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 4);

	ctk_window_set_default_size (CTK_WINDOW (window), 300, 300);

	ctk_widget_show_all (window);

	ctk_main ();

	return 0;
}
