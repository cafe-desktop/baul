#include <ctk/ctk.h>
#include <unistd.h>

#include <libbaul-private/baul-directory.h>
#include <libbaul-private/baul-search-directory.h>
#include <libbaul-private/baul-file.h>

void *client1, *client2;

#if 0
static gboolean
quit_cb (gpointer data)
{
	ctk_main_quit ();

	return FALSE;
}
#endif

static void
files_added (BaulDirectory *directory G_GNUC_UNUSED,
	     GList         *added_files)
{
#if 0
	GList *list;

	for (list = added_files; list != NULL; list = list->next) {
		BaulFile *file = list->data;

		g_print (" - %s\n", baul_file_get_uri (file));
	}
#endif

	g_print ("files added: %d files\n",
		 g_list_length (added_files));
}

static void
files_changed (BaulDirectory *directory G_GNUC_UNUSED,
	       GList         *changed_files)
{
#if 0
	GList *list;

	for (list = changed_files; list != NULL; list = list->next) {
		BaulFile *file = list->data;

		g_print (" - %s\n", baul_file_get_uri (file));
	}
#endif
	g_print ("files changed: %d\n",
		 g_list_length (changed_files));
}

static gboolean
force_reload (BaulDirectory *directory)
{
	g_print ("forcing reload!\n");

	baul_directory_force_reload (directory);

	return FALSE;
}

static void
done_loading (BaulDirectory *directory)
{
	static int i = 0;

	g_print ("done loading\n");

	if (i == 0) {
		g_timeout_add (5000, (GSourceFunc)force_reload, directory);
		i++;
	} else {
	}
}

int
main (int argc, char **argv)
{
	BaulDirectory *directory;
	BaulQuery *query;
	client1 = g_new0 (int, 1);
	client2 = g_new0 (int, 1);

	ctk_init (&argc, &argv);

	query = baul_query_new ();
	baul_query_set_text (query, "richard hult");
	directory = baul_directory_get_by_uri ("x-baul-search://0/");
	baul_search_directory_set_query (BAUL_SEARCH_DIRECTORY (directory), query);
	g_object_unref (query);

	g_signal_connect (directory, "files-added", G_CALLBACK (files_added), NULL);
	g_signal_connect (directory, "files-changed", G_CALLBACK (files_changed), NULL);
	g_signal_connect (directory, "done-loading", G_CALLBACK (done_loading), NULL);
	baul_directory_file_monitor_add (directory, client1, TRUE,
					     BAUL_FILE_ATTRIBUTE_INFO,
					     NULL, NULL);


	ctk_main ();
	return 0;
}
