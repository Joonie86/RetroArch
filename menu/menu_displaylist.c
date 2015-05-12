/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2011-2015 - Daniel De Matteis
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "menu.h"
#include "menu_display.h"
#include "menu_entries.h"
#include "menu_displaylist.h"
#include "menu_navigation.h"
#include "../performance.h"

static void menu_displaylist_push_perfcounter(
      menu_displaylist_info_t *info,
      const struct retro_perf_counter **counters,
      unsigned num, unsigned id)
{
   unsigned i;
   if (!counters || num == 0)
      return;

   for (i = 0; i < num; i++)
      if (counters[i] && counters[i]->ident)
         menu_list_push(info->list,
               counters[i]->ident, "", id + i, 0);
}

static int menu_displaylist_push_perfcounter_generic(
      menu_displaylist_info_t *info,
      const struct retro_perf_counter **counters,
      unsigned num, unsigned ident)
{
   menu_list_clear(info->list);
   menu_displaylist_push_perfcounter(info, counters, num, ident);

   menu_driver_populate_entries(
         info->path, info->label, info->type);

   return 0;
}

int menu_displaylist_push_list(menu_displaylist_info_t *info, unsigned type)
{
   int ret = 0;
   menu_handle_t    *menu = menu_driver_get_ptr();
   menu_list_t *menu_list = menu_list_get_ptr();
   menu_navigation_t *nav = menu_navigation_get_ptr();
   global_t       *global = global_get_ptr();

   switch (type)
   {
      case DISPLAYLIST_NONE:
         break;
      case DISPLAYLIST_MAIN_MENU:
         menu->list_settings = setting_new(SL_FLAG_ALL);

         menu_list_push(menu_list->menu_stack,
               info->path, info->label, info->type, info->flags);
         menu_navigation_clear(nav, true);
         ret = menu_entries_push_list(menu, info->list,
               info->path, info->label, info->type, info->flags);
         break;
      case DISPLAYLIST_SETTINGS:
         ret = menu_entries_push_list(menu, info->list,
               info->path, info->label, info->type, info->flags);
         break;
      case DISPLAYLIST_CORES:
      case DISPLAYLIST_CORES_DETECTED:
      case DISPLAYLIST_SHADER_PASS:
      case DISPLAYLIST_SHADER_PRESET:
         ret = menu_entries_parse_list(info->list, info->menu_list,
               info->path, info->label, info->type,
               info->type_default, info->exts, NULL);
         break;
      case DISPLAYLIST_PERFCOUNTER_SELECTION:
         menu_list_clear(info->list);
         menu_list_push(info->list, "Frontend Counters", "frontend_counters",
               MENU_SETTING_ACTION, 0);
         menu_list_push(info->list, "Core Counters", "core_counters",
               MENU_SETTING_ACTION, 0);

         menu_driver_populate_entries(info->path, info->label, info->type);
         break;
      case DISPLAYLIST_PERFCOUNTERS_CORE:
         ret = menu_displaylist_push_perfcounter_generic(info,
               perf_counters_libretro, perf_ptr_libretro, 
               MENU_SETTINGS_LIBRETRO_PERF_COUNTERS_BEGIN);
         break;
      case DISPLAYLIST_PERFCOUNTERS_FRONTEND:
         ret = menu_displaylist_push_perfcounter_generic(info,
               perf_counters_rarch, perf_ptr_rarch, 
               MENU_SETTINGS_PERF_COUNTERS_BEGIN);
         break;
   }

   return ret;
}

int menu_displaylist_deferred_push(menu_displaylist_info_t *info)
{
   unsigned type             = 0;
   const char *path          = NULL;
   const char *label         = NULL;
   menu_file_list_cbs_t *cbs = NULL;
   menu_handle_t       *menu = menu_driver_get_ptr();

   if (!info->list)
      return -1;

   menu_list_get_last_stack(menu->menu_list, &path, &label, &type);

   if (!strcmp(label, "Main Menu"))
      return menu_entries_push_list(menu, info->list, path, label, type,
            SL_FLAG_MAIN_MENU);
   else if (!strcmp(label, "Horizontal Menu"))
      return menu_entries_push_horizontal_menu_list(menu, info->list, path, label, type);

   cbs = (menu_file_list_cbs_t*)
      menu_list_get_last_stack_actiondata(menu->menu_list);

   if (!cbs->action_deferred_push)
      return 0;

   return cbs->action_deferred_push(info->list, info->menu_list, path, label, type);
}

int menu_displaylist_push(file_list_t *list, file_list_t *menu_list)
{
   int ret;
   menu_handle_t *menu    = menu_driver_get_ptr();
   driver_t       *driver = driver_get_ptr();
   menu_displaylist_info_t info = {0};

   info.list      = list;
   info.menu_list = menu_list;
   
   ret = menu_displaylist_deferred_push(&info);

   menu->need_refresh = false;

   if (ret == 0)
   {
      const ui_companion_driver_t *ui = ui_companion_get_ptr();

      if (ui)
         ui->notify_list_loaded(driver->ui_companion_data, list, menu_list);
   }

   return ret;
}
