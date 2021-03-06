/* dzl-menu-button.c
 *
 * Copyright (C) 2017 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define G_LOG_DOMAIN "dzl-menu-button"

#include "app/dzl-application.h"
#include "bindings/dzl-signal-group.h"
#include "menus/dzl-menu-button.h"
#include "menus/dzl-menu-button-section.h"
#include "menus/dzl-menu-button-item.h"
#include "util/dzl-gtk.h"
#include "widgets/dzl-box.h"

typedef struct
{
  /* Owned references */
  DzlSignalGroup *menu_signals;

  /* Template references */
  GtkPopover     *popover;
  GtkImage       *image;
  GtkImage       *pan_down_image;
  DzlBox         *popover_box;

  guint           show_accels : 1;
  guint           show_icons : 1;
} DzlMenuButtonPrivate;

enum {
  PROP_0,
  PROP_MODEL,
  PROP_MENU_ID,
  PROP_ICON_NAME,
  PROP_SHOW_ACCELS,
  PROP_SHOW_ARROW,
  PROP_SHOW_ICONS,
  N_PROPS
};

G_DEFINE_TYPE_WITH_PRIVATE (DzlMenuButton, dzl_menu_button, GTK_TYPE_MENU_BUTTON)

static GParamSpec *properties [N_PROPS];

static void
collect_items_sections (GtkWidget *widget,
                        GPtrArray *ar)
{
  GtkWidget *item;

  item = dzl_gtk_widget_find_child_typed (widget, DZL_TYPE_MENU_BUTTON_ITEM);
  if (item)
    g_ptr_array_add (ar, item);
}

static void
update_image_and_accels (DzlMenuButton *self)
{
  DzlMenuButtonPrivate *priv = dzl_menu_button_get_instance_private (self);
  g_autoptr(GPtrArray) ar = g_ptr_array_new ();
  gboolean show_image = dzl_menu_button_get_show_icons (self);
  gboolean show_accel = dzl_menu_button_get_show_accels (self);

  gtk_container_foreach (GTK_CONTAINER (priv->popover_box),
                         (GtkCallback) collect_items_sections,
                         ar);

  for (guint i = 0; i < ar->len; i++)
    {
      DzlMenuButtonItem *item = g_ptr_array_index (ar, i);

      g_assert (DZL_IS_MENU_BUTTON_ITEM (item));

      g_object_set (item,
                    "show-image", show_image,
                    "show-accel", show_accel,
                    NULL);
    }
}

static void
dzl_menu_button_add_linked_model (DzlMenuButton *self,
                                  guint          position,
                                  GMenuModel    *model,
                                  const gchar   *label)
{
  DzlMenuButtonPrivate *priv = dzl_menu_button_get_instance_private (self);
  DzlMenuButtonSection *section;

  g_assert (DZL_IS_MENU_BUTTON (self));
  g_assert (G_IS_MENU_MODEL (model));

  section = g_object_new (DZL_TYPE_MENU_BUTTON_SECTION,
                          "label", label,
                          "model", model,
                          "visible", TRUE,
                          NULL);
  dzl_box_insert (priv->popover_box, GTK_WIDGET (section), position);
}

static void
dzl_menu_button_items_changed (DzlMenuButton *self,
                               guint          position,
                               guint          removed,
                               guint          added,
                               GMenuModel    *menu)
{
  DzlMenuButtonPrivate *priv = dzl_menu_button_get_instance_private (self);

  g_assert (DZL_IS_MENU_BUTTON (self));
  g_assert (G_IS_MENU_MODEL (menu));

  for (guint i = 0; i < removed; i++)
    {
      GtkWidget *child = dzl_box_get_nth_child (priv->popover_box, i);
      gtk_widget_destroy (child);
    }

  for (guint i = position; i < position + added; i++)
    {
      g_autofree gchar *label = NULL;
      g_autoptr(GMenuModel) linked_model = NULL;

      /* We only support sections at the top-level */
      g_menu_model_get_item_attribute (menu, i, G_MENU_ATTRIBUTE_LABEL, "s", &label);
      linked_model = g_menu_model_get_item_link (menu, i, G_MENU_LINK_SECTION);

      if (linked_model != NULL)
        dzl_menu_button_add_linked_model (self, i, linked_model, label);
    }

  update_image_and_accels (self);
}

static void
dzl_menu_button_menu_signals_bind (DzlMenuButton  *self,
                                   GMenuModel     *menu,
                                   DzlSignalGroup *menu_signals)
{
  guint n_items;

  g_assert (DZL_IS_MENU_BUTTON (self));
  g_assert (G_IS_MENU_MODEL (menu));
  g_assert (DZL_IS_SIGNAL_GROUP (menu_signals));

  n_items = g_menu_model_get_n_items (menu);
  dzl_menu_button_items_changed (self, 0, 0, n_items, menu);

  gtk_widget_set_sensitive (GTK_WIDGET (self), TRUE);
}

static void
dzl_menu_button_menu_signals_unbind (DzlMenuButton  *self,
                                     DzlSignalGroup *menu_signals)
{
  DzlMenuButtonPrivate *priv = dzl_menu_button_get_instance_private (self);

  g_assert (DZL_IS_MENU_BUTTON (self));
  g_assert (DZL_IS_SIGNAL_GROUP (menu_signals));

  if (gtk_widget_in_destruction (GTK_WIDGET (self)))
    return;

  gtk_container_foreach (GTK_CONTAINER (priv->popover_box),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  gtk_widget_set_sensitive (GTK_WIDGET (self), FALSE);
}

static void
dzl_menu_button_set_menu_id (DzlMenuButton *self,
                             const gchar   *menu_id)
{
  GApplication *app;
  GMenu *model = NULL;

  g_return_if_fail (DZL_IS_MENU_BUTTON (self));

  app = g_application_get_default ();

  if (DZL_IS_APPLICATION (app))
    model = dzl_application_get_menu_by_id (DZL_APPLICATION (app), menu_id);
  else if (GTK_IS_APPLICATION (app))
    model = gtk_application_get_menu_by_id (GTK_APPLICATION (app), menu_id);

  dzl_menu_button_set_model (self, G_MENU_MODEL (model));
}

static void
dzl_menu_button_destroy (GtkWidget *widget)
{
  DzlMenuButton *self = (DzlMenuButton *)widget;
  DzlMenuButtonPrivate *priv = dzl_menu_button_get_instance_private (self);

  g_clear_object (&priv->menu_signals);

  GTK_WIDGET_CLASS (dzl_menu_button_parent_class)->destroy (widget);
}

static void
dzl_menu_button_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  DzlMenuButton *self = DZL_MENU_BUTTON (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, dzl_menu_button_get_model (self));
      break;

    case PROP_SHOW_ARROW:
      g_value_set_boolean (value, dzl_menu_button_get_show_arrow (self));
      break;

    case PROP_SHOW_ACCELS:
      g_value_set_boolean (value, dzl_menu_button_get_show_accels (self));
      break;

    case PROP_SHOW_ICONS:
      g_value_set_boolean (value, dzl_menu_button_get_show_icons (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dzl_menu_button_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  DzlMenuButton *self = DZL_MENU_BUTTON (object);
  DzlMenuButtonPrivate *priv = dzl_menu_button_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_MODEL:
      dzl_menu_button_set_model (self, g_value_get_object (value));
      break;

    case PROP_MENU_ID:
      dzl_menu_button_set_menu_id (self, g_value_get_string (value));
      break;

    case PROP_ICON_NAME:
      g_object_set_property (G_OBJECT (priv->image), "icon-name", value);
      break;

    case PROP_SHOW_ARROW:
      dzl_menu_button_set_show_arrow (self, g_value_get_boolean (value));
      break;

    case PROP_SHOW_ACCELS:
      dzl_menu_button_set_show_accels (self, g_value_get_boolean (value));
      break;

    case PROP_SHOW_ICONS:
      dzl_menu_button_set_show_icons (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dzl_menu_button_class_init (DzlMenuButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = dzl_menu_button_get_property;
  object_class->set_property = dzl_menu_button_set_property;

  widget_class->destroy = dzl_menu_button_destroy;

  /**
   * DzlMenuButton:menu-id:
   *
   * The "menu-id" property can be used to automatically load a
   * #GMenuModel from the applications merged menus. This is
   * performed via dzl_application_get_menu_by_id().
   *
   * Since: 3.26
   */
  properties [PROP_MENU_ID] =
    g_param_spec_string ("menu-id",
                         "Menu Id",
                         "The identifier for the menu model to use",
                         NULL,
                         (G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  properties [PROP_MODEL] =
    g_param_spec_object ("model",
                         "Model",
                         "The GMenuModel to display in the popover",
                         G_TYPE_MENU_MODEL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_ICON_NAME] =
    g_param_spec_string ("icon-name",
                         "Icon Name",
                         "The icon-name for the button",
                         NULL,
                         (G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  properties [PROP_SHOW_ACCELS] =
    g_param_spec_boolean ("show-accels",
                          "Show Accels",
                          "If accelerator keys should be shown",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_SHOW_ARROW] =
    g_param_spec_boolean ("show-arrow",
                          "Show Arrow",
                          "If the down arrow should be shown",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_SHOW_ICONS] =
    g_param_spec_boolean ("show-icons",
                          "Show Icons",
                          "If icons should be shown next to menu items",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/dazzle/ui/dzl-menu-button.ui");
  gtk_widget_class_bind_template_child_private (widget_class, DzlMenuButton, image);
  gtk_widget_class_bind_template_child_private (widget_class, DzlMenuButton, pan_down_image);
  gtk_widget_class_bind_template_child_private (widget_class, DzlMenuButton, popover);
  gtk_widget_class_bind_template_child_private (widget_class, DzlMenuButton, popover_box);
}

static void
dzl_menu_button_init (DzlMenuButton *self)
{
  DzlMenuButtonPrivate *priv = dzl_menu_button_get_instance_private (self);

  gtk_widget_init_template (GTK_WIDGET (self));

  priv->menu_signals = dzl_signal_group_new (G_TYPE_MENU_MODEL);

  g_signal_connect_swapped (priv->menu_signals,
                            "bind",
                            G_CALLBACK (dzl_menu_button_menu_signals_bind),
                            self);

  g_signal_connect_swapped (priv->menu_signals,
                            "unbind",
                            G_CALLBACK (dzl_menu_button_menu_signals_unbind),
                            self);

  dzl_signal_group_connect_swapped (priv->menu_signals,
                                    "items-changed",
                                    G_CALLBACK (dzl_menu_button_items_changed),
                                    self);

  gtk_widget_set_sensitive (GTK_WIDGET (self), FALSE);
}

/**
 * dzl_menu_button_new_with_model:
 * @icon_name: An icon-name for the button
 * @model: (nullable): A #GMenuModel or %NULL
 *
 * Creates a new #DzlMenuButton with the icon @icon_name and
 * the menu contents of @model.
 *
 * Returns: (transfer full): A #DzlMenuButton
 */
GtkWidget *
dzl_menu_button_new_with_model (const gchar *icon_name,
                                GMenuModel  *model)
{
  g_return_val_if_fail (!model || G_IS_MENU_MODEL (model), NULL);

  return g_object_new (DZL_TYPE_MENU_BUTTON,
                       "icon-name", icon_name,
                       "model", model,
                       NULL);
}

gboolean
dzl_menu_button_get_show_arrow (DzlMenuButton *self)
{
  DzlMenuButtonPrivate *priv = dzl_menu_button_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_MENU_BUTTON (self), FALSE);

  return gtk_widget_get_visible (GTK_WIDGET (priv->pan_down_image));
}

/**
 * dzl_menu_button_set_show_arrow:
 * @self: a #DzlMenuButton
 *
 * Sets the #DzlMenuButton:show-arrow property.
 *
 * If %TRUE, an pan-down-symbolic image will be displayed next to the
 * image in the button.
 *
 * Since: 3.26
 */
void
dzl_menu_button_set_show_arrow (DzlMenuButton *self,
                                gboolean       show_arrow)
{
  DzlMenuButtonPrivate *priv = dzl_menu_button_get_instance_private (self);
  GtkWidget *child;

  g_return_if_fail (DZL_IS_MENU_BUTTON (self));

  child = gtk_bin_get_child (GTK_BIN (self));
  g_object_set (child,
                "margin-start", show_arrow ? 3 : 0,
                "margin-end", show_arrow ? 3 : 0,
                NULL);
  gtk_widget_set_visible (GTK_WIDGET (priv->pan_down_image), show_arrow);
  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_SHOW_ARROW]);
}

gboolean
dzl_menu_button_get_show_icons (DzlMenuButton *self)
{
  DzlMenuButtonPrivate *priv = dzl_menu_button_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_MENU_BUTTON (self), FALSE);

  return priv->show_icons;
}

/**
 * dzl_menu_button_set_show_icons:
 * @self: a #DzlMenuButton
 * @show_icons: if icons should be visible
 *
 * Sets the #DzlMenuButton:show-icons property.
 *
 * If %TRUE, icons will be displayed next to menu items that
 * contain a shortcut.
 *
 * Since: 3.26
 */
void
dzl_menu_button_set_show_icons (DzlMenuButton *self,
                                gboolean       show_icons)
{
  DzlMenuButtonPrivate *priv = dzl_menu_button_get_instance_private (self);

  g_return_if_fail (DZL_IS_MENU_BUTTON (self));

  show_icons = !!show_icons;

  if (priv->show_icons != show_icons)
    {
      priv->show_icons = show_icons;
      update_image_and_accels (self);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_SHOW_ICONS]);
    }
}

gboolean
dzl_menu_button_get_show_accels (DzlMenuButton *self)
{
  DzlMenuButtonPrivate *priv = dzl_menu_button_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_MENU_BUTTON (self), FALSE);

  return priv->show_accels;
}

/**
 * dzl_menu_button_set_show_accels:
 * @self: a #DzlMenuButton
 * @show_accels: if accelerators should be visible
 *
 * Sets the #DzlMenuButton:show-accels property.
 *
 * If %TRUE, accelerators will be displayed next to menu items that
 * contain a shortcut.
 *
 * Since: 3.26
 */
void
dzl_menu_button_set_show_accels (DzlMenuButton *self,
                                 gboolean       show_accels)
{
  DzlMenuButtonPrivate *priv = dzl_menu_button_get_instance_private (self);

  g_return_if_fail (DZL_IS_MENU_BUTTON (self));

  show_accels = !!show_accels;

  if (priv->show_accels != show_accels)
    {
      priv->show_accels = show_accels;
      update_image_and_accels (self);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_SHOW_ICONS]);
    }
}

void
dzl_menu_button_set_model (DzlMenuButton *self,
                           GMenuModel    *model)
{
  DzlMenuButtonPrivate *priv = dzl_menu_button_get_instance_private (self);

  g_return_if_fail (DZL_IS_MENU_BUTTON (self));
  g_return_if_fail (!model || G_IS_MENU_MODEL (model));

  if ((gpointer)model != dzl_signal_group_get_target (priv->menu_signals))
    {
      dzl_signal_group_set_target (priv->menu_signals, model);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_MODEL]);
    }
}

/**
 * dzl_menu_button_get_model:
 * @self: a #DzlMenuButton
 *
 * Returns: (transfer none) (nullable): A #DzlMenuButton or %NULL.
 *
 * Since: 3.26
 */
GMenuModel *
dzl_menu_button_get_model (DzlMenuButton *self)
{
  DzlMenuButtonPrivate *priv = dzl_menu_button_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_MENU_BUTTON (self), NULL);

  return dzl_signal_group_get_target (priv->menu_signals);
}
