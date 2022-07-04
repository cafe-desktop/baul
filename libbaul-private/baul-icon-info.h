#ifndef CAJA_ICON_INFO_H
#define CAJA_ICON_INFO_H

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

    /* Names for Caja's different zoom levels, from tiniest items to largest items */
    typedef enum {
        CAJA_ZOOM_LEVEL_SMALLEST,
        CAJA_ZOOM_LEVEL_SMALLER,
        CAJA_ZOOM_LEVEL_SMALL,
        CAJA_ZOOM_LEVEL_STANDARD,
        CAJA_ZOOM_LEVEL_LARGE,
        CAJA_ZOOM_LEVEL_LARGER,
        CAJA_ZOOM_LEVEL_LARGEST
    }
    CajaZoomLevel;

#define CAJA_ZOOM_LEVEL_N_ENTRIES (CAJA_ZOOM_LEVEL_LARGEST + 1)

    /* Nominal icon sizes for each Caja zoom level.
     * This scheme assumes that icons are designed to
     * fit in a square space, though each image needn't
     * be square. Since individual icons can be stretched,
     * each icon is not constrained to this nominal size.
     */
#define CAJA_ICON_SIZE_SMALLEST	16
#define CAJA_ICON_SIZE_SMALLER	24
#define CAJA_ICON_SIZE_SMALL	32
#define CAJA_ICON_SIZE_STANDARD	48
#define CAJA_ICON_SIZE_LARGE	72
#define CAJA_ICON_SIZE_LARGER	96
#define CAJA_ICON_SIZE_LARGEST     192

    /* Maximum size of an icon that the icon factory will ever produce */
#define CAJA_ICON_MAXIMUM_SIZE     320

    typedef struct _CajaIconInfo      CajaIconInfo;
    typedef struct _CajaIconInfoClass CajaIconInfoClass;


#define CAJA_TYPE_ICON_INFO                 (baul_icon_info_get_type ())
#define CAJA_ICON_INFO(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAJA_TYPE_ICON_INFO, CajaIconInfo))
#define CAJA_ICON_INFO_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), CAJA_TYPE_ICON_INFO, CajaIconInfoClass))
#define CAJA_IS_ICON_INFO(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAJA_TYPE_ICON_INFO))
#define CAJA_IS_ICON_INFO_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), CAJA_TYPE_ICON_INFO))
#define CAJA_ICON_INFO_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), CAJA_TYPE_ICON_INFO, CajaIconInfoClass))


    GType    baul_icon_info_get_type (void) G_GNUC_CONST;

    CajaIconInfo *    baul_icon_info_new_for_pixbuf               (GdkPixbuf         *pixbuf,
            int                scale);
    CajaIconInfo *    baul_icon_info_lookup                       (GIcon             *icon,
            int                size,
            int                scale);
    CajaIconInfo *    baul_icon_info_lookup_from_name             (const char        *name,
            int                size,
            int                scale);
    CajaIconInfo *    baul_icon_info_lookup_from_path             (const char        *path,
            int                size,
            int                scale);
    gboolean              baul_icon_info_is_fallback                  (CajaIconInfo  *icon);
    GdkPixbuf *           baul_icon_info_get_pixbuf                   (CajaIconInfo  *icon);
    cairo_surface_t *     baul_icon_info_get_surface                  (CajaIconInfo  *icon);
    GdkPixbuf *           baul_icon_info_get_pixbuf_nodefault         (CajaIconInfo  *icon);
    cairo_surface_t *     baul_icon_info_get_surface_nodefault        (CajaIconInfo  *icon);
    GdkPixbuf *           baul_icon_info_get_pixbuf_nodefault_at_size (CajaIconInfo  *icon,
            gsize              forced_size);
    cairo_surface_t *     baul_icon_info_get_surface_nodefault_at_size(CajaIconInfo  *icon,
            gsize              forced_size);
    GdkPixbuf *           baul_icon_info_get_pixbuf_at_size           (CajaIconInfo  *icon,
            gsize              forced_size);
    cairo_surface_t *     baul_icon_info_get_surface_at_size(CajaIconInfo  *icon,
            gsize              forced_size);
    gboolean              baul_icon_info_get_embedded_rect            (CajaIconInfo  *icon,
            GdkRectangle      *rectangle);
    gboolean              baul_icon_info_get_attach_points            (CajaIconInfo  *icon,
            GdkPoint         **points,
            gint              *n_points);
    const char* baul_icon_info_get_display_name(CajaIconInfo* icon);
    const char* baul_icon_info_get_used_name(CajaIconInfo* icon);

    void                  baul_icon_info_clear_caches                 (void);

    /* Relationship between zoom levels and icons sizes. */
    guint baul_get_icon_size_for_zoom_level          (CajaZoomLevel  zoom_level);
    float baul_get_relative_icon_size_for_zoom_level (CajaZoomLevel  zoom_level);

    guint baul_icon_get_larger_icon_size             (guint              size);
    guint baul_icon_get_smaller_icon_size            (guint              size);

    gint  baul_get_icon_size_for_stock_size          (GtkIconSize        size);
    guint baul_icon_get_emblem_size_for_icon_size    (guint              size);

gboolean baul_icon_theme_can_render              (GThemedIcon *icon);
GIcon * baul_user_special_directory_get_gicon (GUserDirectory directory);



#ifdef __cplusplus
}
#endif

#endif /* CAJA_ICON_INFO_H */

