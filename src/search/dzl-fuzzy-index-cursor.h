/* dzl-fuzzy-index-cursor.h
 *
 * Copyright (C) 2016 Christian Hergert <christian@hergert.me>
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

#ifndef DZL_FUZZY_INDEX_CURSOR_H
#define DZL_FUZZY_INDEX_CURSOR_H

#include <gio/gio.h>

#include "dzl-fuzzy-index.h"

G_BEGIN_DECLS

#define DZL_TYPE_FUZZY_INDEX_CURSOR (dzl_fuzzy_index_cursor_get_type())

G_DECLARE_FINAL_TYPE (DzlFuzzyIndexCursor, dzl_fuzzy_index_cursor, DZL, FUZZY_INDEX_CURSOR, GObject)

DzlFuzzyIndex *dzl_fuzzy_index_cursor_get_index (DzlFuzzyIndexCursor *self);

G_END_DECLS

#endif /* DZL_FUZZY_INDEX_CURSOR_H */
