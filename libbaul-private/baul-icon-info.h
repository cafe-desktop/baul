#ifndef BAUL_ICON_INFO_H
#define BAUL_ICON_INFO_H

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cdk/cdk.h>
#include <gio/gio.h>
#include <ctk/ctk.h>

#ifdef __cplusplus
extern "C" {
#endif

    /* Names for Baul's different zoom levels, from tiniest items to largest items */
    typedef enum {
        BAUL_ZOOM_LEVEL_SMALLEST,
        BAUL_ZOOM_LEVEL_SMALLER,
        BAUL_ZOOM_LEVEL_SMALL,
        BAUL_ZOOM_LEVEL_STANDARD,
        BAUL_ZOOM_LEVEL_LARGE,
        BAUL_ZOOM_LEVEL_LARGER,
        BAUL_ZOOM_LEVEL_LARGEST
    }
    BaulZoomLevel;

#define BAUL_ZOOM_LEVEL_N_ENTRIES (BAUL_ZOOM_LEVEL_LARGEST + 1)

    /* Nominal icon sizes for each Baul zoom level.
     * This scheme assumes that icons are designed to
     * fit in a square space, though each image needn't
     * be square. Since individual icons can be stretched,
     * each icon is not constrained to this nominal size.
     */
#define BAUL_ICON_SIZE_SMALLEST	16
#define BAUL_ICON_SIZE_SMALLER	24
#define BAUL_ICON_SIZE_SMALL	32
#define BAUL_ICON_SIZE_STANDARD	48
#define BAUL_ICON_SIZE_LARGE	72
#define BAUL_ICON_SIZE_LARGER	96
#define BAUL_ICON_SIZE_LARGEST     192

    /* Maximum size of an icon that the icon factory will ever produce */
#define BAUL_ICON_MAXIMUM_SIZE     320

    typedef struct _BaulIconInfo      BaulIconInfo;
    typedef struct _BaulIconInfoClass BaulIconInfoClass;


#define BAUL_TYPE_ICON_INFO                 (baul_icon_info_get_type ())
#define BAUL_ICON_INFO(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_ICON_INFO, BaulIconInfo))
#define BAUL_ICON_INFO_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_ICON_INFO, BaulIconInfoClass))
#define BAUL_IS_ICON_INFO(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_ICON_INFO))
#define BAUL_IS_ICON_INFO_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_ICON_INFO))
#define BAUL_ICON_INFO_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_ICON_INFO, BaulIconInfoClass))


    GType    baul_icon_info_get_type (void) G_GNUC_CONST;

    BaulIconInfo *    baul_icon_info_new_for_pixbuf               (GdkPixbuf         *pixbuf,
            int                scale);
    BaulIconInfo *    baul_icon_info_lookup                       (GIcon             *icon,
            int                size,
            int                scale);
    BaulIconInfo *    baul_icon_info_lookup_from_name             (const char        *name,
            int                size,
            int                scale);
    BaulIconInfo *    baul_icon_info_lookup_from_path             (const char        *path,
            int                size,
            int                scale);
    gboolean              baul_icon_info_is_fallback                  (BaulIconInfo  *icon);
    GdkPixbuf *           baul_icon_info_get_pixbuf                   (BaulIconInfo  *icon);
    cairo_surface_t *     baul_icon_info_get_surface                  (BaulIconInfo  *icon);
    GdkPixbuf *           baul_icon_info_get_pixbuf_nodefault         (BaulIconInfo  *icon);
    cairo_surface_t *     baul_icon_info_get_surface_nodefault        (BaulIconInfo  *icon);
    GdkPixbuf *           baul_icon_info_get_pixbuf_nodefault_at_size (BaulIconInfo  *icon,
            gsize              forced_size);
    cairo_surface_t *     baul_icon_info_get_surface_nodefault_at_size(BaulIconInfo  *icon,
            gsize              forced_size);
    GdkPixbuf *           baul_icon_info_get_pixbuf_at_size           (BaulIconInfo  *icon,
            gsize              forced_size);
    cairo_surface_t *     baul_icon_info_get_surface_at_size(BaulIconInfo  *icon,
            gsize              forced_size);
    gboolean              baul_icon_info_get_embedded_rect            (BaulIconInfo  *icon,
            CdkRectangle      *rectangle);
    gboolean              baul_icon_info_get_attach_points            (BaulIconInfo  *icon,
            CdkPoint         **points,
            gint              *n_points);
    const char* baul_icon_info_get_display_name(BaulIconInfo* icon);
    const char* baul_icon_info_get_used_name(BaulIconInfo* icon);

    void                  baul_icon_info_clear_caches                 (void);

    /* Relationship between zoom levels and icons sizes. */
    guint baul_get_icon_size_for_zoom_level          (BaulZoomLevel  zoom_level);
    float baul_get_relative_icon_size_for_zoom_level (BaulZoomLevel  zoom_level);

    guint baul_icon_get_larger_icon_size             (guint              size);
    guint baul_icon_get_smaller_icon_size            (guint              size);

    gint  baul_get_icon_size_for_stock_size          (CtkIconSize        size);
    guint baul_icon_get_emblem_size_for_icon_size    (guint              size);

gboolean baul_icon_theme_can_render              (GThemedIcon *icon);
GIcon * baul_user_special_directory_get_gicon (GUserDirectory directory);



#ifdef __cplusplus
}
#endif

#endif /* BAUL_ICON_INFO_H */

