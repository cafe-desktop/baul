#include <libbaul-private/baul-file-operations.h>
#include <libbaul-private/baul-progress-info.h>

#include "test.h"

static void
copy_done (GHashTable *debuting_uris, gpointer data)
{
	g_print ("Copy done\n");
}

static void
changed_cb (BaulProgressInfo *info,
	    gpointer data)
{
	g_print ("Changed: %s -- %s\n",
		 baul_progress_info_get_status (info),
		 baul_progress_info_get_details (info));
}

static void
progress_changed_cb (BaulProgressInfo *info,
		     gpointer data)
{
	g_print ("Progress changed: %f\n",
		 baul_progress_info_get_progress (info));
}

static void
finished_cb (BaulProgressInfo *info,
	     gpointer data)
{
	g_print ("Finished\n");
	ctk_main_quit ();
}

int
main (int argc, char* argv[])
{
	CtkWidget *window;
	GList *sources;
	GFile *dest;
	GFile *source;
	int i;
	GList *infos;
	BaulProgressInfo *progress_info;

	test_init (&argc, &argv);

	if (argc < 3) {
		g_print ("Usage test-copy <sources...> <dest dir>\n");
		return 1;
	}

	sources = NULL;
	for (i = 1; i < argc - 1; i++) {
		source = g_file_new_for_commandline_arg (argv[i]);
		sources = g_list_prepend (sources, source);
	}
	sources = g_list_reverse (sources);

	dest = g_file_new_for_commandline_arg (argv[i]);

	window = test_window_new ("copy test", 5);

	ctk_widget_show (window);

	baul_file_operations_copy (sources,
				       NULL /* GArray *relative_item_points */,
				       dest,
				       GTK_WINDOW (window),
				       copy_done, NULL);

	infos = baul_get_all_progress_info ();

	if (infos == NULL) {
		return 0;
	}

	progress_info = BAUL_PROGRESS_INFO (infos->data);

	g_signal_connect (progress_info, "changed", (GCallback)changed_cb, NULL);
	g_signal_connect (progress_info, "progress-changed", (GCallback)progress_changed_cb, NULL);
	g_signal_connect (progress_info, "finished", (GCallback)finished_cb, NULL);

	ctk_main ();

	return 0;
}


