/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * gdl-dock-item.c
 *
 * Author: Gustavo Giráldez <gustavo.giraldez@gmx.net>
 *         Naba Kumar  <naba@gnome.org>
 *
 * Based on GnomeDockItem/BonoboDockItem.  Original copyright notice follows.
 *
 * Copyright (C) 1998 Ettore Perazzoli
 * Copyright (C) 1998 Elliot Lee
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n-lib.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>

#include "gdl-dock.h"
#include "gdl-dock-item.h"
#include "gdl-dock-item-grip.h"
#include "gdl-dock-notebook.h"
#include "gdl-dock-paned.h"
#include "gdl-dock-master.h"
#include "libgdltypebuiltins.h"
#include "libgdlmarshal.h"

/**
 * SECTION:gdl-dock-item
 * @title: GdlDockItem
 * @short_description: Adds docking capability to its child widget.
 * @see_also: #GdlDockItem
 * @stability: Unstable
 *
 * A dock item is a container widget that can be docked at different place.
 * It accepts a single child and adds a grip allowing the user to click on it
 * to drag and drop the widget.
 *
 * The grip is implemented as a #GdlDockItemGrip.
 */

#define NEW_DOCK_ITEM_RATIO 0.3

/* ----- Private prototypes ----- */

static void  gdl_dock_item_base_class_init (GdlDockItemClass *class);
static void  gdl_dock_item_class_init     (GdlDockItemClass *class);
static void  gdl_dock_item_init		  (GdlDockItem *item);

static GObject *gdl_dock_item_constructor (GType                  type,
                                           guint                  n_construct_properties,
                                           GObjectConstructParam *construct_param);

static void  gdl_dock_item_set_property  (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static void  gdl_dock_item_get_property  (GObject      *object,
                                          guint         prop_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);

static void  gdl_dock_item_dispose       (GObject      *object);

static void  gdl_dock_item_add           (GtkContainer *container,
                                          GtkWidget    *widget);
static void  gdl_dock_item_remove        (GtkContainer *container,
                                          GtkWidget    *widget);
static void  gdl_dock_item_forall        (GtkContainer *container,
                                          gboolean      include_internals,
                                          GtkCallback   callback,
                                          gpointer      callback_data);
static GType gdl_dock_item_child_type  (GtkContainer *container);

static void  gdl_dock_item_get_preferred_width  (GtkWidget *widget,
                                                 gint      *minimum,
                                                 gint      *natural);
static void  gdl_dock_item_get_preferred_height (GtkWidget *widget,
                                                 gint      *minimum,
                                                 gint      *natural);

static void  gdl_dock_item_set_focus_child (GtkContainer *container,
                                            GtkWidget    *widget);

static void  gdl_dock_item_size_allocate (GtkWidget *widget,
                                          GtkAllocation *allocation);
static void  gdl_dock_item_map           (GtkWidget *widget);
static void  gdl_dock_item_unmap         (GtkWidget *widget);
static void  gdl_dock_item_realize       (GtkWidget *widget);

static void  gdl_dock_item_move_focus_child (GdlDockItem      *item,
                                             GtkDirectionType  dir);

static gint  gdl_dock_item_button_changed (GtkWidget *widget,
                                           GdkEventButton *event);
static gint  gdl_dock_item_motion         (GtkWidget *widget,
                                           GdkEventMotion *event);
static gboolean  gdl_dock_item_key_press  (GtkWidget *widget,
                                           GdkEventKey *event);

static gboolean gdl_dock_item_dock_request (GdlDockObject    *object,
                                            gint              x,
                                            gint              y,
                                            GdlDockRequest   *request);
static void     gdl_dock_item_dock         (GdlDockObject    *object,
                                            GdlDockObject    *requestor,
                                            GdlDockPlacement  position,
                                            GValue           *other_data);
static void     gdl_dock_item_present      (GdlDockObject     *object,
                                             GdlDockObject     *child);


static void  gdl_dock_item_popup_menu    (GdlDockItem *item,
                                          guint        button,
                                          guint32      time);
static void  gdl_dock_item_drag_start    (GdlDockItem *item);
static gboolean gdl_dock_item_drag_end   (GdlDockItem *item,
                                          gboolean     cancel);

static void  gdl_dock_item_tab_button    (GtkWidget      *widget,
                                          GdkEventButton *event,
                                          gpointer        data);

static void  gdl_dock_item_hide_cb       (GtkWidget   *widget,
                                          GdlDockItem *item);

static void  gdl_dock_item_lock_cb       (GtkWidget   *widget,
                                          GdlDockItem *item);

static void  gdl_dock_item_unlock_cb     (GtkWidget   *widget,
                                          GdlDockItem *item);

static void  gdl_dock_item_showhide_grip (GdlDockItem *item);

static void  gdl_dock_item_real_set_orientation (GdlDockItem    *item,
                                                 GtkOrientation  orientation);

static void gdl_dock_param_export_gtk_orientation (const GValue *src,
                                                   GValue       *dst);
static void gdl_dock_param_import_gtk_orientation (const GValue *src,
                                                   GValue       *dst);



/* ----- Class variables and definitions ----- */

enum {
    PROP_0,
    PROP_ORIENTATION,
    PROP_RESIZE,
    PROP_BEHAVIOR,
    PROP_LOCKED,
    PROP_PREFERRED_WIDTH,
    PROP_PREFERRED_HEIGHT,
    PROP_ICONIFIED,
    PROP_CLOSED,
};

enum {
    DOCK_DRAG_BEGIN,
    DOCK_DRAG_MOTION,
    DOCK_DRAG_END,
    SELECTED,
    DESELECTED,
    MOVE_FOCUS_CHILD,
    LAST_SIGNAL
};

static guint gdl_dock_item_signals [LAST_SIGNAL] = { 0 };

#define GDL_DOCK_ITEM_GRIP_SHOWN(item) \
    (GDL_DOCK_ITEM_HAS_GRIP (item))

struct _GdlDockItemPrivate {
    GtkWidget *child;

    GdlDockItemBehavior  behavior;
    GtkOrientation       orientation;

    guint                iconified: 1;
    guint                resize : 1;
    guint                in_predrag : 1;
    guint                in_drag : 1;

    gint                 dragoff_x, dragoff_y;

    GtkWidget *menu;
    GtkWidget *menu_item_hide;

    gboolean   grip_shown;
    GtkWidget *grip;
    guint      grip_size;

    GtkWidget *tab_label;
    gboolean  intern_tab_label;
    guint     notify_label;
    guint     notify_stock_id;

    gint       preferred_width;
    gint       preferred_height;

    gint       start_x, start_y;
};

struct _GdlDockItemClassPrivate {
    gboolean        has_grip;

    GtkCssProvider *css;
};

/* FIXME: implement the rest of the behaviors */

#define SPLIT_RATIO  0.4


/* ----- Private functions ----- */

/* It is not possible to use G_DEFINE_TYPE_* macro for GdlDockItem because it
 * has some class private data. The priv pointer has to be initialized in
 * the base class initialization function because this function is called for
 * each derived type.
 */
static gpointer gdl_dock_item_parent_class = NULL;

GType
gdl_dock_item_get_type (void)
{
    static GType gtype = 0;

    if (G_UNLIKELY (gtype == 0)) {
        const GTypeInfo gtype_info = {
	    sizeof (GdlDockItemClass),
	    (GBaseInitFunc) gdl_dock_item_base_class_init,
	    NULL,
	    (GClassInitFunc) gdl_dock_item_class_init,
	    NULL,		/* class_finalize */
	    NULL,		/* class_data */
	    sizeof (GdlDockItem),
	    0,		/* n_preallocs */
	    (GInstanceInitFunc) gdl_dock_item_init,
	    NULL,		/* value_table */
        };

        gtype = g_type_register_static (GDL_TYPE_DOCK_OBJECT,
                                        "GdlDockItem",
                                        &gtype_info,
                                        0);

        g_type_add_class_private (gtype, sizeof (GdlDockItemClassPrivate));
    }

    return gtype;
}


static void
add_tab_bindings (GtkBindingSet    *binding_set,
		  GdkModifierType   modifiers,
		  GtkDirectionType  direction)
{
    gtk_binding_entry_add_signal (binding_set, GDK_KEY_Tab, modifiers,
                                  "move_focus_child", 1,
                                  GTK_TYPE_DIRECTION_TYPE, direction);
    gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Tab, modifiers,
                                  "move_focus_child", 1,
                                  GTK_TYPE_DIRECTION_TYPE, direction);
}

static void
add_arrow_bindings (GtkBindingSet    *binding_set,
		    guint             keysym,
		    GtkDirectionType  direction)
{
    guint keypad_keysym = keysym - GDK_KEY_Left + GDK_KEY_KP_Left;

    gtk_binding_entry_add_signal (binding_set, keysym, 0,
                                  "move_focus_child", 1,
                                  GTK_TYPE_DIRECTION_TYPE, direction);
    gtk_binding_entry_add_signal (binding_set, keysym, GDK_CONTROL_MASK,
                                  "move_focus_child", 1,
                                  GTK_TYPE_DIRECTION_TYPE, direction);
    gtk_binding_entry_add_signal (binding_set, keysym, GDK_CONTROL_MASK,
                                  "move_focus_child", 1,
                                  GTK_TYPE_DIRECTION_TYPE, direction);
    gtk_binding_entry_add_signal (binding_set, keypad_keysym, GDK_CONTROL_MASK,
                                  "move_focus_child", 1,
                                  GTK_TYPE_DIRECTION_TYPE, direction);
}

static void
gdl_dock_item_base_class_init (GdlDockItemClass *klass)
{
    klass->priv = G_TYPE_CLASS_GET_PRIVATE (klass, GDL_TYPE_DOCK_ITEM, GdlDockItemClassPrivate);
}

static void
gdl_dock_item_class_init (GdlDockItemClass *klass)
{
    GObjectClass       *object_class;
    GtkWidgetClass     *widget_class;
    GtkContainerClass  *container_class;
    GdlDockObjectClass *dock_object_class;
    static const gchar style[] =
       "* {\n"
           "padding: 0;\n"
       "}";
    GtkBindingSet      *binding_set;

    gdl_dock_item_parent_class = g_type_class_peek_parent (klass);

    object_class = G_OBJECT_CLASS (klass);
    widget_class = GTK_WIDGET_CLASS (klass);
    container_class = GTK_CONTAINER_CLASS (klass);
    dock_object_class = GDL_DOCK_OBJECT_CLASS (klass);

    object_class->constructor = gdl_dock_item_constructor;
    object_class->set_property = gdl_dock_item_set_property;
    object_class->get_property = gdl_dock_item_get_property;
    object_class->dispose = gdl_dock_item_dispose;

    widget_class->realize = gdl_dock_item_realize;
    widget_class->map = gdl_dock_item_map;
    widget_class->unmap = gdl_dock_item_unmap;
    widget_class->get_preferred_width = gdl_dock_item_get_preferred_width;
    widget_class->get_preferred_height = gdl_dock_item_get_preferred_height;
    widget_class->size_allocate = gdl_dock_item_size_allocate;
    widget_class->button_press_event = gdl_dock_item_button_changed;
    widget_class->button_release_event = gdl_dock_item_button_changed;
    widget_class->motion_notify_event = gdl_dock_item_motion;
    widget_class->key_press_event = gdl_dock_item_key_press;

    container_class->add = gdl_dock_item_add;
    container_class->remove = gdl_dock_item_remove;
    container_class->forall = gdl_dock_item_forall;
    container_class->child_type = gdl_dock_item_child_type;
    container_class->set_focus_child = gdl_dock_item_set_focus_child;
    gtk_container_class_handle_border_width (container_class);

    gdl_dock_object_class_set_is_compound (object_class, FALSE);
    dock_object_class->dock_request = gdl_dock_item_dock_request;
    dock_object_class->dock = gdl_dock_item_dock;
    dock_object_class->present = gdl_dock_item_present;

    klass->priv->has_grip = TRUE;
    klass->dock_drag_begin = NULL;
    klass->dock_drag_motion = NULL;
    klass->dock_drag_end = NULL;
    klass->move_focus_child = gdl_dock_item_move_focus_child;
    klass->set_orientation = gdl_dock_item_real_set_orientation;

    /* properties */

    /**
     * GdlDockItem:orientation:
     *
     * The orientation of the docking item. If the orientation is set to
     * #GTK_ORIENTATION_VERTICAL, the grip widget will be shown along
     * the top of the edge of item (if it is not hidden). If the
     * orientation is set to #GTK_ORIENTATION_HORIZONTAL, the grip
     * widget will be shown down the left edge of the item (even if the
     * widget text direction is set to RTL).
     */
    g_object_class_install_property (
        object_class, PROP_ORIENTATION,
        g_param_spec_enum ("orientation", _("Orientation"),
                           _("Orientation of the docking item"),
                           GTK_TYPE_ORIENTATION,
                           GTK_ORIENTATION_VERTICAL,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                           GDL_DOCK_PARAM_EXPORT));

    /* --- register exporter/importer for GTK_ORIENTATION */
    g_value_register_transform_func (GTK_TYPE_ORIENTATION, GDL_TYPE_DOCK_PARAM,
                                     gdl_dock_param_export_gtk_orientation);
    g_value_register_transform_func (GDL_TYPE_DOCK_PARAM, GTK_TYPE_ORIENTATION,
                                     gdl_dock_param_import_gtk_orientation);
    /* --- end of registration */

    g_object_class_install_property (
        object_class, PROP_RESIZE,
        g_param_spec_boolean ("resize", _("Resizable"),
                              _("If set, the dock item can be resized when "
                                "docked in a GtkPanel widget"),
                              TRUE,
                              G_PARAM_READWRITE | GDL_DOCK_PARAM_EXPORT));

    g_object_class_install_property (
        object_class, PROP_BEHAVIOR,
        g_param_spec_flags ("behavior", _("Item behavior"),
                            _("General behavior for the dock item (i.e. "
                              "whether it can float, if it's locked, etc.)"),
                            GDL_TYPE_DOCK_ITEM_BEHAVIOR,
                            GDL_DOCK_ITEM_BEH_NORMAL,
                            G_PARAM_READWRITE));

    g_object_class_install_property (
        object_class, PROP_LOCKED,
        g_param_spec_boolean ("locked", _("Locked"),
                              _("If set, the dock item cannot be dragged around "
                                "and it doesn't show a grip"),
                              FALSE,
                              G_PARAM_READWRITE |
                              GDL_DOCK_PARAM_EXPORT));

    g_object_class_install_property (
        object_class, PROP_PREFERRED_WIDTH,
        g_param_spec_int ("preferred-width", _("Preferred width"),
                          _("Preferred width for the dock item"),
                          -1, G_MAXINT, -1,
                          G_PARAM_READWRITE));

    g_object_class_install_property (
        object_class, PROP_PREFERRED_HEIGHT,
        g_param_spec_int ("preferred-height", _("Preferred height"),
                          _("Preferred height for the dock item"),
                          -1, G_MAXINT, -1,
                          G_PARAM_READWRITE));

    /**
     * GdlDockItem:iconified:
     *
     * If set, the dock item is hidden but it has a corresponding icon in the
     * dock bar allowing to show it again.
     *
     * Since: 3.6
     */
    g_object_class_install_property (
        object_class, PROP_ICONIFIED,
        g_param_spec_boolean ("iconified", _("Iconified"),
                              _("If set, the dock item is hidden but it has a "
                                "corresponding icon in the dock bar allowing to "
                                "show it again."),
                              FALSE,
                              G_PARAM_READWRITE |
                              GDL_DOCK_PARAM_EXPORT));

    /**
     * GdlDockItem:closed:
     *
     * If set, the dock item is closed.
     *
     * Since: 3.6
     */
    g_object_class_install_property (
        object_class, PROP_CLOSED,
        g_param_spec_boolean ("closed", _("Closed"),
                              _("Whether the widget is closed."),
                              FALSE,
                              G_PARAM_READWRITE |
                              GDL_DOCK_PARAM_EXPORT));

    /**
     * GdlDockItem::dock-drag-begin:
     * @item: The dock item which is being dragged.
     *
     * Signals that the dock item has begun to be dragged.
     **/
    gdl_dock_item_signals [DOCK_DRAG_BEGIN] =
        g_signal_new ("dock-drag-begin",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GdlDockItemClass, dock_drag_begin),
                      NULL, /* accumulator */
                      NULL, /* accu_data */
                      gdl_marshal_VOID__VOID,
                      G_TYPE_NONE,
                      0);

    /**
     * GdlDockItem::dock-drag-motion:
     * @item: The dock item which is being dragged.
     * @device: The device used.
     * @x: The x-position that the dock item has been dragged to.
     * @y: The y-position that the dock item has been dragged to.
     *
     * Signals that a dock item dragging motion event has occured.
     **/
    gdl_dock_item_signals [DOCK_DRAG_MOTION] =
        g_signal_new ("dock-drag-motion",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GdlDockItemClass, dock_drag_motion),
                      NULL, /* accumulator */
                      NULL, /* accu_data */
                      gdl_marshal_VOID__OBJECT_INT_INT,
                      G_TYPE_NONE,
                      3,
                      GDK_TYPE_DEVICE,
                      G_TYPE_INT,
                      G_TYPE_INT);

    /**
     * GdlDockItem::dock-drag-end:
     * @item: The dock item which is no longer being dragged.
     * @cancel: This value is set to TRUE if the drag was cancelled by
     * the user. #cancel is set to FALSE if the drag was accepted.
     *
     * Signals that the dock item dragging has ended.
     **/
    gdl_dock_item_signals [DOCK_DRAG_END] =
        g_signal_new ("dock_drag_end",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GdlDockItemClass, dock_drag_end),
                      NULL, /* accumulator */
                      NULL, /* accu_data */
                      gdl_marshal_VOID__BOOLEAN,
                      G_TYPE_NONE,
                      1,
                      G_TYPE_BOOLEAN);

    /**
     * GdlDockItem::selected:
     *
     * Signals that this dock has been selected from a switcher.
     */
    gdl_dock_item_signals [SELECTED] =
        g_signal_new ("selected",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      0,
                      NULL,
                      NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE,
                      0);

    /**
     * GdlDockItem::move-focus-child:
     * @gdldockitem: The dock item in which a change of focus is requested
     * @dir: The direction in which to move focus
     *
     * The ::move-focus-child signal is emitted when a change of focus is
     * requested for the child widget of a dock item.  The @dir parameter
     * specifies the direction in which focus is to be shifted.
     *
     * Since: 3.3.2
     */
    gdl_dock_item_signals [MOVE_FOCUS_CHILD] =
        g_signal_new ("move_focus_child",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (GdlDockItemClass, move_focus_child),
                      NULL, /* accumulator */
                      NULL, /* accu_data */
                      gdl_marshal_VOID__ENUM,
                      G_TYPE_NONE,
                      1,
                      GTK_TYPE_DIRECTION_TYPE);


    /**
     * GdlDockItem::deselected:
     *
     * Signals that this dock has been deselected in a switcher.
     */
    gdl_dock_item_signals [DESELECTED] =
        g_signal_new ("deselected",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      0,
                      NULL,
                      NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE,
                      0);

    /* key bindings */

    binding_set = gtk_binding_set_by_class (klass);

    add_arrow_bindings (binding_set, GDK_KEY_Up, GTK_DIR_UP);
    add_arrow_bindings (binding_set, GDK_KEY_Down, GTK_DIR_DOWN);
    add_arrow_bindings (binding_set, GDK_KEY_Left, GTK_DIR_LEFT);
    add_arrow_bindings (binding_set, GDK_KEY_Right, GTK_DIR_RIGHT);

    add_tab_bindings (binding_set, 0, GTK_DIR_TAB_FORWARD);
    add_tab_bindings (binding_set, GDK_CONTROL_MASK, GTK_DIR_TAB_FORWARD);
    add_tab_bindings (binding_set, GDK_SHIFT_MASK, GTK_DIR_TAB_BACKWARD);
    add_tab_bindings (binding_set, GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_DIR_TAB_BACKWARD);

    g_type_class_add_private (object_class, sizeof (GdlDockItemPrivate));

    /* set the style */
    klass->priv->css = gtk_css_provider_new ();
    gtk_css_provider_load_from_data (klass->priv->css, style, -1, NULL);
}

static void
gdl_dock_item_init (GdlDockItem *item)
{
    item->priv = G_TYPE_INSTANCE_GET_PRIVATE (item,
                                              GDL_TYPE_DOCK_ITEM,
                                              GdlDockItemPrivate);

    gtk_widget_set_has_window (GTK_WIDGET (item), TRUE);
    gtk_widget_set_can_focus (GTK_WIDGET (item), TRUE);

    item->priv->child = NULL;

    item->priv->orientation = GTK_ORIENTATION_VERTICAL;
    item->priv->behavior = GDL_DOCK_ITEM_BEH_NORMAL;

    item->priv->iconified = FALSE;

    item->priv->resize = TRUE;

    item->priv->dragoff_x = item->priv->dragoff_y = 0;
    item->priv->in_predrag = item->priv->in_drag = FALSE;

    item->priv->menu = NULL;
    item->priv->menu_item_hide = NULL;

    item->priv->preferred_width = item->priv->preferred_height = -1;
    item->priv->tab_label = NULL;
    item->priv->intern_tab_label = FALSE;
}

static gboolean
on_grab_broken_event (GtkWidget *widget,
                      GdkEvent  *event,
                      gpointer   user_data)
{
    return gdl_dock_item_drag_end (GDL_DOCK_ITEM (user_data), TRUE);
}

static void
on_long_name_changed (GObject* item,
                      GParamSpec* spec,
                      gpointer user_data)
{
    gchar* long_name;
    g_object_get (item, "long-name", &long_name, NULL);
    gtk_label_set_label (GTK_LABEL (user_data), long_name);
    g_free(long_name);
}

static void
on_stock_id_changed (GObject* item,
                      GParamSpec* spec,
                      gpointer user_data)
{
    gchar* stock_id;
    g_object_get (item, "stock_id", &stock_id, NULL);
    gtk_image_set_from_stock (GTK_IMAGE (user_data), stock_id, GTK_ICON_SIZE_MENU);
    g_free(stock_id);
}

static GObject *
gdl_dock_item_constructor (GType                  type,
                           guint                  n_construct_properties,
                           GObjectConstructParam *construct_param)
{
    GObject *g_object;

    g_object = G_OBJECT_CLASS (gdl_dock_item_parent_class)-> constructor (type,
                                                                     n_construct_properties,
                                                                     construct_param);
    if (g_object) {
        GdlDockItem *item = GDL_DOCK_ITEM (g_object);
        GtkWidget *hbox;
        GtkWidget *label;
        GtkWidget *icon;
        gchar* long_name;
        gchar* stock_id;

        if (GDL_DOCK_ITEM_HAS_GRIP (item)) {
            item->priv->grip_shown = TRUE;
            item->priv->grip = gdl_dock_item_grip_new (item);
            /* There is an automatic pointer grab when clicking in the grip
             * widget but it can be removed if the widget is hidden before
             * releasing the button */
            g_signal_connect (item->priv->grip, "grab-broken-event",
                              G_CALLBACK (on_grab_broken_event),
                              item);
            gtk_widget_set_parent (item->priv->grip, GTK_WIDGET (item));
            gtk_widget_show (item->priv->grip);
        }
        else {
            item->priv->grip_shown = FALSE;
        }

        g_object_get (g_object, "long-name", &long_name, "stock-id", &stock_id, NULL);

        hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
        label = gtk_label_new (long_name);
        icon = gtk_image_new ();
        if (stock_id)
            gtk_image_set_from_stock (GTK_IMAGE (icon), stock_id,
                                      GTK_ICON_SIZE_MENU);
        gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

        item->priv->notify_label =
            g_signal_connect (item, "notify::long-name", G_CALLBACK (on_long_name_changed),
                              label);
        item->priv->notify_stock_id =
            g_signal_connect (item, "notify::stock-id", G_CALLBACK (on_stock_id_changed),
                              icon);

        gtk_widget_show_all (hbox);

        gdl_dock_item_set_tablabel (item, hbox);
        item->priv->intern_tab_label = TRUE;

        g_free (long_name);
        g_free (stock_id);
    }

    return g_object;
}

static void
gdl_dock_item_set_property  (GObject      *g_object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
    GdlDockItem *item = GDL_DOCK_ITEM (g_object);

    switch (prop_id) {
        case PROP_ORIENTATION:
            gdl_dock_item_set_orientation (item, g_value_get_enum (value));
            break;
        case PROP_RESIZE:
            item->priv->resize = g_value_get_boolean (value);
            {
                GObject * parent = gtk_widget_get_parent (GTK_WIDGET (item));
                //if we docked update "resize" child_property of our parent
                if(parent)
                {
                        gboolean resize;
                        gtk_container_child_get(GTK_CONTAINER(parent),
                                GTK_WIDGET(item),"resize",&resize,NULL);
                        if(resize != item->priv->resize)
                        {
                                gtk_container_child_set(GTK_CONTAINER(parent),
                                        GTK_WIDGET(item),"resize",item->priv->resize,NULL);
                        }
                }
            }
            gtk_widget_queue_resize (GTK_WIDGET (item));
            break;
        case PROP_BEHAVIOR:
        {
            gdl_dock_item_set_behavior_flags (item, g_value_get_flags (value), TRUE);
            break;
        }
        case PROP_LOCKED:
        {
            GdlDockItemBehavior old_beh = item->priv->behavior;

            if (g_value_get_boolean (value))
                item->priv->behavior |= GDL_DOCK_ITEM_BEH_LOCKED;
            else
                item->priv->behavior &= ~GDL_DOCK_ITEM_BEH_LOCKED;

            if (old_beh ^ item->priv->behavior) {
                gdl_dock_item_showhide_grip (item);
                g_object_notify (g_object, "behavior");

                gdl_dock_object_layout_changed_notify (GDL_DOCK_OBJECT (item));
            }
            break;
        }
        case PROP_PREFERRED_WIDTH:
            item->priv->preferred_width = g_value_get_int (value);
            break;
        case PROP_PREFERRED_HEIGHT:
            item->priv->preferred_height = g_value_get_int (value);
            break;
        case PROP_ICONIFIED:
            if (g_value_get_boolean (value)) {
                if (!item->priv->iconified) gdl_dock_item_iconify_item (item);
            }
            else if (item->priv->iconified) {
                item->priv->iconified = FALSE;
                gtk_widget_show (GTK_WIDGET (item));
                gtk_widget_queue_resize (GTK_WIDGET (item));
            }
            break;
        case PROP_CLOSED:
            if (g_value_get_boolean (value)) {
                gtk_widget_hide (GTK_WIDGET (item));
            } else {
                if (!item->priv->iconified && !gdl_dock_item_is_placeholder (item))
                    gtk_widget_show (GTK_WIDGET (item));
	    }
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (g_object, prop_id, pspec);
            break;
    }
}

static void
gdl_dock_item_get_property  (GObject      *g_object,
                             guint         prop_id,
                             GValue       *value,
                             GParamSpec   *pspec)
{
    GdlDockItem *item = GDL_DOCK_ITEM (g_object);

    switch (prop_id) {
        case PROP_ORIENTATION:
            g_value_set_enum (value, item->priv->orientation);
            break;
        case PROP_RESIZE:
            g_value_set_boolean (value, item->priv->resize);
            break;
        case PROP_BEHAVIOR:
            g_value_set_flags (value, item->priv->behavior);
            break;
        case PROP_LOCKED:
            g_value_set_boolean (value, !GDL_DOCK_ITEM_NOT_LOCKED (item));
            break;
        case PROP_PREFERRED_WIDTH:
            g_value_set_int (value, item->priv->preferred_width);
            break;
        case PROP_PREFERRED_HEIGHT:
            g_value_set_int (value, item->priv->preferred_height);
            break;
        case PROP_ICONIFIED:
            g_value_set_boolean (value, gdl_dock_item_is_iconified (item));
            break;
        case PROP_CLOSED:
            g_value_set_boolean (value, gdl_dock_item_is_closed (item));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (g_object, prop_id, pspec);
            break;
    }
}

static void
gdl_dock_item_dispose (GObject *object)
{
    GdlDockItem *item = GDL_DOCK_ITEM (object);
    GdlDockItemPrivate *priv = item->priv;

    if (priv->tab_label) {
        gdl_dock_item_set_tablabel (item, NULL);
    }

    if (priv->menu) {
        gtk_menu_detach (GTK_MENU (priv->menu));
        priv->menu = NULL;
        priv->menu_item_hide = NULL;
    }

    if (priv->grip) {
        gtk_container_remove (GTK_CONTAINER (item), priv->grip);
        priv->grip = NULL;
    }

    G_OBJECT_CLASS (gdl_dock_item_parent_class)->dispose (object);
}

static void
gdl_dock_item_add (GtkContainer *container,
                   GtkWidget    *widget)
{
    GdlDockItem *item;

    g_return_if_fail (GDL_IS_DOCK_ITEM (container));

    item = GDL_DOCK_ITEM (container);
    if (GDL_IS_DOCK_ITEM (widget)) {
        g_warning (_("You can't add a dock object (%p of type %s) inside a %s. "
                     "Use a GdlDock or some other compound dock object."),
                   widget, G_OBJECT_TYPE_NAME (widget), G_OBJECT_TYPE_NAME (item));
        return;
    }

    if (item->priv->child != NULL) {
        g_warning (_("Attempting to add a widget with type %s to a %s, "
                     "but it can only contain one widget at a time; "
                     "it already contains a widget of type %s"),
                     G_OBJECT_TYPE_NAME (widget),
                     G_OBJECT_TYPE_NAME (item),
                     G_OBJECT_TYPE_NAME (item->priv->child));
        return;
    }

    gtk_widget_set_parent (widget, GTK_WIDGET (item));
    item->priv->child = widget;
}

static void
gdl_dock_item_remove (GtkContainer *container,
                      GtkWidget    *widget)
{
    GdlDockItem *item;
    gboolean     was_visible;

    g_return_if_fail (GDL_IS_DOCK_ITEM (container));

    item = GDL_DOCK_ITEM (container);
    if (item->priv && widget == item->priv->grip) {
        gboolean grip_was_visible = gtk_widget_get_visible (widget);
        gtk_widget_unparent (widget);
        item->priv->grip = NULL;
        if (grip_was_visible)
            gtk_widget_queue_resize (GTK_WIDGET (item));
        return;
    }

    gdl_dock_item_drag_end (item, TRUE);

    g_return_if_fail (item->priv->child == widget);

    was_visible = gtk_widget_get_visible (widget);

    gtk_widget_unparent (widget);
    item->priv->child = NULL;

    if (was_visible)
        gtk_widget_hide (GTK_WIDGET (container));
}

static void
gdl_dock_item_forall (GtkContainer *container,
                      gboolean      include_internals,
                      GtkCallback   callback,
                      gpointer      callback_data)
{
    GdlDockItem *item = (GdlDockItem *) container;

    g_return_if_fail (callback != NULL);

    if (include_internals && item->priv->grip)
        (* callback) (item->priv->grip, callback_data);

    if (item->priv->child)
        (* callback) (item->priv->child, callback_data);
}

static GType
gdl_dock_item_child_type (GtkContainer *container)
{
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (container), G_TYPE_NONE);

    if (!GDL_DOCK_ITEM (container)->priv->child)
        return GTK_TYPE_WIDGET;
    else
        return G_TYPE_NONE;
}

static void
gdl_dock_item_set_focus_child (GtkContainer *container,
                               GtkWidget    *child)
{
    g_return_if_fail (GDL_IS_DOCK_ITEM (container));

    if (GTK_CONTAINER_CLASS (gdl_dock_item_parent_class)->set_focus_child) {
        (* GTK_CONTAINER_CLASS (gdl_dock_item_parent_class)->set_focus_child) (container, child);
    }
}

static void
gdl_dock_item_get_preferred_width (GtkWidget *widget,
                                   gint      *minimum,
                                   gint      *natural)
{
    GdlDockItem    *item;
    gint child_min, child_nat;
    GtkStyleContext *context;
    GtkBorder padding;

    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    item = GDL_DOCK_ITEM (widget);

    /* If our child is not visible, we still request its size, since
       we won't have any useful hint for our size otherwise.  */
    if (item->priv->child)
        gtk_widget_get_preferred_width (item->priv->child, &child_min, &child_nat);
    else
        child_min = child_nat = 0;

    if (item->priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
        if (GDL_DOCK_ITEM_GRIP_SHOWN (item)) {
            gtk_widget_get_preferred_width (item->priv->grip, minimum, natural);
        } else
            *minimum = *natural = 0;

        if (item->priv->child) {
            *minimum += child_min;
            *natural += child_nat;
        }
    } else {
        if (item->priv->child) {
            *minimum = child_min;
            *natural = child_nat;
        } else
            *minimum = *natural = 0;
    }

    context = gtk_widget_get_style_context (widget);
    gtk_style_context_get_padding (context, gtk_style_context_get_state (context),
                                   &padding);

    *minimum += padding.left + padding.right;
    *natural += padding.left + padding.right;
}

static void
gdl_dock_item_get_preferred_height (GtkWidget *widget,
                                    gint      *minimum,
                                    gint      *natural)
{
    GdlDockItem    *item;
    gint child_min, child_nat;
    GtkStyleContext *context;
    GtkBorder padding;

    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    item = GDL_DOCK_ITEM (widget);

    /* If our child is not visible, we still request its size, since
       we won't have any useful hint for our size otherwise.  */
    if (item->priv->child)
        gtk_widget_get_preferred_height (item->priv->child, &child_min, &child_nat);
    else
        child_min = child_nat = 0;

    if (item->priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
        if (item->priv->child) {
            *minimum = child_min;
            *natural = child_nat;
        } else
            *minimum = *natural = 0;
    } else {
        if (GDL_DOCK_ITEM_GRIP_SHOWN (item)) {
            gtk_widget_get_preferred_height (item->priv->grip, minimum, natural);
        } else
            *minimum = *natural = 0;

        if (item->priv->child) {
            *minimum += child_min;
            *natural += child_nat;
        }
    }

    context = gtk_widget_get_style_context (widget);
    gtk_style_context_get_padding (context, gtk_style_context_get_state (context),
                                   &padding);

    *minimum += padding.top + padding.bottom;
    *natural += padding.top + padding.bottom;
}

static void
gdl_dock_item_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
    GdlDockItem *item;

    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));
    g_return_if_fail (allocation != NULL);

    item = GDL_DOCK_ITEM (widget);

    gtk_widget_set_allocation (widget, allocation);

    /* Once size is allocated, preferred size is no longer necessary */
    item->priv->preferred_height = -1;
    item->priv->preferred_width = -1;

    if (gtk_widget_get_realized (widget))
        gdk_window_move_resize (gtk_widget_get_window (widget),
                                allocation->x,
                                allocation->y,
                                allocation->width,
                                allocation->height);

    if (item->priv->child && gtk_widget_get_visible (item->priv->child)) {
        GtkAllocation  child_allocation;
        GtkStyleContext *context;
        GtkStateFlags state;
        GtkBorder padding;

        context = gtk_widget_get_style_context (widget);
        state = gtk_style_context_get_state (context);
        gtk_style_context_get_padding (context, state, &padding);

        child_allocation.x = padding.left;
        child_allocation.y = padding.top;
        child_allocation.width = allocation->width
            - padding.left - padding.right;
        child_allocation.height = allocation->height
            - padding.top - padding.bottom;

        if (GDL_DOCK_ITEM_GRIP_SHOWN (item)) {
            GtkAllocation grip_alloc = child_allocation;
            GtkRequisition grip_req;

            gtk_widget_get_preferred_size (item->priv->grip, &grip_req, NULL);

            if (item->priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
                child_allocation.x += grip_req.width;
                child_allocation.width -= grip_req.width;
                grip_alloc.width = grip_req.width;
            } else {
                child_allocation.y += grip_req.height;
                child_allocation.height -= grip_req.height;
                grip_alloc.height = grip_req.height;
            }
            if (item->priv->grip)
                gtk_widget_size_allocate (item->priv->grip, &grip_alloc);
        }
        /* Allocation can't be negative */
        if (child_allocation.width < 0)
            child_allocation.width = 0;
        if (child_allocation.height < 0)
            child_allocation.height = 0;
        gtk_widget_size_allocate (item->priv->child, &child_allocation);
    }
}

static void
gdl_dock_item_map (GtkWidget *widget)
{
    GdlDockItem *item;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    gtk_widget_set_mapped (widget, TRUE);

    item = GDL_DOCK_ITEM (widget);

    gdk_window_show (gtk_widget_get_window (widget));

    if (item->priv->child
        && gtk_widget_get_visible (item->priv->child)
        && !gtk_widget_get_mapped (item->priv->child))
        gtk_widget_map (item->priv->child);

    if (item->priv->grip
        && gtk_widget_get_visible (GTK_WIDGET (item->priv->grip))
        && !gtk_widget_get_mapped (GTK_WIDGET (item->priv->grip)))
        gtk_widget_map (item->priv->grip);
}

static void
gdl_dock_item_unmap (GtkWidget *widget)
{
    GdlDockItem *item;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    gtk_widget_set_mapped (widget, FALSE);

    item = GDL_DOCK_ITEM (widget);

    gdk_window_hide (gtk_widget_get_window (widget));

    if (item->priv->child)
	gtk_widget_unmap (item->priv->child);

    if (item->priv->grip)
        gtk_widget_unmap (item->priv->grip);
}

static void
gdl_dock_item_realize (GtkWidget *widget)
{
    GdlDockItem   *item;
    GtkAllocation  allocation;
    GdkWindow     *window;
    GdkWindowAttr  attributes;
    gint           attributes_mask;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    item = GDL_DOCK_ITEM (widget);

    gtk_widget_set_realized (widget, TRUE);

    /* widget window */
    gtk_widget_get_allocation (widget, &allocation);
    attributes.x = allocation.x;
    attributes.y = allocation.y;
    attributes.width = allocation.width;
    attributes.height = allocation.height;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual (widget);
    attributes.event_mask = (gtk_widget_get_events (widget) |
                             GDK_EXPOSURE_MASK |
                             GDK_BUTTON1_MOTION_MASK |
                             GDK_BUTTON_PRESS_MASK |
                             GDK_BUTTON_RELEASE_MASK);
    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;
    window = gdk_window_new (gtk_widget_get_parent_window (widget),
                             &attributes, attributes_mask);
    gtk_widget_set_window (widget, window);
    gdk_window_set_user_data (window, widget);

    gtk_style_context_set_background (gtk_widget_get_style_context (widget),
                                      window);

    if (item->priv->child)
        gtk_widget_set_parent_window (item->priv->child, window);

    if (item->priv->grip)
        gtk_widget_set_parent_window (item->priv->grip, window);
}

static void
gdl_dock_item_move_focus_child (GdlDockItem      *item,
                                GtkDirectionType  dir)
{
    g_return_if_fail (GDL_IS_DOCK_ITEM (item));
    gtk_widget_child_focus (GTK_WIDGET (item->priv->child), dir);
}

#define EVENT_IN_GRIP_EVENT_WINDOW(ev,gr) \
    ((gr) != NULL && gdl_dock_item_grip_has_event (GDL_DOCK_ITEM_GRIP (gr), (GdkEvent *)(ev)))

static gint
gdl_dock_item_button_changed (GtkWidget      *widget,
                              GdkEventButton *event)
{
    GdlDockItem *item;
    GtkAllocation allocation;
    gboolean     locked;
    gboolean     event_handled;
    gboolean     in_handle;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    item = GDL_DOCK_ITEM (widget);

    if (!EVENT_IN_GRIP_EVENT_WINDOW (event, item->priv->grip))
        return FALSE;

    locked = !GDL_DOCK_ITEM_NOT_LOCKED (item);

    event_handled = FALSE;

    gtk_widget_get_allocation (item->priv->grip, &allocation);

    /* Check if user clicked on the drag handle. */
    switch (item->priv->orientation) {
    case GTK_ORIENTATION_HORIZONTAL:
        in_handle = event->x < allocation.width;
        break;
    case GTK_ORIENTATION_VERTICAL:
        in_handle = event->y < allocation.height;
        break;
    default:
        in_handle = FALSE;
        break;
    }

    /* Left mousebutton click on dockitem. */
    if (!locked && event->button == 1 && event->type == GDK_BUTTON_PRESS) {

        if (!gdl_dock_item_or_child_has_focus (item))
            gtk_widget_grab_focus (GTK_WIDGET (item));

        /* Set in_drag flag, grab pointer and call begin drag operation. */
        if (in_handle) {
            item->priv->start_x = event->x;
            item->priv->start_y = event->y;

            item->priv->in_predrag = TRUE;

            gdl_dock_item_grip_set_cursor (GDL_DOCK_ITEM_GRIP (item->priv->grip), TRUE);

            event_handled = TRUE;
        };

    } else if (!locked &&event->type == GDK_BUTTON_RELEASE && event->button == 1) {
        /* User dropped widget somewhere */
        event_handled = gdl_dock_item_drag_end (item, FALSE);

    } else if (event->button == 3 && event->type == GDK_BUTTON_PRESS && in_handle) {
        gdl_dock_item_popup_menu (item, event->button, event->time);
        event_handled = TRUE;
    }

    return event_handled;
}

static gint
gdl_dock_item_motion (GtkWidget      *widget,
                      GdkEventMotion *event)
{
    GdlDockItem *item;
    gint         new_x, new_y;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    item = GDL_DOCK_ITEM (widget);

    /* motion drag events are coming from the grip window because there is an
     * automatic pointer grab when clicking on the grip widget to start drag. */
    if (!EVENT_IN_GRIP_EVENT_WINDOW (event, item->priv->grip))
        return FALSE;

    if (item->priv->in_predrag) {
        if (gtk_drag_check_threshold (widget,
                                      item->priv->start_x,
                                      item->priv->start_y,
                                      event->x,
                                      event->y)) {
            item->priv->in_predrag = FALSE;
            item->priv->dragoff_x = item->priv->start_x;
            item->priv->dragoff_y = item->priv->start_y;

            gdl_dock_item_drag_start (item);
        }
    }

    if (!item->priv->in_drag)
        return FALSE;

    new_x = event->x_root;
    new_y = event->y_root;

    g_signal_emit (item, gdl_dock_item_signals [DOCK_DRAG_MOTION],
                   0, event->device, new_x, new_y);

    return TRUE;
}

static gboolean
gdl_dock_item_key_press (GtkWidget   *widget,
                         GdkEventKey *event)
{
    /* Cancel drag */
    if (gdl_dock_item_drag_end (GDL_DOCK_ITEM (widget), TRUE))
        return TRUE;
    else
        return GTK_WIDGET_CLASS (gdl_dock_item_parent_class)->key_press_event (widget, event);
}

static gboolean
gdl_dock_item_dock_request (GdlDockObject  *object,
                            gint            x,
                            gint            y,
                            GdlDockRequest *request)
{
    GtkAllocation  alloc;
    gint           rel_x, rel_y;

    /* we get (x,y) in our allocation coordinates system */

    /* Get item's allocation. */
    gtk_widget_get_allocation (GTK_WIDGET (object), &alloc);

    /* Get coordinates relative to our window. */
    rel_x = x - alloc.x;
    rel_y = y - alloc.y;

    /* Location is inside. */
    if (rel_x > 0 && rel_x < alloc.width &&
        rel_y > 0 && rel_y < alloc.height) {
        float rx, ry;
        GtkRequisition my, other;
        gint divider = -1;

        /* this are for calculating the extra docking parameter */
        gdl_dock_item_preferred_size (GDL_DOCK_ITEM (request->applicant), &other);
        gdl_dock_item_preferred_size (GDL_DOCK_ITEM (object), &my);

        /* Calculate location in terms of the available space (0-100%). */
        rx = (float) rel_x / alloc.width;
        ry = (float) rel_y / alloc.height;

        /* Determine dock location. */
        if (rx < SPLIT_RATIO) {
            request->position = GDL_DOCK_LEFT;
            divider = other.width;
        }
        else if (rx > (1 - SPLIT_RATIO)) {
            request->position = GDL_DOCK_RIGHT;
            rx = 1 - rx;
            divider = MAX (0, my.width - other.width);
        }
        else if (ry < SPLIT_RATIO && ry < rx) {
            request->position = GDL_DOCK_TOP;
            divider = other.height;
        }
        else if (ry > (1 - SPLIT_RATIO) && (1 - ry) < rx) {
            request->position = GDL_DOCK_BOTTOM;
            divider = MAX (0, my.height - other.height);
        }
        else
            request->position = GDL_DOCK_CENTER;

        /* Reset rectangle coordinates to entire item. */
        request->rect.x = 0;
        request->rect.y = 0;
        request->rect.width = alloc.width;
        request->rect.height = alloc.height;

        GdlDockItemBehavior behavior = GDL_DOCK_ITEM(object)->priv->behavior;

        /* Calculate docking indicator rectangle size for new locations. Only
           do this when we're not over the item's current location. */
        if (request->applicant != object) {
            switch (request->position) {
                case GDL_DOCK_TOP:
                    if (behavior & GDL_DOCK_ITEM_BEH_CANT_DOCK_TOP)
                        return FALSE;
                    request->rect.height *= SPLIT_RATIO;
                    break;
                case GDL_DOCK_BOTTOM:
                    if (behavior & GDL_DOCK_ITEM_BEH_CANT_DOCK_BOTTOM)
                        return FALSE;
                    request->rect.y += request->rect.height * (1 - SPLIT_RATIO);
                    request->rect.height *= SPLIT_RATIO;
                    break;
                case GDL_DOCK_LEFT:
                    if (behavior & GDL_DOCK_ITEM_BEH_CANT_DOCK_LEFT)
                        return FALSE;
                    request->rect.width *= SPLIT_RATIO;
                    break;
                case GDL_DOCK_RIGHT:
                    if (behavior & GDL_DOCK_ITEM_BEH_CANT_DOCK_RIGHT)
                        return FALSE;
                    request->rect.x += request->rect.width * (1 - SPLIT_RATIO);
                    request->rect.width *= SPLIT_RATIO;
                    break;
                case GDL_DOCK_CENTER:
                    if (behavior & GDL_DOCK_ITEM_BEH_CANT_DOCK_CENTER)
                        return FALSE;
                    request->rect.x = request->rect.width * SPLIT_RATIO/2;
                    request->rect.y = request->rect.height * SPLIT_RATIO/2;
                    request->rect.width = (request->rect.width *
                                           (1 - SPLIT_RATIO/2)) - request->rect.x;
                    request->rect.height = (request->rect.height *
                                            (1 - SPLIT_RATIO/2)) - request->rect.y;
                    break;
                default:
                    break;
            }
        }

        /* adjust returned coordinates so they are have the same
           origin as our window */
        request->rect.x += alloc.x;
        request->rect.y += alloc.y;

        /* Set possible target location and return TRUE. */
        request->target = object;

        /* fill-in other dock information */
        if (request->position != GDL_DOCK_CENTER && divider >= 0) {
            if (G_IS_VALUE (&request->extra))
                g_value_unset (&request->extra);
            g_value_init (&request->extra, G_TYPE_UINT);
            g_value_set_uint (&request->extra, (guint) divider);
        }

        return TRUE;
    }
    else /* No docking possible at this location. */
        return FALSE;
}

static void
gdl_dock_item_dock (GdlDockObject    *object,
                    GdlDockObject    *requestor,
                    GdlDockPlacement  position,
                    GValue           *other_data)
{
    GdlDockObject *new_parent = NULL;
    GdlDockObject *parent, *requestor_parent;
    GtkAllocation  allocation;
    gboolean       add_ourselves_first = FALSE;

    guint	   available_space=0;
    gint	   pref_size=-1;
    guint	   splitpos=0;
    GtkRequisition req, object_req, parent_req;

    parent = gdl_dock_object_get_parent_object (object);
    gdl_dock_item_preferred_size (GDL_DOCK_ITEM (requestor), &req);
    gdl_dock_item_preferred_size (GDL_DOCK_ITEM (object), &object_req);
    if (GDL_IS_DOCK_ITEM (parent))
        gdl_dock_item_preferred_size (GDL_DOCK_ITEM (parent), &parent_req);
    else
    {
        gtk_widget_get_allocation (GTK_WIDGET (parent), &allocation);
        parent_req.height = allocation.height;
        parent_req.width = allocation.width;
    }

    /* If preferred size is not set on the requestor (perhaps a new item),
     * then estimate and set it. The default value (either 0 or 1 pixels) is
     * not any good.
     */
    switch (position) {
        case GDL_DOCK_TOP:
        case GDL_DOCK_BOTTOM:
            if (req.width < 2)
            {
                req.width = object_req.width;
                g_object_set (requestor, "preferred-width", req.width, NULL);
            }
            if (req.height < 2)
            {
                req.height = NEW_DOCK_ITEM_RATIO * object_req.height;
                g_object_set (requestor, "preferred-height", req.height, NULL);
            }
            if (req.width > 1)
                g_object_set (object, "preferred-width", req.width, NULL);
            if (req.height > 1)
                g_object_set (object, "preferred-height",
                              object_req.height - req.height, NULL);
            break;
        case GDL_DOCK_LEFT:
        case GDL_DOCK_RIGHT:
            if (req.height < 2)
            {
                req.height = object_req.height;
                g_object_set (requestor, "preferred-height", req.height, NULL);
            }
            if (req.width < 2)
            {
                req.width = NEW_DOCK_ITEM_RATIO * object_req.width;
                g_object_set (requestor, "preferred-width", req.width, NULL);
            }
            if (req.height > 1)
                g_object_set (object, "preferred-height", req.height, NULL);
            if (req.width > 1)
                g_object_set (object, "preferred-width",
                          object_req.width - req.width, NULL);
            break;
        case GDL_DOCK_CENTER:
            if (req.height < 2)
            {
                req.height = object_req.height;
                g_object_set (requestor, "preferred-height", req.height, NULL);
            }
            if (req.width < 2)
            {
                req.width = object_req.width;
                g_object_set (requestor, "preferred-width", req.width, NULL);
            }
            if (req.height > 1)
                g_object_set (object, "preferred-height", req.height, NULL);
            if (req.width > 1)
                g_object_set (object, "preferred-width", req.width, NULL);
            break;
        default:
        {
            GEnumClass *enum_class = G_ENUM_CLASS (g_type_class_ref (GDL_TYPE_DOCK_PLACEMENT));
            GEnumValue *enum_value = g_enum_get_value (enum_class, position);
            const gchar *name = enum_value ? enum_value->value_name : NULL;

            g_warning (_("Unsupported docking strategy %s in dock object of type %s"),
                       name,  G_OBJECT_TYPE_NAME (object));
            g_type_class_unref (enum_class);
            return;
        }
    }
    switch (position) {
        case GDL_DOCK_TOP:
        case GDL_DOCK_BOTTOM:
            /* get a paned style dock object */
            new_parent = g_object_new (gdl_dock_object_type_from_nick ("paned"),
                                       "orientation", GTK_ORIENTATION_VERTICAL,
                                       "preferred-width", object_req.width,
                                       "preferred-height", object_req.height,
                                       NULL);
            add_ourselves_first = (position == GDL_DOCK_BOTTOM);
            if (parent)
                available_space = parent_req.height;
            pref_size = req.height;
            break;
        case GDL_DOCK_LEFT:
        case GDL_DOCK_RIGHT:
            new_parent = g_object_new (gdl_dock_object_type_from_nick ("paned"),
                                       "orientation", GTK_ORIENTATION_HORIZONTAL,
                                       "preferred-width", object_req.width,
                                       "preferred-height", object_req.height,
                                       NULL);
            add_ourselves_first = (position == GDL_DOCK_RIGHT);
            if(parent)
                available_space = parent_req.width;
            pref_size = req.width;
            break;
        case GDL_DOCK_CENTER:
            /* If the parent is already a DockNotebook, we don't need
             to create a new one. */
            if (!GDL_IS_DOCK_NOTEBOOK (parent))
            {
                new_parent = g_object_new (gdl_dock_object_type_from_nick ("notebook"),
                                           "preferred-width", object_req.width,
                                           "preferred-height", object_req.height,
                                           NULL);
                add_ourselves_first = TRUE;
            }
            break;
        default:
        {
            GEnumClass *enum_class = G_ENUM_CLASS (g_type_class_ref (GDL_TYPE_DOCK_PLACEMENT));
            GEnumValue *enum_value = g_enum_get_value (enum_class, position);
            const gchar *name = enum_value ? enum_value->value_name : NULL;

            g_warning (_("Unsupported docking strategy %s in dock object of type %s"),
                       name,  G_OBJECT_TYPE_NAME (object));
            g_type_class_unref (enum_class);
            return;
        }
    }

    /* freeze the parent so it doesn't reduce automatically */
    if (parent)
        gdl_dock_object_freeze (parent);


    if (new_parent)
    {
        /* ref ourselves since we could be destroyed when detached */
        g_object_ref (object);
        gdl_dock_object_detach (object, FALSE);

        /* freeze the new parent, so reduce won't get called before it's
           actually added to our parent */
        gdl_dock_object_freeze (new_parent);

        /* bind the new parent to our master, so the following adds work */
        gdl_dock_object_bind (new_parent, gdl_dock_object_get_master (GDL_DOCK_OBJECT (object)));

        /* add the objects */
        if (add_ourselves_first) {
            gtk_container_add (GTK_CONTAINER (new_parent), GTK_WIDGET (object));
            gtk_container_add (GTK_CONTAINER (new_parent), GTK_WIDGET (requestor));
            splitpos = available_space - pref_size;
        } else {
            gtk_container_add (GTK_CONTAINER (new_parent), GTK_WIDGET (requestor));
            gtk_container_add (GTK_CONTAINER (new_parent), GTK_WIDGET (object));
            splitpos = pref_size;
        }

        /* add the new parent to the parent */
        if (parent)
            gtk_container_add (GTK_CONTAINER (parent), GTK_WIDGET (new_parent));

        /* show automatic object */
        gtk_widget_show (GTK_WIDGET (new_parent));
        gdl_dock_object_thaw (new_parent);

        /* use extra docking parameter */
        if (position != GDL_DOCK_CENTER && other_data &&
            G_VALUE_HOLDS (other_data, G_TYPE_UINT)) {

            g_object_set (G_OBJECT (new_parent),
                          "position", g_value_get_uint (other_data),
                          NULL);
        } else if (splitpos > 0 && splitpos < available_space) {
            g_object_set (G_OBJECT (new_parent), "position", splitpos, NULL);
        }

        g_object_unref (object);
    }
    else
    {
        /* If the parent is already a DockNotebook, we don't need
         to create a new one. */
        gtk_container_add (GTK_CONTAINER (parent), GTK_WIDGET (requestor));
    }

    requestor_parent = gdl_dock_object_get_parent_object (requestor);
    if (GDL_IS_DOCK_NOTEBOOK (requestor_parent) && gtk_widget_get_visible (GTK_WIDGET (requestor)))
    {
        /* Activate the page we just added */
        GdlDockItem* notebook = GDL_DOCK_ITEM (gdl_dock_object_get_parent_object (requestor));
        gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook->priv->child),
                               gtk_notebook_page_num (GTK_NOTEBOOK (notebook->priv->child), GTK_WIDGET (requestor)));
    }

    if (parent)
        gdl_dock_object_thaw (parent);
}

static void
gdl_dock_item_present (GdlDockObject     *object,
                       GdlDockObject     *child)
{
    GdlDockItem *item = GDL_DOCK_ITEM (object);

    gdl_dock_item_show_item (item);
    GDL_DOCK_OBJECT_CLASS (gdl_dock_item_parent_class)->present (object, child);
}

static void
gdl_dock_item_detach_menu (GtkWidget *widget,
                           GtkMenu   *menu)
{
    GdlDockItem *item;

    item = GDL_DOCK_ITEM (widget);
    item->priv->menu = NULL;
}

static void
gdl_dock_item_popup_menu (GdlDockItem  *item,
                          guint         button,
                          guint32       time)
{
    GtkWidget *mitem;

    if (!item->priv->menu) {
        /* Create popup menu and attach it to the dock item */
        item->priv->menu = gtk_menu_new ();
        gtk_menu_attach_to_widget (GTK_MENU (item->priv->menu),
                                   GTK_WIDGET (item),
                                   gdl_dock_item_detach_menu);

        if (item->priv->behavior & GDL_DOCK_ITEM_BEH_LOCKED) {
            /* UnLock menuitem */
            mitem = gtk_menu_item_new_with_label (_("UnLock"));
            gtk_menu_shell_append (GTK_MENU_SHELL (item->priv->menu),
                                   mitem);
            g_signal_connect (mitem, "activate",
                              G_CALLBACK (gdl_dock_item_unlock_cb), item);
        } else {
            /* Hide menuitem. */
            mitem = gtk_menu_item_new_with_label (_("Hide"));
            gtk_menu_shell_append (GTK_MENU_SHELL (item->priv->menu), mitem);
            g_signal_connect (mitem, "activate",
                              G_CALLBACK (gdl_dock_item_hide_cb), item);
            item->priv->menu_item_hide = mitem;
            /* Lock menuitem */
            mitem = gtk_menu_item_new_with_label (_("Lock"));
            gtk_menu_shell_append (GTK_MENU_SHELL (item->priv->menu), mitem);
            g_signal_connect (mitem, "activate",
                              G_CALLBACK (gdl_dock_item_lock_cb), item);
        }
    }

    /* Show popup menu. */
    gtk_widget_show_all (item->priv->menu);
    if (item->priv->menu_item_hide != NULL)
        gtk_widget_set_visible(item->priv->menu_item_hide, !GDL_DOCK_ITEM_CANT_CLOSE(item));
    gtk_menu_popup (GTK_MENU (item->priv->menu), NULL, NULL, NULL, NULL,
                    button, time);
}

static void
gdl_dock_item_drag_start (GdlDockItem *item)
{
    if (!gtk_widget_get_realized (GTK_WIDGET (item)))
        gtk_widget_realize (GTK_WIDGET (item));

    item->priv->in_drag = TRUE;

    /* grab the keyboard & pointer. The pointer has already been grabbed by the grip
     * window when it has received a press button event. See gdk_pointer_grab. */
    gtk_grab_add (GTK_WIDGET (item));

    g_signal_emit (item, gdl_dock_item_signals [DOCK_DRAG_BEGIN], 0);
}

/* Terminate a drag operation, return TRUE if a drag or pre-drag was running */
static gboolean
gdl_dock_item_drag_end (GdlDockItem *item,
                        gboolean     cancel)
{
    if (item->priv->in_drag) {
        /* Release pointer & keyboard. */
        gtk_grab_remove (GTK_WIDGET (item));
        g_signal_emit (item, gdl_dock_item_signals [DOCK_DRAG_END], 0, cancel);
        gtk_widget_grab_focus (GTK_WIDGET (item));

        item->priv->in_drag = FALSE;
    }
    else if (item->priv->in_predrag) {
        item->priv->in_predrag = FALSE;
    }
    else {
        /* No drag not pre-drag has been started */
        return FALSE;
    }

    /* Restore old cursor */
    gdl_dock_item_grip_set_cursor (GDL_DOCK_ITEM_GRIP (item->priv->grip), FALSE);

    return TRUE;
}

static void
gdl_dock_item_tab_button (GtkWidget      *widget,
                          GdkEventButton *event,
                          gpointer        data)
{
    GdlDockItem *item;
    GtkAllocation allocation;

    item = GDL_DOCK_ITEM (data);

    if (!GDL_DOCK_ITEM_NOT_LOCKED (item))
        return;

    switch (event->button) {
    case 1:
        /* set dragoff_{x,y} as we the user clicked on the middle of the
           drag handle */
        switch (item->priv->orientation) {
        case GTK_ORIENTATION_HORIZONTAL:
            gtk_widget_get_allocation (GTK_WIDGET (data), &allocation);
            /*item->priv->dragoff_x = item->priv->grip_size / 2;*/
            item->priv->dragoff_y = allocation.height / 2;
            break;
        case GTK_ORIENTATION_VERTICAL:
            /*item->priv->dragoff_x = GTK_WIDGET (data)->allocation.width / 2;*/
            item->priv->dragoff_y = item->priv->grip_size / 2;
            break;
        };
        gdl_dock_item_drag_start (item);
        break;

    case 3:
        gdl_dock_item_popup_menu (item, event->button, event->time);
        break;

    default:
        break;
    };
}

static void
gdl_dock_item_hide_cb (GtkWidget   *widget,
                       GdlDockItem *item)
{
    g_return_if_fail (item != NULL);

    gdl_dock_item_hide_item (item);
}

static void
gdl_dock_item_lock_cb (GtkWidget   *widget,
                       GdlDockItem *item)
{
    g_return_if_fail (item != NULL);

    gdl_dock_item_lock (item);
}

static void
gdl_dock_item_unlock_cb (GtkWidget   *widget,
                       GdlDockItem *item)
{
    g_return_if_fail (item != NULL);

    gdl_dock_item_unlock (item);
}

static void
gdl_dock_item_showhide_grip (GdlDockItem *item)
{
    gdl_dock_item_detach_menu (GTK_WIDGET (item), NULL);

    if (item->priv->grip && GDL_DOCK_ITEM_NOT_LOCKED(item) &&
        GDL_DOCK_ITEM_HAS_GRIP(item)) {

        if (item->priv->grip_shown) {
            gdl_dock_item_grip_show_handle (GDL_DOCK_ITEM_GRIP (item->priv->grip));
        } else {
            gdl_dock_item_grip_hide_handle (GDL_DOCK_ITEM_GRIP (item->priv->grip));
        }
    }
}

static void
gdl_dock_item_real_set_orientation (GdlDockItem    *item,
                                    GtkOrientation  orientation)
{
    item->priv->orientation = orientation;

    if (gtk_widget_is_drawable (GTK_WIDGET (item)))
        gtk_widget_queue_draw (GTK_WIDGET (item));
    gtk_widget_queue_resize (GTK_WIDGET (item));
}


/* ----- Public interface ----- */

/**
 * gdl_dock_item_new:
 * @name: Unique name for identifying the dock object.
 * @long_name: Human readable name for the dock object.
 * @behavior: General behavior for the dock item (i.e. whether it can
 *            float, if it's locked, etc.), as specified by
 *            #GdlDockItemBehavior flags.
 *
 * Creates a new dock item widget.
 * Returns: The newly created dock item grip widget.
 **/
GtkWidget *
gdl_dock_item_new (const gchar         *name,
                   const gchar         *long_name,
                   GdlDockItemBehavior  behavior)
{
    GdlDockItem *item;

    item = GDL_DOCK_ITEM (g_object_new (GDL_TYPE_DOCK_ITEM,
                                        "name", name,
                                        "long-name", long_name,
                                        "behavior", behavior,
                                        NULL));
    gdl_dock_object_set_manual (GDL_DOCK_OBJECT (item));
    gtk_widget_show (GTK_WIDGET (item));

    return GTK_WIDGET (item);
}

/**
 * gdl_dock_item_new_with_stock:
 * @name: Unique name for identifying the dock object.
 * @long_name: Human readable name for the dock object.
 * @stock_id: Stock icon for the dock object.
 * @behavior: General behavior for the dock item (i.e. whether it can
 *            float, if it's locked, etc.), as specified by
 *            #GdlDockItemBehavior flags.
 *
 * Creates a new dock item grip widget with a given stock id.
 * Returns: The newly created dock item grip widget.
 **/
GtkWidget *
gdl_dock_item_new_with_stock (const gchar         *name,
                              const gchar         *long_name,
                              const gchar         *stock_id,
                              GdlDockItemBehavior  behavior)
{
    GdlDockItem *item;

    item = GDL_DOCK_ITEM (g_object_new (GDL_TYPE_DOCK_ITEM,
                                        "name", name,
                                        "long-name", long_name,
                                        "stock-id", stock_id,
                                        "behavior", behavior,
                                        NULL));
   gdl_dock_object_set_manual (GDL_DOCK_OBJECT (item));

    return GTK_WIDGET (item);
}

/**
 * gdl_dock_item_new_with_pixbuf_icon:
 * @name: Unique name for identifying the dock object.
 * @long_name: Human readable name for the dock object.
 * @pixbuf_icon: Pixbuf icon for the dock object.
 * @behavior: General behavior for the dock item (i.e. whether it can
 *            float, if it's locked, etc.), as specified by
 *            #GdlDockItemBehavior flags.
 *
 * Creates a new dock item grip widget with a given pixbuf icon.
 * Returns: The newly created dock item grip widget.
 *
 * Since: 3.3.2
 **/
GtkWidget *
gdl_dock_item_new_with_pixbuf_icon (const gchar         *name,
                                    const gchar         *long_name,
                                    const GdkPixbuf     *pixbuf_icon,
                                    GdlDockItemBehavior  behavior)
{
    GdlDockItem *item;

    item = GDL_DOCK_ITEM (g_object_new (GDL_TYPE_DOCK_ITEM,
                                        "name", name,
                                        "long-name", long_name,
                                        "pixbuf-icon", pixbuf_icon,
                                        "behavior", behavior,
                                        NULL));

    gdl_dock_object_set_manual (GDL_DOCK_OBJECT (item));
    gdl_dock_item_set_tablabel (item, gtk_label_new (long_name));

    return GTK_WIDGET (item);
}

/* convenient function (and to preserve source compat) */
/**
 * gdl_dock_item_dock_to:
 * @item: The dock item that will be relocated to the dock position.
 * @target: (allow-none): The dock item that will be used as the point of reference.
 * @position: The position to dock #item, relative to #target.
 * @docking_param: This value is unused, and will be ignored.
 *
 * Relocates a dock item to a new location relative to another dock item.
 **/
void
gdl_dock_item_dock_to (GdlDockItem      *item,
                       GdlDockItem      *target,
                       GdlDockPlacement  position,
                       gint              docking_param)
{
    g_return_if_fail (item != NULL);
    g_return_if_fail (item != target);
    g_return_if_fail (target != NULL || position == GDL_DOCK_FLOATING);
    g_return_if_fail ((item->priv->behavior & GDL_DOCK_ITEM_BEH_NEVER_FLOATING) == 0 || position != GDL_DOCK_FLOATING);

    if (position == GDL_DOCK_FLOATING || !target) {
        GdlDockObject *controller;

        if (!gdl_dock_object_is_bound (GDL_DOCK_OBJECT (item))) {
            g_warning (_("Attempt to bind an unbound item %p"), item);
            return;
        }

        controller = gdl_dock_object_get_controller (GDL_DOCK_OBJECT (item));

        /* FIXME: save previous docking position for later
           re-docking... does this make sense now? */

        /* Create new floating dock for widget. */
        item->priv->dragoff_x = item->priv->dragoff_y = 0;
        gdl_dock_add_floating_item (GDL_DOCK (controller),
                                    item, 0, 0, -1, -1);

    } else
        gdl_dock_object_dock (GDL_DOCK_OBJECT (target),
                              GDL_DOCK_OBJECT (item),
                              position, NULL);
}

/**
 * gdl_dock_item_set_orientation:
 * @item: The dock item which will get it's orientation set.
 * @orientation: The orientation to set the item to. If the orientation
 * is set to #GTK_ORIENTATION_VERTICAL, the grip widget will be shown
 * along the top of the edge of item (if it is not hidden). If the
 * orientation is set to #GTK_ORIENTATION_HORIZONTAL, the grip widget
 * will be shown down the left edge of the item (even if the widget
 * text direction is set to RTL).
 *
 * This function sets the layout of the dock item.
 **/
void
gdl_dock_item_set_orientation (GdlDockItem    *item,
                               GtkOrientation  orientation)
{
    GParamSpec *pspec;

    g_return_if_fail (item != NULL);

    if (item->priv->orientation != orientation) {
        /* push the property down the hierarchy if our child supports it */
        if (item->priv->child != NULL) {
            pspec = g_object_class_find_property (
                G_OBJECT_GET_CLASS (item->priv->child), "orientation");
            if (pspec && pspec->value_type == GTK_TYPE_ORIENTATION)
                g_object_set (G_OBJECT (item->priv->child),
                              "orientation", orientation,
                              NULL);
        };
        if (GDL_DOCK_ITEM_GET_CLASS (item)->set_orientation)
            GDL_DOCK_ITEM_GET_CLASS (item)->set_orientation (item, orientation);
        g_object_notify (G_OBJECT (item), "orientation");
    }
}

/**
 * gdl_dock_item_get_orientation:
 * @item: a #GdlDockItem
 *
 * Retrieves the orientation of the object.
 *
 * Return value: the orientation of the object.
 *
 * Since: 3.6
 */
GtkOrientation
gdl_dock_item_get_orientation (GdlDockItem *item)
{
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (item), GTK_ORIENTATION_HORIZONTAL);

    return item->priv->orientation;
}

/**
 * gdl_dock_item_set_behavior_flags:
 * @item: The dock item which will get it's behavior set.
 * @behavior: Behavior flags to turn on
 * @clear: Whether to clear state before turning on @flags
 *
 * This function sets the behavior of the dock item.
 *
 * Since: 3.6
 */
void
gdl_dock_item_set_behavior_flags (GdlDockItem *item,
                                  GdlDockItemBehavior behavior,
                                  gboolean clear)
{
    GdlDockItemBehavior old_beh = item->priv->behavior;
    g_return_if_fail (GDL_IS_DOCK_ITEM (item));

    if (clear)
        item->priv->behavior = behavior;
    else
        item->priv->behavior |= behavior;

    if ((old_beh ^ behavior) & GDL_DOCK_ITEM_BEH_LOCKED) {
        gdl_dock_object_layout_changed_notify (GDL_DOCK_OBJECT (item));
        g_object_notify (G_OBJECT (item), "locked");
        gdl_dock_item_showhide_grip (item);
    }
}

/**
 * gdl_dock_item_unset_behavior_flags:
 * @item: The dock item which will get it's behavior set.
 * @behavior: Behavior flags to turn off
 *
 * This function sets the behavior of the dock item.
 *
 * Since: 3.6
 */
void
gdl_dock_item_unset_behavior_flags (GdlDockItem *item,
                                    GdlDockItemBehavior behavior)
{
    GdlDockItemBehavior old_beh = item->priv->behavior;
    g_return_if_fail (GDL_IS_DOCK_ITEM (item));

    item->priv->behavior &= ~behavior;

    if ((old_beh ^ behavior) & GDL_DOCK_ITEM_BEH_LOCKED) {
        gdl_dock_object_layout_changed_notify (GDL_DOCK_OBJECT (item));
        g_object_notify (G_OBJECT (item), "locked");
        gdl_dock_item_showhide_grip (item);
    }
}

/**
 * gdl_dock_item_get_behavior_flags:
 * @item: a #GdlDockItem
 *
 * Retrieves the behavior of the item.
 *
 * Return value: the behavior of the item.
 *
 * Since: 3.6
 */
GdlDockItemBehavior
gdl_dock_item_get_behavior_flags (GdlDockItem *item)
{
    GdlDockItemBehavior behavior;
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (item), GDL_DOCK_ITEM_BEH_NORMAL);

    behavior = item->priv->behavior;
    if (!(behavior & GDL_DOCK_ITEM_BEH_NO_GRIP) && !(GDL_DOCK_ITEM_GET_CLASS (item)->priv->has_grip))
        behavior |= GDL_DOCK_ITEM_BEH_NO_GRIP;
    if (behavior & GDL_DOCK_ITEM_BEH_LOCKED)
        behavior |= GDL_DOCK_ITEM_BEH_CANT_ICONIFY |
                    GDL_DOCK_ITEM_BEH_CANT_ICONIFY |
                    GDL_DOCK_ITEM_BEH_CANT_DOCK_TOP |
                    GDL_DOCK_ITEM_BEH_CANT_DOCK_BOTTOM |
                    GDL_DOCK_ITEM_BEH_CANT_DOCK_LEFT |
                    GDL_DOCK_ITEM_BEH_CANT_DOCK_RIGHT |
                    GDL_DOCK_ITEM_BEH_CANT_DOCK_CENTER;

    return behavior;
}

/**
 * gdl_dock_item_get_tablabel:
 * @item: The dock item from which to get the tab label widget.
 *
 * Gets the current tab label widget. Note that this label widget is
 * only visible when the "switcher-style" property of the #GdlDockMaster
 * is set to #GDL_SWITCHER_STYLE_TABS
 *
 * Returns: (transfer none): Returns the tab label widget.
 **/
GtkWidget *
gdl_dock_item_get_tablabel (GdlDockItem *item)
{
    g_return_val_if_fail (item != NULL, NULL);
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (item), NULL);

    return item->priv->tab_label;
}

/**
 * gdl_dock_item_set_tablabel:
 * @item: The dock item which will get it's tab label widget set.
 * @tablabel: The widget that will become the tab label.
 *
 * Replaces the current tab label widget with another widget. Note that
 * this label widget is only visible when the "switcher-style" property
 * of the #GdlDockMaster is set to #GDL_SWITCHER_STYLE_TABS
 **/
void
gdl_dock_item_set_tablabel (GdlDockItem *item,
                            GtkWidget   *tablabel)
{
    g_return_if_fail (item != NULL);

    if (item->priv->intern_tab_label)
    {
        item->priv->intern_tab_label = FALSE;
        g_signal_handler_disconnect (item, item->priv->notify_label);
        g_signal_handler_disconnect (item, item->priv->notify_stock_id);
    }

    if (item->priv->tab_label) {
        /* disconnect and unref the previous tablabel */
        g_object_unref (item->priv->tab_label);
        item->priv->tab_label = NULL;
    }

    if (tablabel) {
        g_object_ref_sink (G_OBJECT (tablabel));
        item->priv->tab_label = tablabel;
    }
}

/**
 * gdl_dock_item_get_grip:
 * @item: The dock item from which to to get the grip of.
 *
 * This function returns the dock item's grip label widget.
 *
 * Returns: (allow-none) (transfer none): Returns the current label widget.
 **/
GtkWidget *
gdl_dock_item_get_grip(GdlDockItem *item)
{
    g_return_val_if_fail (item != NULL, NULL);
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (item), NULL);

    return item->priv->grip;
}

/**
 * gdl_dock_item_hide_grip:
 * @item: The dock item to hide the grip of.
 *
 * This function hides the dock item's grip widget.
 **/
void
gdl_dock_item_hide_grip (GdlDockItem *item)
{
    g_return_if_fail (item != NULL);
    if (item->priv->grip_shown) {
        item->priv->grip_shown = FALSE;
        gdl_dock_item_showhide_grip (item);
    };
}

/**
 * gdl_dock_item_show_grip:
 * @item: The dock item to show the grip of.
 *
 * This function shows the dock item's grip widget.
 **/
void
gdl_dock_item_show_grip (GdlDockItem *item)
{
    g_return_if_fail (item != NULL);
    if (!item->priv->grip_shown) {
        item->priv->grip_shown = TRUE;
        gdl_dock_item_showhide_grip (item);
    };
}

/**
 * gdl_dock_item_notify_selected:
 * @item: the dock item to emit a selected signal on.
 *
 * This function emits the selected signal. It is to be used by #GdlSwitcher
 * to let clients know that this item has been switched to.
 **/
void
gdl_dock_item_notify_selected (GdlDockItem *item)
{
    g_signal_emit (item, gdl_dock_item_signals [SELECTED], 0);
}

/**
 * gdl_dock_item_notify_deselected:
 * @item: the dock item to emit a deselected signal on.
 *
 * This function emits the deselected signal. It is used by #GdlSwitcher
 * to let clients know that this item has been deselected.
 **/
void
gdl_dock_item_notify_deselected (GdlDockItem *item)
{
    g_signal_emit (item, gdl_dock_item_signals [DESELECTED], 0);
}

/* convenient function (and to preserve source compat) */
/**
 * gdl_dock_item_bind:
 * @item: The item to bind.
 * @dock: The #GdlDock widget to bind it to. Note that this widget must
 * be a type of #GdlDock.
 *
 * Binds this dock item to a new dock master.
 **/
void
gdl_dock_item_bind (GdlDockItem *item,
                    GtkWidget   *dock)
{
    g_return_if_fail (item != NULL);
    g_return_if_fail (dock == NULL || GDL_IS_DOCK (dock));

    gdl_dock_object_bind (GDL_DOCK_OBJECT (item),
                          gdl_dock_object_get_master (GDL_DOCK_OBJECT (dock)));
}

/* convenient function (and to preserve source compat) */
/**
 * gdl_dock_item_unbind:
 * @item: The item to unbind.
 *
 * Unbinds this dock item from it's dock master.
 **/
void
gdl_dock_item_unbind (GdlDockItem *item)
{
    g_return_if_fail (item != NULL);

    gdl_dock_object_unbind (GDL_DOCK_OBJECT (item));
}

/**
 * gdl_dock_item_hide_item:
 * @item: The dock item to hide.
 *
 * This function hides the dock item. Since version 3.6, when dock items
 * are hidden they are not removed from the layout.
 *
 * The dock item close button causes the panel to be hidden.
 */
void
gdl_dock_item_hide_item (GdlDockItem *item)
{
    g_return_if_fail (item != NULL);

    gtk_widget_hide (GTK_WIDGET (item));
    return;
}

/**
 * gdl_dock_item_iconify_item:
 * @item: The dock item to iconify.
 *
 * This function iconifies the dock item. When dock items are iconified
 * they are hidden, and appear only as icons in dock bars.
 *
 * The dock item iconify button causes the panel to be iconified.
 **/
void
gdl_dock_item_iconify_item (GdlDockItem *item)
{
    g_return_if_fail (item != NULL);

    item->priv->iconified = TRUE;
    gtk_widget_hide (GTK_WIDGET (item));
}

/**
 * gdl_dock_item_show_item:
 * @item: The dock item to show.
 *
 * This function shows the dock item. When dock items are shown, they
 * are displayed in their normal layout position.
 **/
void
gdl_dock_item_show_item (GdlDockItem *item)
{
    g_return_if_fail (item != NULL);

    /* Check if we need to dock the window */
    if (gtk_widget_get_parent (GTK_WIDGET (item)) == NULL) {
        if (gdl_dock_object_is_bound (GDL_DOCK_OBJECT (item))) {
            GdlDockObject *toplevel;

            toplevel = gdl_dock_object_get_controller (GDL_DOCK_OBJECT (item));
            if (toplevel == GDL_DOCK_OBJECT (item)) return;

            if (item->priv->behavior & GDL_DOCK_ITEM_BEH_NEVER_FLOATING) {
                g_warning("Object %s has no default position and flag GDL_DOCK_ITEM_BEH_NEVER_FLOATING is set.\n",
                          gdl_dock_object_get_name (GDL_DOCK_OBJECT (item)));
                return;
            } else if (toplevel) {
                gdl_dock_object_dock (toplevel, GDL_DOCK_OBJECT (item),
                                      GDL_DOCK_FLOATING, NULL);
            } else
                g_warning("There is no toplevel window. GdlDockItem %s cannot be shown.\n", gdl_dock_object_get_name (GDL_DOCK_OBJECT (item)));
                return;
        } else
            g_warning("GdlDockItem %s is not bound. It cannot be shown.\n",
                      gdl_dock_object_get_name (GDL_DOCK_OBJECT (item)));
            return;
    }

    item->priv->iconified = FALSE;
    gtk_widget_show (GTK_WIDGET (item));

    return;
}

/**
 * gdl_dock_item_lock:
 * @item: The dock item to lock.
 *
 * This function locks the dock item. When locked the dock item cannot
 * be dragged around and it doesn't show a grip.
 **/
void
gdl_dock_item_lock (GdlDockItem *item)
{
    g_object_set (item, "locked", TRUE, NULL);
}

/**
 * gdl_dock_item_unlock:
 * @item: The dock item to unlock.
 *
 * This function unlocks the dock item. When unlocked the dock item can
 * be dragged around and can show a grip.
 **/
void
gdl_dock_item_unlock (GdlDockItem *item)
{
    g_object_set (item, "locked", FALSE, NULL);
}

/**
 * gdl_dock_item_preferred_size:
 * @item: The dock item to get the preferred size of.
 * @req: A pointer to a #GtkRequisition into which the preferred size
 * will be written.
 *
 * Gets the preferred size of the dock item in pixels.
 **/
void
gdl_dock_item_preferred_size (GdlDockItem    *item,
                              GtkRequisition *req)
{
    GtkAllocation allocation;

    if (!req)
        return;

    gtk_widget_get_allocation (GTK_WIDGET (item), &allocation);

    req->width = MAX (item->priv->preferred_width, allocation.width);
    req->height = MAX (item->priv->preferred_height, allocation.height);
}

/**
 * gdl_dock_item_get_drag_area:
 * @item: The dock item to get the preferred size of.
 * @rect: A pointer to a #GdkRectangle that will receive the drag position
 *
 * Gets the size and the position of the drag window in pixels.
 *
 * Since: 3.6
 */
void
gdl_dock_item_get_drag_area (GdlDockItem    *item,
                             GdkRectangle   *rect)
{
    GtkAllocation allocation;

    g_return_if_fail (GDL_IS_DOCK_ITEM (item));
    g_return_if_fail (rect != NULL);

    rect->x = item->priv->dragoff_x;
    rect->y = item->priv->dragoff_y;
    gtk_widget_get_allocation (GTK_WIDGET (item), &allocation);
    rect->width = MAX (item->priv->preferred_width, allocation.width);
    rect->height = MAX (item->priv->preferred_height, allocation.height);
}

/**
 * gdl_dock_item_or_child_has_focus:
 * @item: The dock item to be checked
 *
 * Checks whether a given #GdlDockItem or its child widget has focus.
 * This check is performed recursively on child widgets.
 *
 * Returns: %TRUE if the dock item or its child widget has focus;
 * %FALSE otherwise.
 *
 * Since: 3.3.2
 */
gboolean
gdl_dock_item_or_child_has_focus (GdlDockItem *item)
{
    GtkWidget *item_child;
    gboolean item_or_child_has_focus;

    g_return_val_if_fail (GDL_IS_DOCK_ITEM (item), FALSE);

    for (item_child = gtk_container_get_focus_child (GTK_CONTAINER (item));
         item_child && GTK_IS_CONTAINER (item_child) && gtk_container_get_focus_child (GTK_CONTAINER (item_child));
         item_child = gtk_container_get_focus_child (GTK_CONTAINER (item_child))) ;

    item_or_child_has_focus =
        (gtk_widget_has_focus (GTK_WIDGET (item)) ||
         (GTK_IS_WIDGET (item_child) && gtk_widget_has_focus (item_child)));

    return item_or_child_has_focus;
}

/**
 * gdl_dock_item_is_placeholder:
 * @item: The dock item to be checked
 *
 * Checks whether a given #GdlDockItem is a placeholder created by the
 * #GdlDockLayout object and does not contain a child.
 *
 * Returns: %TRUE if the dock item is a placeholder
 *
 * Since: 3.6
 */
gboolean
gdl_dock_item_is_placeholder (GdlDockItem *item)
{
    return item->priv->child == NULL;
}

/**
 * gdl_dock_item_is_closed:
 * @item: The dock item to be checked
 *
 * Checks whether a given #GdlDockItem is closed. It can be only hidden or
 * detached.
 *
 * Returns: %TRUE if the dock item is closed.
 *
 * Since: 3.6
 */
gboolean
gdl_dock_item_is_closed (GdlDockItem *item)
{
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (item), FALSE);

    return gdl_dock_object_is_closed (GDL_DOCK_OBJECT (item));
}

/**
 * gdl_dock_item_is_iconified:
 * @item: The dock item to be checked
 *
 * Checks whether a given #GdlDockItem is iconified.
 *
 * Returns: %TRUE if the dock item is iconified.
 *
 * Since: 3.6
 */
gboolean
gdl_dock_item_is_iconified (GdlDockItem *item)
{
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (item), FALSE);

    return item->priv->iconified;
}

/**
 * gdl_dock_item_set_child:
 * @item: a #GdlDockItem
 * @child: (allow-none): a #GtkWidget
 *
 * Set a new child for the #GdlDockItem. This child is different from the
 * children using the #GtkContainer interface. It is a private child reserved
 * for the widget implementation.
 *
 * If a child is already present, it will be replaced. If @widget is %NULL the
 * child will be removed.
 *
 * Since: 3.6
 */
void
gdl_dock_item_set_child (GdlDockItem *item,
                         GtkWidget *child)
{
    g_return_if_fail (GDL_IS_DOCK_ITEM (item));

    if (item->priv->child != NULL) {
        gtk_widget_unparent (item->priv->child);
        item->priv->child = NULL;
    }

    if (child != NULL) {
        gtk_widget_set_parent (child, GTK_WIDGET (item));
        item->priv->child = child;
    }
}

/**
 * gdl_dock_item_get_child:
 * @item: a #GdlDockItem
 *
 * Gets the child of the #GdlDockItem, or %NULL if the item contains
 * no child widget. The returned widget does not have a reference
 * added, so you do not need to unref it.
 *
 * Return value: (transfer none): pointer to child of the #GdlDockItem
 *
 * Since: 3.6
 */
GtkWidget*
gdl_dock_item_get_child (GdlDockItem *item)
{
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (item), NULL);

    return item->priv->child;
}


/**
 * gdl_dock_item_class_set_has_grip:
 * @item_class: a #GdlDockItemClass
 * @has_grip: %TRUE is the dock item has a grip
 *
 * Define in the corresponding kind of dock item has a grip. Even if an item
 * has a grip it can be hidden.
 *
 * Since: 3.6
 */
void
gdl_dock_item_class_set_has_grip (GdlDockItemClass *item_class,
                                  gboolean         has_grip)
{
    g_return_if_fail (GDL_IS_DOCK_ITEM_CLASS (item_class));

    item_class->priv->has_grip = has_grip;
}



/* ----- gtk orientation type exporter/importer ----- */

static void
gdl_dock_param_export_gtk_orientation (const GValue *src,
                                       GValue       *dst)
{
    dst->data [0].v_pointer =
        g_strdup_printf ("%s", (src->data [0].v_int == GTK_ORIENTATION_HORIZONTAL) ?
                         "horizontal" : "vertical");
}

static void
gdl_dock_param_import_gtk_orientation (const GValue *src,
                                       GValue       *dst)
{
    if (!strcmp (src->data [0].v_pointer, "horizontal"))
        dst->data [0].v_int = GTK_ORIENTATION_HORIZONTAL;
    else
        dst->data [0].v_int = GTK_ORIENTATION_VERTICAL;
}

