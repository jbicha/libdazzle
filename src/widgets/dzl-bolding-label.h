/* dzl-bolding-label.h
 *
 * Copyright (C) 2016 Christian Hergert <chergert@redhat.com>
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

#ifndef DZL_BOLDING_LABEL_H
#define DZL_BOLDING_LABEL_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DZL_TYPE_BOLDING_LABEL (dzl_bolding_label_get_type())

G_DECLARE_FINAL_TYPE (DzlBoldingLabel, dzl_bolding_label, DZL, BOLDING_LABEL, GtkLabel)

G_END_DECLS

#endif /* DZL_BOLDING_LABEL_H */
