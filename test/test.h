#ifndef TEST_H
#define TEST_H

#include <config.h>

#include <ctk/ctk.h>

#include <eel/eel-debug.h>
#include <eel/eel.h>
#include <libbaul-private/baul-file-utilities.h>

void       test_init                            (int                         *argc,
						 char                      ***argv);
int        test_quit                            (int                          exit_code);
void       test_delete_event                    (CtkWidget                   *widget,
						 CdkEvent                    *event,
						 gpointer                     callback_data);
CtkWidget *test_window_new                      (const char                  *title,
						 guint                        border_width);
void       test_ctk_widget_set_background_image (CtkWidget                   *widget,
						 const char                  *image_name);
void       test_ctk_widget_set_background_color (CtkWidget                   *widget,
						 const char                  *color_spec);
GdkPixbuf *test_pixbuf_new_named                (const char                  *name,
						 float                        scale);
CtkWidget *test_label_new                       (const char                  *text,
						 gboolean                     with_background,
						 int                          num_sizes_larger);
void       test_pixbuf_draw_rectangle_tiled     (GdkPixbuf                   *pixbuf,
						 const char                  *tile_name,
						 int                          x0,
						 int                          y0,
						 int                          x1,
						 int                          y1,
						 int                          opacity);
void       test_window_set_title_with_pid       (CtkWindow                   *window,
						 const char                  *title);

#endif /* TEST_H */
