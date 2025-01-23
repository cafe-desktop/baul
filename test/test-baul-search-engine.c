#include <ctk/ctk.h>

#include <libbaul-private/baul-search-engine.h>

static void
hits_added_cb (BaulSearchEngine *engine G_GNUC_UNUSED,
	       GSList           *hits)
{
	g_print ("hits added\n");
	while (hits) {
		g_print (" - %s\n", (char *)hits->data);
		hits = hits->next;
	}
}

static void
hits_subtracted_cb (BaulSearchEngine *engine G_GNUC_UNUSED,
		    GSList           *hits)
{
	g_print ("hits subtracted\n");
	while (hits) {
		g_print (" - %s\n", (char *)hits->data);
		hits = hits->next;
	}
}

static void
finished_cb (BaulSearchEngine *engine G_GNUC_UNUSED)
{
	g_print ("finished!\n");
//	ctk_main_quit ();
}

int
main (int argc, char* argv[])
{
	BaulSearchEngine *engine;
	BaulQuery *query;

	ctk_init (&argc, &argv);

	engine = baul_search_engine_new ();
	g_signal_connect (engine, "hits-added",
			  G_CALLBACK (hits_added_cb), NULL);
	g_signal_connect (engine, "hits-subtracted",
			  G_CALLBACK (hits_subtracted_cb), NULL);
	g_signal_connect (engine, "finished",
			  G_CALLBACK (finished_cb), NULL);

	query = baul_query_new ();
	baul_query_set_text (query, "richard hult");
	baul_search_engine_set_query (engine, query);
	g_object_unref (query);

	baul_search_engine_start (engine);

	ctk_main ();
	return 0;
}
