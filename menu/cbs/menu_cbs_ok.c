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

#include <file/file_path.h>
#include <retro_stat.h>

#include "../menu.h"
#include "../menu_cbs.h"
#include "../menu_display.h"
#include "../menu_setting.h"
#include "../menu_shader.h"
#include "../menu_hash.h"

#include "../../general.h"
#include "../../runloop_data.h"
#include "../../input/input_remapping.h"
#include "../../system.h"

/* FIXME - Global variables, refactor */
char detect_content_path[PATH_MAX_LENGTH];
unsigned rdb_entry_start_game_selection_ptr, rpl_entry_selection_ptr;
size_t hack_shader_pass = 0;
#ifdef HAVE_NETWORKING
char core_updater_path[PATH_MAX_LENGTH];
#endif

int generic_action_ok_displaylist_push(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx,
      unsigned action_type)
{
   char tmp[PATH_MAX_LENGTH];
   char action_path[PATH_MAX_LENGTH];
   unsigned dl_type                  = DISPLAYLIST_GENERIC;
   menu_displaylist_info_t      info = {0};
   const char           *menu_label  = NULL;
   const char            *menu_path  = NULL;
   const char          *content_path = NULL;
   const char          *info_label   = NULL;
   const char          *info_path    = NULL;
   global_t                 *global  = global_get_ptr();
   settings_t            *settings   = config_get_ptr();
   menu_list_t            *menu_list = menu_list_get_ptr();
   menu_handle_t            *menu    = menu_driver_get_ptr();

   if (!menu_list)
      return -1;

   menu_list_get_last_stack(menu_list,
         &menu_path, &menu_label, NULL, NULL);

   if (path && menu_path)
      fill_pathname_join(action_path, menu_path, path, sizeof(action_path));

   info.list          = menu_list->menu_stack;

   switch (action_type)
   {
      case ACTION_OK_DL_OPEN_ARCHIVE_DETECT_CORE:
      case ACTION_OK_DL_OPEN_ARCHIVE:
         if (menu)
         {
            menu_path  = menu->scratch2_buf;
            content_path = menu->scratch_buf;
         }
         fill_pathname_join(detect_content_path, menu_path, content_path,
               sizeof(detect_content_path));

         switch (action_type)
         {
            case ACTION_OK_DL_OPEN_ARCHIVE_DETECT_CORE:
               info_label = menu_hash_to_str(MENU_LABEL_DEFERRED_ARCHIVE_OPEN_DETECT_CORE);
               break;
            case ACTION_OK_DL_OPEN_ARCHIVE:
               info_label = menu_hash_to_str(MENU_LABEL_DEFERRED_ARCHIVE_OPEN);
               break;
         }
         info_path          = path;
         info.type          = type;
         info.directory_ptr = idx;
         break;
      case ACTION_OK_DL_HELP:
         info_label = label;
         menu->push_help_screen = true;
         dl_type                = DISPLAYLIST_HELP;
         break;
      case ACTION_OK_DL_RPL_ENTRY:
         strlcpy(menu->deferred_path, label, sizeof(menu->deferred_path));
         info_label = menu_hash_to_str(MENU_LABEL_DEFERRED_RPL_ENTRY_ACTIONS);
         info.directory_ptr      = idx;
         rpl_entry_selection_ptr = idx;
         break;
      case ACTION_OK_DL_AUDIO_DSP_PLUGIN:
         info.directory_ptr = idx;
         info_path          = settings->audio.filter_dir;
         info_label         = menu_hash_to_str(MENU_LABEL_AUDIO_DSP_PLUGIN);
         break;
      case ACTION_OK_DL_SHADER_PASS:
         info.type          = type;
         info.directory_ptr = idx;
         info_path          = settings->video.shader_dir;
         info_label         = label;
         break;
      case ACTION_OK_DL_SHADER_PARAMETERS:
         info.type          = MENU_SETTING_ACTION;
         info.directory_ptr = idx;
         info_label = label;
         break;
      case ACTION_OK_DL_GENERIC:
         if (path)
            strlcpy(menu->deferred_path, path,
                  sizeof(menu->deferred_path));

         info.type          = type;
         info.directory_ptr = idx;
         info_label = label;
         break;
      case ACTION_OK_DL_PUSH_DEFAULT:
         info.type          = type;
         info.directory_ptr = idx;
         info_path          = label; 
         info_label = label;
         break;
      case ACTION_OK_DL_SHADER_PRESET:
         info.type          = type;
         info.directory_ptr = idx;
         info_path          = settings->video.shader_dir; 
         info_label = label;
         break;
      case ACTION_OK_DL_DOWNLOADS_DIR:
         info.type          = MENU_FILE_DIRECTORY;
         info.directory_ptr = idx;
         info_path          = settings->core_assets_directory;
         info_label         = label;
         break;
      case ACTION_OK_DL_CONTENT_LIST:
         info.type          = MENU_FILE_DIRECTORY;
         info.directory_ptr = idx;
         info_path          = settings->menu_content_directory;
         info_label         = label;
         break;
      case ACTION_OK_DL_REMAP_FILE:
         info.type          = type;
         info.directory_ptr = idx;
         info_path          = settings->input_remapping_directory;
         info_label         = label;
         break;
      case ACTION_OK_DL_RECORD_CONFIGFILE:
         info.type          = type;
         info.directory_ptr = idx;
         info_path          = global->record.config_dir;
         info_label         = label;
         break;
      case ACTION_OK_DL_DISK_IMAGE_APPEND_LIST:
         info.type          = type;
         info.directory_ptr = idx;
         info_path          = settings->menu_content_directory;
         info_label         = label;
         break;
      case ACTION_OK_DL_PLAYLIST_COLLECTION:
         info.type          = type;
         info.directory_ptr = idx;
         info_path          = path;
         info_label         = label;
         break;
      case ACTION_OK_DL_CHEAT_FILE:
         info.type          = type;
         info.directory_ptr = idx;
         info_path          = settings->cheat_database;
         info_label         = label;
         break;
      case ACTION_OK_DL_CORE_LIST:
         info.type          = type;
         info.directory_ptr = idx;
         info_path          = settings->libretro_directory;
         info_label         = label;
         break;
      case ACTION_OK_DL_CONTENT_COLLECTION_LIST:
         info.type          = type;
         info.directory_ptr = idx;
         info_path          = settings->playlist_directory;
         info_label         = label;
         break;
      case ACTION_OK_DL_RDB_ENTRY:
         fill_pathname_join_delim(tmp, menu_hash_to_str(MENU_LABEL_DEFERRED_RDB_ENTRY_DETAIL),
               path, '|', sizeof(tmp));

         info.directory_ptr = idx;
         info_path          = label;
         info_label         = tmp;
         break;
      case ACTION_OK_DL_RDB_ENTRY_SUBMENU:
         info.directory_ptr = idx;
         info_label         = label;
         info_path          = path;
         break;
      case ACTION_OK_DL_CONFIGURATIONS_LIST:
         info.type          = type;
         info.directory_ptr = idx;
         if (settings->menu_config_directory[0] != '\0')
            info_path        = settings->menu_config_directory;
         else
            info_path        = label;
         info_label = label;
         break;
      case ACTION_OK_DL_COMPRESSED_ARCHIVE_PUSH_DETECT_CORE:
      case ACTION_OK_DL_COMPRESSED_ARCHIVE_PUSH:
         info.type          = type;
         info.directory_ptr = idx;
         info_path          = path;
         switch (action_type)
         {
            case ACTION_OK_DL_COMPRESSED_ARCHIVE_PUSH_DETECT_CORE:
               info_label = menu_hash_to_str(MENU_LABEL_DEFERRED_ARCHIVE_ACTION_DETECT_CORE);
               break;
            case ACTION_OK_DL_COMPRESSED_ARCHIVE_PUSH:
               info_label = menu_hash_to_str(MENU_LABEL_DEFERRED_ARCHIVE_ACTION);
               break;
         }

         strlcpy(menu->scratch_buf, path, sizeof(menu->scratch_buf));
         strlcpy(menu->scratch2_buf, menu_path, sizeof(menu->scratch2_buf));
         break;
      case ACTION_OK_DL_DIRECTORY_PUSH:
         info.type          = type;
         info.directory_ptr = idx;
         info_path          = action_path;
         info_label         = menu_label;
         break;
      case ACTION_OK_DL_DATABASE_MANAGER_LIST:
         fill_pathname_join(tmp, settings->content_database,
               path, sizeof(tmp));

         info.directory_ptr = idx;
         info_path          = tmp;
         info_label         = menu_hash_to_str(MENU_LABEL_DEFERRED_DATABASE_MANAGER_LIST);
         break;
      case ACTION_OK_DL_CURSOR_MANAGER_LIST:
         fill_pathname_join(tmp, settings->cursor_directory,
               path, sizeof(tmp));

         info.directory_ptr = idx;
         info_path          = tmp;
         info_label         = menu_hash_to_str(MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST);
         break;
      case ACTION_OK_DL_CORE_UPDATER_LIST:
         info.type          = type;
         info.directory_ptr = idx;
         info_path          = path;
         info_label         = menu_hash_to_str(MENU_LABEL_DEFERRED_CORE_UPDATER_LIST);
         break;
      case ACTION_OK_DL_CORE_CONTENT_LIST:
         info.type          = type;
         info.directory_ptr = idx;
         info_path          = path;
         info_label         = menu_hash_to_str(MENU_LABEL_DEFERRED_CORE_CONTENT_LIST);
         break;
      case ACTION_OK_DL_DEFERRED_CORE_LIST:
         info.directory_ptr = idx;
         info_path          = settings->libretro_directory;
         info_label         = menu_hash_to_str(MENU_LABEL_DEFERRED_CORE_LIST);
         break;
      case ACTION_OK_DL_DEFERRED_CORE_LIST_SET:
         info.directory_ptr                 = idx;
         rdb_entry_start_game_selection_ptr = idx;
         info_path                          = settings->libretro_directory;
         info_label                         = menu_hash_to_str(MENU_LABEL_DEFERRED_CORE_LIST_SET);
         break;
      case ACTION_OK_DL_CONTENT_SETTINGS:
         dl_type            = DISPLAYLIST_CONTENT_SETTINGS;
         info.list          = menu_list->selection_buf;
         info_path          = menu_hash_to_str(MENU_LABEL_VALUE_CONTENT_SETTINGS);
         info_label         = menu_hash_to_str(MENU_LABEL_CONTENT_SETTINGS);
         menu_list_push(menu_list->menu_stack,
               info_path, info_label, 0, 0, 0);
         break;
   }

   if (info_label)
      strlcpy(info.label, info_label, sizeof(info.label));
   if (info_path)
      strlcpy(info.path, info_path, sizeof(info.path));

   return menu_displaylist_push_list(&info, dl_type);
}

static int rarch_defer_core_wrapper(size_t idx, size_t entry_idx, const char *path, uint32_t hash_label,
      bool is_carchive)
{
   char menu_path_new[PATH_MAX_LENGTH];
   const char *menu_path    = NULL;
   const char *menu_label   = NULL;
   int ret                  = 0;
   menu_handle_t *menu      = menu_driver_get_ptr();
   menu_list_t *menu_list   = menu_list_get_ptr();
   global_t *global         = global_get_ptr();

   if (!menu)
      return -1;

   menu_list_get_last_stack(menu_list,
         &menu_path, &menu_label, NULL, NULL);

   strlcpy(menu_path_new, menu_path, sizeof(menu_path_new));

   if (
         !strcmp(menu_label, menu_hash_to_str(MENU_LABEL_DEFERRED_ARCHIVE_OPEN_DETECT_CORE))
      )
   {
      fill_pathname_join(menu_path_new, menu->scratch2_buf, menu->scratch_buf,
            sizeof(menu_path_new));
   }
   else if (!strcmp(menu_label, menu_hash_to_str(MENU_LABEL_DEFERRED_ARCHIVE_OPEN)))
   {
      fill_pathname_join(menu_path_new, menu->scratch2_buf, menu->scratch_buf,
            sizeof(menu_path_new));
   }

   ret = rarch_defer_core(global->core_info.list,
         menu_path_new, path, menu_label, menu->deferred_path,
         sizeof(menu->deferred_path));

   if (!is_carchive)
      fill_pathname_join(detect_content_path, menu_path_new, path,
            sizeof(detect_content_path));

   if (hash_label == MENU_LABEL_COLLECTION)
      return generic_action_ok_displaylist_push(path,
            NULL, 0, idx, entry_idx, ACTION_OK_DL_DEFERRED_CORE_LIST_SET);

   switch (ret)
   {
      case -1:
         event_command(EVENT_CMD_LOAD_CORE);
         return menu_common_load_content(NULL, NULL, false, CORE_TYPE_PLAIN);
      case 0:
         return generic_action_ok_displaylist_push(path,
               NULL, 0, idx, entry_idx, ACTION_OK_DL_DEFERRED_CORE_LIST);
      default:
         break;
   }

   return ret;
}

static int action_ok_file_load_with_detect_core_carchive(
      const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   uint32_t hash_label      = menu_hash_calculate(label);

   fill_pathname_join_delim(detect_content_path, detect_content_path, path,
         '#', sizeof(detect_content_path));

   return rarch_defer_core_wrapper(idx, entry_idx, path, hash_label, true);
}

static int action_ok_file_load_with_detect_core(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   uint32_t hash_label      = menu_hash_calculate(label);

   return rarch_defer_core_wrapper(idx, entry_idx, path, hash_label, false);
}

static int action_ok_file_load_detect_core(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return menu_common_load_content(path, detect_content_path,
         false, CORE_TYPE_PLAIN);
}


static int action_ok_playlist_entry(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   size_t selection;
   uint32_t core_name_hash, core_path_hash;
   const char *entry_path       = NULL;
   const char *entry_label      = NULL;
   const char *core_path        = NULL;
   const char *core_name        = NULL;
   size_t selection_ptr         = 0;
   content_playlist_t *playlist = g_defaults.history;
   bool is_history              = true;
   menu_handle_t *menu          = menu_driver_get_ptr();
   menu_list_t *menu_list       = menu_list_get_ptr();
   uint32_t hash_label          = menu_hash_calculate(label);

   if (!menu)
      return -1;
   if (!menu_navigation_ctl(MENU_NAVIGATION_CTL_GET_SELECTION, &selection))
      return -1;

   switch (hash_label)
   {
      case MENU_LABEL_COLLECTION:
      case MENU_LABEL_RDB_ENTRY_START_CONTENT:
         if (!menu->playlist)
         {
            menu->playlist = content_playlist_init(menu->db_playlist_file, COLLECTION_SIZE);

            if (!menu->playlist)
               return -1;
         }

         playlist = menu->playlist;
         is_history = false;
         break;
   }

   selection_ptr = entry_idx;

   switch (hash_label)
   {
      case MENU_LABEL_RDB_ENTRY_START_CONTENT:
         selection_ptr = rdb_entry_start_game_selection_ptr;
         break;
   }

   content_playlist_get_index(playlist, selection_ptr,
         &entry_path, &entry_label, &core_path, &core_name, NULL, NULL); 

#if 0
   RARCH_LOG("path: %s, label: %s, core path: %s, core name: %s, idx: %d\n", entry_path, entry_label,
         core_path, core_name, selection_ptr);
#endif

   core_path_hash = core_path ? menu_hash_calculate(core_path) : 0;
   core_name_hash = core_name ? menu_hash_calculate(core_name) : 0;

   if (
         (core_path_hash == MENU_VALUE_DETECT) &&
         (core_name_hash == MENU_VALUE_DETECT)
      )
      return action_ok_file_load_with_detect_core(entry_path, label, type, selection_ptr, entry_idx);

   rarch_playlist_load_content(playlist, selection_ptr);

   if (is_history)
   {
      switch (hash_label)
      {
         case MENU_LABEL_COLLECTION:
         case MENU_LABEL_RDB_ENTRY_START_CONTENT:
            menu_list_pop_stack(menu_list, &selection);
            menu_navigation_ctl(MENU_NAVIGATION_CTL_SET_SELECTION, &selection);
            break;
         default:
            menu_list_flush_stack(menu_list, NULL, MENU_SETTINGS);
            break;
      }

      generic_action_ok_displaylist_push("",
            "", 0, 0, 0, ACTION_OK_DL_CONTENT_SETTINGS);
   }

   return -1;
}

static int action_ok_cheat_apply_changes(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   event_command(EVENT_CMD_CHEATS_APPLY);

   return 0;
}

enum
{
   ACTION_OK_LOAD_PRESET = 0,
   ACTION_OK_LOAD_SHADER_PASS,
   ACTION_OK_LOAD_RECORD_CONFIGFILE,
   ACTION_OK_LOAD_REMAPPING_FILE,
   ACTION_OK_LOAD_CHEAT_FILE,
   ACTION_OK_APPEND_DISK_IMAGE,
   ACTION_OK_LOAD_CONFIG_FILE,
   ACTION_OK_LOAD_CORE,
   ACTION_OK_LOAD_WALLPAPER,
   ACTION_OK_SET_PATH
};


static int generic_action_ok(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx,
      unsigned id, unsigned flush_id)
{
   bool msg_force;
   char action_path[PATH_MAX_LENGTH];
   unsigned flush_type               = 0;
   int ret                           = 0;
   const char             *menu_path = NULL;
   const char *flush_char            = NULL;
   global_t                  *global = global_get_ptr();
   menu_handle_t               *menu = menu_driver_get_ptr();
   settings_t              *settings = config_get_ptr();
   menu_list_t       *menu_list      = menu_list_get_ptr();

   if (!menu || !menu_list)
      goto error;

   menu_list_get_last_stack(menu_list, &menu_path, NULL,
         NULL, NULL);

   fill_pathname_join(action_path, menu_path, path, sizeof(action_path));
   flush_char = menu_hash_to_str(flush_id);

   switch (id)
   {
      case ACTION_OK_LOAD_WALLPAPER:
         flush_char = NULL;
         flush_type = 49;
         if (path_file_exists(action_path))
         {
            strlcpy(settings->menu.wallpaper, action_path, sizeof(settings->menu.wallpaper));

            rarch_main_data_msg_queue_push(DATA_TYPE_IMAGE, action_path, "cb_menu_wallpaper", 0, 1,
                  true);
         }
         break;
      case ACTION_OK_LOAD_CORE:
         flush_char = NULL;
         flush_type = MENU_SETTINGS;
         strlcpy(settings->libretro, action_path, sizeof(settings->libretro));
         event_command(EVENT_CMD_LOAD_CORE);

#if defined(HAVE_DYNAMIC)
         /* No content needed for this core, load core immediately. */
         if (menu->load_no_content && settings->core.set_supports_no_game_enable)
         {
            *global->path.fullpath = '\0';
            ret = menu_common_load_content(NULL, NULL, false, CORE_TYPE_PLAIN);
         }
         else
            ret = 0;
#elif defined(RARCH_CONSOLE)
         /* Core selection on non-console just updates directory listing.
          * Will take effect on new content load. */
         ret = -1;
         event_command(EVENT_CMD_RESTART_RETROARCH);
#endif

         break;

      case ACTION_OK_LOAD_CONFIG_FILE:
         flush_char      = NULL;
         flush_type      = MENU_SETTINGS;
         msg_force       = true;
         menu_display_ctl(MENU_DISPLAY_CTL_SET_MSG_FORCE, &msg_force);

         if (rarch_ctl(RARCH_ACTION_STATE_REPLACE_CONFIG, action_path))
         {
            bool pending_push = false;
            menu_navigation_ctl(MENU_NAVIGATION_CTL_CLEAR, &pending_push);
            ret = -1;
         }
         break;
#ifdef HAVE_SHADER_MANAGER
      case ACTION_OK_LOAD_PRESET:
         menu_shader_manager_set_preset(menu->shader,
               video_shader_parse_type(action_path, RARCH_SHADER_NONE),
               action_path);
         break;
      case ACTION_OK_LOAD_SHADER_PASS:
         strlcpy(
               menu->shader->pass[hack_shader_pass].source.path,
               action_path,
               sizeof(menu->shader->pass[hack_shader_pass].source.path));
         video_shader_resolve_parameters(NULL, menu->shader);
         break;
#endif
      case ACTION_OK_LOAD_RECORD_CONFIGFILE:
         strlcpy(global->record.config, action_path,
               sizeof(global->record.config));
         break;
      case ACTION_OK_LOAD_REMAPPING_FILE:
         input_remapping_load_file(action_path);
         break;
      case ACTION_OK_LOAD_CHEAT_FILE:
         if (global->cheat)
            cheat_manager_free(global->cheat);

         global->cheat = cheat_manager_load(action_path);

         if (!global->cheat)
            goto error;
         break;
      case ACTION_OK_APPEND_DISK_IMAGE:
         flush_char = NULL;
         flush_type = 49;
         event_disk_control_append_image(action_path);
         event_command(EVENT_CMD_RESUME);
         break;
      case ACTION_OK_SET_PATH:
         flush_char = NULL;
         flush_type = 49;
         {
            menu_file_list_cbs_t *cbs = menu_list_get_last_stack_actiondata(menu_list);

            if (cbs)
            {
               setting_set_with_string_representation(cbs->setting, action_path);
               ret = menu_setting_generic(cbs->setting, false);
            }
         }
         break;
      default:
         break;
   }

   menu_list_flush_stack(menu_list, flush_char, flush_type);

   return ret;

error:
   return -1;
}

static int action_ok_set_path(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok(path, label, type, idx, entry_idx,
         ACTION_OK_SET_PATH, MENU_SETTINGS);
}

static int action_ok_menu_wallpaper_load(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{

   return generic_action_ok(path, label, type, idx, entry_idx,
         ACTION_OK_LOAD_WALLPAPER, MENU_SETTINGS);
}

static int action_ok_core_load(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok(path, label, type, idx, entry_idx,
         ACTION_OK_LOAD_CORE, MENU_SETTINGS);
}

static int action_ok_config_load(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok(path, label, type, idx, entry_idx,
         ACTION_OK_LOAD_CONFIG_FILE, MENU_SETTINGS);
}

static int action_ok_disk_image_append(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok(path, label, type, idx, entry_idx,
         ACTION_OK_APPEND_DISK_IMAGE, MENU_SETTINGS);
}

static int action_ok_cheat_file_load(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok(path, label, type, idx, entry_idx,
         ACTION_OK_LOAD_CHEAT_FILE, MENU_LABEL_CORE_CHEAT_OPTIONS);
}

static int action_ok_record_configfile_load(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok(path, label, type, idx, entry_idx,
         ACTION_OK_LOAD_RECORD_CONFIGFILE, MENU_LABEL_RECORDING_SETTINGS);
}

static int action_ok_remap_file_load(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok(path, label, type, idx, entry_idx,
         ACTION_OK_LOAD_REMAPPING_FILE, MENU_LABEL_CORE_INPUT_REMAPPING_OPTIONS);
}

static int action_ok_shader_preset_load(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok(path, label, type, idx, entry_idx,
         ACTION_OK_LOAD_PRESET, MENU_LABEL_SHADER_OPTIONS);
}

static int action_ok_shader_pass_load(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok(path, label, type, idx, entry_idx,
         ACTION_OK_LOAD_SHADER_PASS, MENU_LABEL_SHADER_OPTIONS);
}


static int action_ok_cheat(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   menu_input_key_start_line("Input Cheat",
         label, type, idx, menu_input_st_cheat_callback);
   return 0;
}

static int action_ok_shader_preset_save_as(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   menu_input_key_start_line("Preset Filename",
         label, type, idx, menu_input_st_string_callback);
   return 0;
}

static int action_ok_cheat_file_save_as(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   menu_input_key_start_line("Cheat Filename",
         label, type, idx, menu_input_st_string_callback);
   return 0;
}

static int action_ok_remap_file_save_as(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   menu_input_key_start_line("Remapping Filename",
         label, type, idx, menu_input_st_string_callback);
   return 0;
}

enum
{
   ACTION_OK_REMAP_FILE_SAVE_CORE = 0,
   ACTION_OK_REMAP_FILE_SAVE_GAME
};

static int generic_action_ok_remap_file_save(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx,
      unsigned action_type)
{
   char directory[PATH_MAX_LENGTH];
   char file[PATH_MAX_LENGTH];
   global_t *global                = global_get_ptr();
   settings_t *settings            = config_get_ptr();
   rarch_system_info_t *info       = rarch_system_info_get_ptr();
   const char *game_name           = NULL;
   const char *core_name           = info ? info->info.library_name : NULL;

   fill_pathname_join(directory, settings->input_remapping_directory, core_name, PATH_MAX_LENGTH);

   switch (action_type)
   {
      case ACTION_OK_REMAP_FILE_SAVE_CORE:
         fill_pathname_join(file, core_name, core_name, PATH_MAX_LENGTH);
         break;
      case ACTION_OK_REMAP_FILE_SAVE_GAME:
         if (global)
            game_name           = path_basename(global->name.base);
         fill_pathname_join(file, core_name, game_name, PATH_MAX_LENGTH);
         break;
   }

   if(!path_file_exists(directory))
       path_mkdir(directory);

   if(input_remapping_save_file(file))
      menu_display_msg_queue_push("Remap file saved successfully", 1, 100, true);
   else
      menu_display_msg_queue_push("Error saving remap file", 1, 100, true);

   return 0;
}

static int action_ok_remap_file_save_core(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_remap_file_save(path, label, type,
         idx, entry_idx, ACTION_OK_REMAP_FILE_SAVE_CORE);
}

static int action_ok_remap_file_save_game(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_remap_file_save(path, label, type,
         idx, entry_idx, ACTION_OK_REMAP_FILE_SAVE_GAME);
}

int action_ok_path_use_directory(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return menu_entry_pathdir_set_value(0, NULL);
}

static int action_ok_scan_file(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return action_scan_file(path, label, type, idx);
}

static int action_ok_path_scan_directory(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return action_scan_directory(NULL, label, type, idx);
}

static int action_ok_core_deferred_set(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   size_t selection;
   char core_display_name[PATH_MAX_LENGTH];
   menu_handle_t               *menu = menu_driver_get_ptr();
   menu_list_t            *menu_list = menu_list_get_ptr();
   if (!menu || !menu_list)
      return -1;
   if (!menu_navigation_ctl(MENU_NAVIGATION_CTL_GET_SELECTION, &selection))
      return -1;

   rarch_assert(menu->playlist != NULL);

   core_info_get_name(path, core_display_name, sizeof(core_display_name));

   idx = rdb_entry_start_game_selection_ptr;

   content_playlist_update(menu->playlist, idx,
         menu->playlist->entries[idx].path, menu->playlist->entries[idx].label,
         path , core_display_name,
         menu->playlist->entries[idx].crc32,
         menu->playlist->entries[idx].db_name);

   content_playlist_write_file(menu->playlist);

   menu_list_pop_stack(menu_list, &selection);
   menu_navigation_ctl(MENU_NAVIGATION_CTL_SET_SELECTION, &selection);

   return -1;
}

static int action_ok_core_load_deferred(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   menu_handle_t *menu      = menu_driver_get_ptr();

   if (!menu)
      return -1;

   return menu_common_load_content(path, menu->deferred_path, false, CORE_TYPE_PLAIN);
}

static int action_ok_deferred_list_stub(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return 0;
}

enum
{
   ACTION_OK_FFMPEG = 0,
   ACTION_OK_IMAGEVIEWER
};

static int generic_action_ok_file_load(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx,
      enum rarch_core_type action_type, unsigned id)
{
   char new_path[PATH_MAX_LENGTH];
   const char *menu_path    = NULL;
   menu_list_t   *menu_list = menu_list_get_ptr();

   (void)id;

   if (!menu_list)
      return -1;

   menu_list_get_last(menu_list->menu_stack,
         &menu_path, NULL, NULL, NULL);

   fill_pathname_join(new_path, menu_path, path,
         sizeof(new_path));

   switch (id)
   {
      case ACTION_OK_FFMPEG:
      case ACTION_OK_IMAGEVIEWER:
         menu_common_load_content(NULL, new_path, true, action_type);
         break;
      default:
         break;
   }

   return 0;
}

#ifdef HAVE_FFMPEG
static int action_ok_file_load_ffmpeg(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_file_load(path, label, type, idx,
         entry_idx, CORE_TYPE_FFMPEG, ACTION_OK_FFMPEG);
}
#endif

static int action_ok_file_load_imageviewer(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_file_load(path, label, type, idx,
         entry_idx, CORE_TYPE_IMAGEVIEWER, ACTION_OK_IMAGEVIEWER);
}

static int action_ok_file_load(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   char menu_path_new[PATH_MAX_LENGTH];
   char full_path_new[PATH_MAX_LENGTH];
   const char *menu_label   = NULL;
   const char *menu_path    = NULL;
   rarch_setting_t *setting = NULL;
   menu_handle_t *menu      = menu_driver_get_ptr();
   menu_list_t   *menu_list = menu_list_get_ptr();

   if (!menu_list)
      return -1;

   menu_list_get_last(menu_list->menu_stack,
         &menu_path, &menu_label, NULL, NULL);

   strlcpy(menu_path_new, menu_path, sizeof(menu_path_new));

   if (
         !strcmp(menu_label, menu_hash_to_str(MENU_LABEL_DEFERRED_ARCHIVE_OPEN_DETECT_CORE)) ||
         !strcmp(menu_label, menu_hash_to_str(MENU_LABEL_DEFERRED_ARCHIVE_OPEN))
      )
   {
      fill_pathname_join(menu_path_new, menu->scratch2_buf, menu->scratch_buf,
            sizeof(menu_path_new));
   }

   setting = menu_setting_find(menu_label);

   if (menu_setting_get_type(setting) == ST_PATH)
      return action_ok_set_path(path, label, type, idx, entry_idx);

   if (type == MENU_FILE_IN_CARCHIVE)
      fill_pathname_join_delim(full_path_new, menu_path_new, path,
            '#',sizeof(full_path_new));
   else
      fill_pathname_join(full_path_new, menu_path_new, path,
            sizeof(full_path_new));

   return menu_common_load_content(NULL, full_path_new, true, CORE_TYPE_PLAIN);
}


static int generic_action_ok_command(enum event_command cmd)
{
   if (!event_command(cmd))
      return -1;
   return 0;
}

static int action_ok_load_state(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   if (generic_action_ok_command(EVENT_CMD_LOAD_STATE) == -1)
      return -1;
   return generic_action_ok_command(EVENT_CMD_RESUME);
 }


static int action_ok_save_state(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   if (generic_action_ok_command(EVENT_CMD_SAVE_STATE) == -1)
      return -1;
   return generic_action_ok_command(EVENT_CMD_RESUME);
}

static int action_ok_download_generic(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx,
      const char *type_msg)
{
#ifdef HAVE_NETWORKING
   char s[PATH_MAX_LENGTH];
   char s2[PATH_MAX_LENGTH];
   settings_t *settings            = config_get_ptr();

   fill_pathname_join(s, settings->network.buildbot_assets_url,
         "frontend", sizeof(s));
   if (!strcmp(type_msg, "cb_core_content_download"))
   {
      fill_pathname_join(s, settings->network.buildbot_assets_url,
            "cores/gw", sizeof(s));
   }
   else if (!strcmp(type_msg, "cb_update_assets"))
      path = "assets.zip";
   else if (!strcmp(type_msg, "cb_update_autoconfig_profiles"))
      path = "autoconf.zip";

#ifdef HAVE_HID
   else if (!strcmp(type_msg, "cb_update_autoconfig_profiles_hid"))
      path = "autoconf_hid.zip";
#endif
   else if (!strcmp(type_msg, "cb_update_core_info_files"))
      path = "info.zip";
   else if (!strcmp(type_msg, "cb_update_cheats"))
      path = "cheats.zip";
   else if (!strcmp(type_msg, "cb_update_overlays"))
      path = "overlays.zip";
   else if (!strcmp(type_msg, "cb_update_databases"))
      path = "database-rdb.zip";
   else if (!strcmp(type_msg, "cb_update_shaders_glsl"))
      path = "shaders_glsl.zip";
   else if (!strcmp(type_msg, "cb_update_shaders_cg"))
      path = "shaders_cg.zip";
   else
      strlcpy(s, settings->network.buildbot_url, sizeof(s));

   fill_pathname_join(s, s, path, sizeof(s));

   strlcpy(core_updater_path, path, sizeof(core_updater_path));

   snprintf(s2, sizeof(s2),
         "%s %s.",
         menu_hash_to_str(MENU_LABEL_VALUE_STARTING_DOWNLOAD),
         path);

   menu_display_msg_queue_push(s2, 1, 90, true);

   rarch_main_data_msg_queue_push(DATA_TYPE_HTTP, s,
         type_msg, 0, 1, true);
#endif
   return 0;
}

static int action_ok_core_content_download(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return action_ok_download_generic(path, label, type, idx, entry_idx,
         "cb_core_content_download");
}

static int action_ok_core_updater_download(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return action_ok_download_generic(path, label, type, idx, entry_idx,
         "cb_core_updater_download");
}

static int action_ok_update_assets(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return action_ok_download_generic(path, label, type, idx, entry_idx,
         "cb_update_assets");
}

static int action_ok_update_core_info_files(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return action_ok_download_generic(path, label, type, idx, entry_idx,
         "cb_update_core_info_files");
}

static int action_ok_update_overlays(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return action_ok_download_generic(path, label, type, idx, entry_idx,
         "cb_update_overlays");
}

static int action_ok_update_shaders_cg(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return action_ok_download_generic(path, label, type, idx, entry_idx,
         "cb_update_shaders_cg");
}

static int action_ok_update_shaders_glsl(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return action_ok_download_generic(path, label, type, idx, entry_idx,
         "cb_update_shaders_glsl");
}

static int action_ok_update_databases(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return action_ok_download_generic(path, label, type, idx, entry_idx,
         "cb_update_databases");
}

static int action_ok_update_cheats(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return action_ok_download_generic(path, label, type, idx, entry_idx,
         "cb_update_cheats");
}

static int action_ok_update_autoconfig_profiles(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return action_ok_download_generic(path, label, type, idx, entry_idx,
         "cb_update_autoconfig_profiles");
}

static int action_ok_update_autoconfig_profiles_hid(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return action_ok_download_generic(path, label, type, idx, entry_idx,
         "cb_update_autoconfig_profiles_hid");
}

static int action_ok_disk_cycle_tray_status(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_command(EVENT_CMD_DISK_EJECT_TOGGLE);
}

static int action_ok_close_content(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_command(EVENT_CMD_UNLOAD_CORE);
}

static int action_ok_quit(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_command(EVENT_CMD_QUIT);
}

static int action_ok_save_new_config(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_command(EVENT_CMD_MENU_SAVE_CONFIG);
}

static int action_ok_resume_content(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_command(EVENT_CMD_RESUME);
}

static int action_ok_restart_content(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_command(EVENT_CMD_RESET);
}

static int action_ok_screenshot(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_command(EVENT_CMD_TAKE_SCREENSHOT);
}

static int action_ok_file_load_or_resume(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   menu_handle_t *menu   = menu_driver_get_ptr();
   global_t      *global = global_get_ptr();

   if (!menu)
      return -1;

   if (!strcmp(menu->deferred_path, global->path.fullpath))
      return generic_action_ok_command(EVENT_CMD_RESUME);

   strlcpy(global->path.fullpath,
         menu->deferred_path, sizeof(global->path.fullpath));
   event_command(EVENT_CMD_LOAD_CORE);
   rarch_ctl(RARCH_ACTION_STATE_LOAD_CONTENT, NULL);

   return -1;
}

static int action_ok_shader_apply_changes(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_command(EVENT_CMD_SHADERS_APPLY_CHANGES);
}

static int action_ok_lookup_setting(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return menu_setting_set(type, label, MENU_ACTION_OK, false);
}


#ifdef HAVE_NETWORKING
enum
{
   ACTION_OK_NETWORK_CORE_CONTENT_LIST = 0,
   ACTION_OK_NETWORK_CORE_UPDATER_LIST
};

static int generic_action_ok_network(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx,
      unsigned type_id)
{
   char url_path[PATH_MAX_LENGTH];
   settings_t *settings           = config_get_ptr();
   unsigned type_id2              = 0;
   const char *url_label          = NULL;

   menu_entries_set_refresh(true);

   if (settings->network.buildbot_url[0] == '\0')
      return -1;

   event_command(EVENT_CMD_NETWORK_INIT);

   switch (type_id)
   {
      case ACTION_OK_NETWORK_CORE_CONTENT_LIST:
         fill_pathname_join(url_path, settings->network.buildbot_assets_url,
               "cores/gw/.index", sizeof(url_path));
         url_label = "cb_core_content_list";
         type_id2 = ACTION_OK_DL_CORE_CONTENT_LIST;
         break;
      case ACTION_OK_NETWORK_CORE_UPDATER_LIST:
         fill_pathname_join(url_path, settings->network.buildbot_url,
               ".index", sizeof(url_path));
         url_label = "cb_core_updater_list";
         type_id2  = ACTION_OK_DL_CORE_UPDATER_LIST;
         break;
   }

   rarch_main_data_msg_queue_push(DATA_TYPE_HTTP, url_path, url_label, 0, 1,
         true);
   return generic_action_ok_displaylist_push(path,
         label, type, idx, entry_idx, type_id2);
}

static int action_ok_core_content_list(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_network(path, label, type, idx, entry_idx,
         ACTION_OK_NETWORK_CORE_CONTENT_LIST);
} 

static int action_ok_core_updater_list(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_network(path, label, type, idx, entry_idx,
         ACTION_OK_NETWORK_CORE_UPDATER_LIST);
}
#endif


static int action_ok_rdb_entry_submenu(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   union string_list_elem_attr attr;
   char new_label[PATH_MAX_LENGTH];
   char new_path[PATH_MAX_LENGTH];
   int ret = -1;
   char *rdb                       = NULL;
   int len                         = 0;
   struct string_list *str_list    = NULL;
   struct string_list *str_list2   = NULL;
   menu_list_t          *menu_list = menu_list_get_ptr();

   if (!menu_list || !label)
      return -1;

   str_list = string_split(label, "|");

   if (!str_list)
      goto end;

   str_list2 = string_list_new();
   if (!str_list2)
      goto end;

   /* element 0 : label
    * element 1 : value
    * element 2 : database path
    */

   attr.i = 0;

   len += strlen(str_list->elems[1].data) + 1;
   string_list_append(str_list2, str_list->elems[1].data, attr);

   len += strlen(str_list->elems[2].data) + 1;
   string_list_append(str_list2, str_list->elems[2].data, attr);

   rdb = (char*)calloc(len, sizeof(char));

   if (!rdb)
      goto end;

   string_list_join_concat(rdb, len, str_list2, "|");
   strlcpy(new_path, rdb, sizeof(new_path));

   fill_pathname_join_delim(new_label, 
         menu_hash_to_str(MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST),
         str_list->elems[0].data, '_',
         sizeof(new_label));

   ret = generic_action_ok_displaylist_push(new_path,
         new_label, type, idx, entry_idx,
         ACTION_OK_DL_RDB_ENTRY_SUBMENU);

end:
   if (str_list)
      string_list_free(str_list);
   if (str_list2)
      string_list_free(str_list2);

   return ret;
}

#ifdef HAVE_SHADER_MANAGER
extern size_t hack_shader_pass;
#endif

static int action_ok_shader_pass(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   hack_shader_pass             = type - MENU_SETTINGS_SHADER_PASS_0;
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_SHADER_PASS);
}

static int action_ok_shader_parameters(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_SHADER_PARAMETERS);
}

int action_ok_directory_push(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_DIRECTORY_PUSH);
}

static int action_ok_database_manager_list(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_DATABASE_MANAGER_LIST);
}

static int action_ok_cursor_manager_list(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_CURSOR_MANAGER_LIST);
}

static int action_ok_compressed_archive_push(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_COMPRESSED_ARCHIVE_PUSH);
}

static int action_ok_compressed_archive_push_detect_core(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_COMPRESSED_ARCHIVE_PUSH_DETECT_CORE);
}

static int action_ok_configurations_list(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_CONFIGURATIONS_LIST);
}

static int action_ok_rdb_entry(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_RDB_ENTRY);
}

static int action_ok_content_collection_list(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_CONTENT_COLLECTION_LIST);
}

static int action_ok_core_list(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_CORE_LIST);
}

static int action_ok_cheat_file(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_CHEAT_FILE);
}

static int action_ok_playlist_collection(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_PLAYLIST_COLLECTION);
}

static int action_ok_disk_image_append_list(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_DISK_IMAGE_APPEND_LIST);
}

static int action_ok_record_configfile(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_RECORD_CONFIGFILE);
}

static int action_ok_remap_file(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_REMAP_FILE);
}

static int action_ok_push_content_list(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_CONTENT_LIST);
}

static int action_ok_push_downloads_dir(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_DOWNLOADS_DIR);
}

static int action_ok_shader_preset(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_SHADER_PRESET);
}

int action_ok_push_generic_list(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_GENERIC);
}

static int action_ok_push_default(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_PUSH_DEFAULT);
}

static int action_ok_audio_dsp_plugin(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_AUDIO_DSP_PLUGIN);
}

static int action_ok_rpl_entry(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, type, idx,
         entry_idx, ACTION_OK_DL_RPL_ENTRY);
}

static int  generic_action_ok_help(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx,
      unsigned id, menu_help_type_t id2)
{
   const char *lbl        = menu_hash_to_str(id);
   menu_handle_t            *menu    = menu_driver_get_ptr();
   menu->help_screen_type = id2;
   return generic_action_ok_displaylist_push(path, lbl, type, idx,
         entry_idx, ACTION_OK_DL_HELP);
}

static int action_ok_open_archive_detect_core(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, 0, 0, entry_idx,
         ACTION_OK_DL_OPEN_ARCHIVE_DETECT_CORE);
}

static int action_ok_open_archive(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_displaylist_push(path, label, 0, 0, entry_idx,
         ACTION_OK_DL_OPEN_ARCHIVE);
}

static int action_ok_load_archive(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   menu_handle_t *menu      = menu_driver_get_ptr();
   const char *menu_path    = menu ? menu->scratch2_buf : NULL;
   const char *content_path = menu ? menu->scratch_buf  : NULL;

   fill_pathname_join(detect_content_path, menu_path, content_path,
         sizeof(detect_content_path));

   event_command(EVENT_CMD_LOAD_CORE);
   menu_common_load_content(NULL, detect_content_path, false, CORE_TYPE_PLAIN);

   return 0;
}

static int action_ok_load_archive_detect_core(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   size_t selection;
   int ret                  = 0;
   global_t      *global    = global_get_ptr();
   menu_handle_t *menu      = menu_driver_get_ptr();
   menu_list_t *menu_list   = menu_list_get_ptr();
   const char *menu_path    = menu ? menu->scratch2_buf : NULL;
   const char *content_path = menu ? menu->scratch_buf  : NULL;

   if (!menu || !menu_list)
      return -1;

   if (!menu_navigation_ctl(MENU_NAVIGATION_CTL_GET_SELECTION, &selection))
      return false;

   ret = rarch_defer_core(global->core_info.list, menu_path, content_path, label,
         menu->deferred_path, sizeof(menu->deferred_path));

   fill_pathname_join(detect_content_path, menu_path, content_path,
         sizeof(detect_content_path));

   switch (ret)
   {
      case -1:
         event_command(EVENT_CMD_LOAD_CORE);
         return menu_common_load_content(NULL, NULL, false, CORE_TYPE_PLAIN);
      case 0:
         return generic_action_ok_displaylist_push(path, label, type,
               selection, entry_idx, ACTION_OK_DL_DEFERRED_CORE_LIST);
      default:
         break;
   }

   return ret;
}

static int action_ok_help_audio_video_troubleshooting(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_help(path, label, type, idx, entry_idx,
         MENU_LABEL_HELP_AUDIO_VIDEO_TROUBLESHOOTING, MENU_HELP_AUDIO_VIDEO_TROUBLESHOOTING);
}

static int action_ok_help(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_help(path, label, type, idx, entry_idx,
         MENU_LABEL_HELP, MENU_HELP_WELCOME);
}

static int action_ok_help_controls(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_help(path, label, type, idx, entry_idx,
         MENU_LABEL_HELP_CONTROLS, MENU_HELP_CONTROLS);
}

static int action_ok_help_what_is_a_core(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_help(path, label, type, idx, entry_idx,
         MENU_LABEL_HELP_WHAT_IS_A_CORE, MENU_HELP_WHAT_IS_A_CORE);
}

static int action_ok_help_scanning_content(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_help(path, label, type, idx, entry_idx,
         MENU_LABEL_HELP_SCANNING_CONTENT, MENU_HELP_SCANNING_CONTENT);
}

static int action_ok_help_change_virtual_gamepad(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_help(path, label, type, idx, entry_idx,
         MENU_LABEL_HELP_CHANGE_VIRTUAL_GAMEPAD, MENU_HELP_CHANGE_VIRTUAL_GAMEPAD);
}

static int action_ok_help_load_content(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   return generic_action_ok_help(path, label, type, idx, entry_idx,
         MENU_LABEL_HELP_LOADING_CONTENT, MENU_HELP_LOADING_CONTENT);
}

static int action_ok_video_resolution(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   global_t *global = global_get_ptr();

#ifdef __CELLOS_LV2__
   if (global->console.screen.resolutions.list[
         global->console.screen.resolutions.current.idx] ==
         CELL_VIDEO_OUT_RESOLUTION_576)
   {
      if (global->console.screen.pal_enable)
         global->console.screen.pal60_enable = true;
   }
   else
   {
      global->console.screen.pal_enable = false;
      global->console.screen.pal60_enable = false;
   }

   event_command(EVENT_CMD_REINIT);
#else
   unsigned width   = 0;
   unsigned  height = 0;

   if (video_driver_get_video_output_size(&width, &height))
   {
      char msg[PATH_MAX_LENGTH] = {0};

      video_driver_set_video_mode(width, height, true);
      global->console.screen.resolutions.width = width;
      global->console.screen.resolutions.height = height;

      snprintf(msg, sizeof(msg),"Applying: %dx%d\n START to reset",width, height);
      menu_display_msg_queue_push(msg, 1, 100, true);
   }
#endif

   return 0;
}

static int is_rdb_entry(uint32_t label_hash)
{
   switch (label_hash)
   {
      case MENU_LABEL_RDB_ENTRY_PUBLISHER:
      case MENU_LABEL_RDB_ENTRY_DEVELOPER:
      case MENU_LABEL_RDB_ENTRY_ORIGIN:
      case MENU_LABEL_RDB_ENTRY_FRANCHISE:
      case MENU_LABEL_RDB_ENTRY_ENHANCEMENT_HW:
      case MENU_LABEL_RDB_ENTRY_ESRB_RATING:
      case MENU_LABEL_RDB_ENTRY_BBFC_RATING:
      case MENU_LABEL_RDB_ENTRY_ELSPA_RATING:
      case MENU_LABEL_RDB_ENTRY_PEGI_RATING:
      case MENU_LABEL_RDB_ENTRY_CERO_RATING:
      case MENU_LABEL_RDB_ENTRY_EDGE_MAGAZINE_RATING:
      case MENU_LABEL_RDB_ENTRY_EDGE_MAGAZINE_ISSUE:
      case MENU_LABEL_RDB_ENTRY_FAMITSU_MAGAZINE_RATING:
      case MENU_LABEL_RDB_ENTRY_RELEASE_MONTH:
      case MENU_LABEL_RDB_ENTRY_RELEASE_YEAR:
      case MENU_LABEL_RDB_ENTRY_MAX_USERS:
         break;
      default:
         return -1;
   }

   return 0;
}

static int menu_cbs_init_bind_ok_compare_label(menu_file_list_cbs_t *cbs,
      const char *label, uint32_t hash, const char *elem0)
{
   uint32_t elem0_hash      = menu_hash_calculate(elem0);

   if (elem0[0] != '\0' && (is_rdb_entry(elem0_hash) == 0))
   {
      cbs->action_ok = action_ok_rdb_entry_submenu;
      return 0;
   }

   if (cbs->setting && cbs->setting->browser_selection_type == ST_DIR)
   {
      cbs->action_ok = action_ok_push_generic_list;
      return 0;
   }

   switch (hash)
   {
      case MENU_LABEL_OPEN_ARCHIVE_DETECT_CORE:
         cbs->action_ok = action_ok_open_archive_detect_core;
         break;
      case MENU_LABEL_OPEN_ARCHIVE:
         cbs->action_ok = action_ok_open_archive;
         break;
      case MENU_LABEL_LOAD_ARCHIVE_DETECT_CORE:
         cbs->action_ok = action_ok_load_archive_detect_core;
         break;
      case MENU_LABEL_LOAD_ARCHIVE:
         cbs->action_ok = action_ok_load_archive;
         break;
      case MENU_LABEL_CUSTOM_BIND_ALL:
         cbs->action_ok = action_ok_lookup_setting;
         break;
      case MENU_LABEL_SAVESTATE:
         cbs->action_ok = action_ok_save_state;
         break;
      case MENU_LABEL_LOADSTATE:
         cbs->action_ok = action_ok_load_state;
         break;
      case MENU_LABEL_RESUME_CONTENT:
         cbs->action_ok = action_ok_resume_content;
         break;
      case MENU_LABEL_RESTART_CONTENT:
         cbs->action_ok = action_ok_restart_content;
         break;
      case MENU_LABEL_TAKE_SCREENSHOT:
         cbs->action_ok = action_ok_screenshot;
         break;
      case MENU_LABEL_FILE_LOAD_OR_RESUME:
         cbs->action_ok = action_ok_file_load_or_resume;
         break;
      case MENU_LABEL_QUIT_RETROARCH:
         cbs->action_ok = action_ok_quit;
         break;
      case MENU_LABEL_CLOSE_CONTENT:
         cbs->action_ok = action_ok_close_content;
         break;
      case MENU_LABEL_SAVE_NEW_CONFIG:
         cbs->action_ok = action_ok_save_new_config;
         break;
      case MENU_LABEL_HELP:
         cbs->action_ok = action_ok_help;
         break;
      case MENU_LABEL_HELP_CONTROLS:
         cbs->action_ok = action_ok_help_controls;
         break;
      case MENU_LABEL_HELP_WHAT_IS_A_CORE:
         cbs->action_ok = action_ok_help_what_is_a_core;
         break;
      case MENU_LABEL_HELP_CHANGE_VIRTUAL_GAMEPAD:
         cbs->action_ok = action_ok_help_change_virtual_gamepad;
         break;
      case MENU_LABEL_HELP_AUDIO_VIDEO_TROUBLESHOOTING:
         cbs->action_ok = action_ok_help_audio_video_troubleshooting;
         break;
      case MENU_LABEL_HELP_SCANNING_CONTENT:
         cbs->action_ok = action_ok_help_scanning_content;
         break;
      case MENU_LABEL_HELP_LOADING_CONTENT:
         cbs->action_ok = action_ok_help_load_content;
         break;
      case MENU_LABEL_VIDEO_SHADER_PASS:
         cbs->action_ok = action_ok_shader_pass;
         break;
      case MENU_LABEL_VIDEO_SHADER_PRESET:
         cbs->action_ok = action_ok_shader_preset;
         break;
      case MENU_LABEL_CHEAT_FILE_LOAD:
         cbs->action_ok = action_ok_cheat_file;
         break;
      case MENU_LABEL_AUDIO_DSP_PLUGIN:
         cbs->action_ok = action_ok_audio_dsp_plugin;
         break;
      case MENU_LABEL_REMAP_FILE_LOAD:
         cbs->action_ok = action_ok_remap_file;
         break;
      case MENU_LABEL_RECORD_CONFIG:
         cbs->action_ok = action_ok_record_configfile;
         break;
#ifdef HAVE_NETWORKING
      case MENU_LABEL_DOWNLOAD_CORE_CONTENT:
         cbs->action_ok = action_ok_core_content_list;
         break;
      case MENU_LABEL_VALUE_CORE_UPDATER_LIST:
         cbs->action_ok = action_ok_core_updater_list;
         break;
#endif
      case MENU_LABEL_VIDEO_SHADER_PARAMETERS:
      case MENU_LABEL_VIDEO_SHADER_PRESET_PARAMETERS:
         cbs->action_ok = action_ok_shader_parameters;
         break;
      case MENU_LABEL_SHADER_OPTIONS:
      case MENU_VALUE_INPUT_SETTINGS:
      case MENU_LABEL_CORE_OPTIONS:
      case MENU_LABEL_CORE_CHEAT_OPTIONS:
      case MENU_LABEL_CORE_INPUT_REMAPPING_OPTIONS:
      case MENU_LABEL_CORE_INFORMATION:
      case MENU_LABEL_SYSTEM_INFORMATION:
      case MENU_LABEL_DEBUG_INFORMATION:
      case MENU_LABEL_DISK_OPTIONS:
      case MENU_LABEL_SETTINGS:
      case MENU_LABEL_FRONTEND_COUNTERS:
      case MENU_LABEL_CORE_COUNTERS:
      case MENU_LABEL_MANAGEMENT:
      case MENU_LABEL_ONLINE_UPDATER:
      case MENU_LABEL_LOAD_CONTENT_LIST:
      case MENU_LABEL_ADD_CONTENT_LIST:
      case MENU_LABEL_HELP_LIST:
      case MENU_LABEL_INFORMATION_LIST:
      case MENU_LABEL_CONTENT_SETTINGS:
         cbs->action_ok = action_ok_push_default;
         break;
      case MENU_LABEL_SCAN_FILE:
      case MENU_LABEL_SCAN_DIRECTORY:
      case MENU_LABEL_LOAD_CONTENT:
      case MENU_LABEL_DETECT_CORE_LIST:
         cbs->action_ok = action_ok_push_content_list;
         break;
      case MENU_LABEL_DOWNLOADED_FILE_DETECT_CORE_LIST:
         cbs->action_ok = action_ok_push_downloads_dir;
         break;
      case MENU_LABEL_DETECT_CORE_LIST_OK:
         cbs->action_ok = action_ok_file_load_detect_core;
         break;
      case MENU_LABEL_LOAD_CONTENT_HISTORY:
      case MENU_LABEL_CURSOR_MANAGER_LIST:
      case MENU_LABEL_DATABASE_MANAGER_LIST:
         cbs->action_ok = action_ok_push_generic_list;
         break;
      case MENU_LABEL_SHADER_APPLY_CHANGES:
         cbs->action_ok = action_ok_shader_apply_changes;
         break;
      case MENU_LABEL_CHEAT_APPLY_CHANGES:
         cbs->action_ok = action_ok_cheat_apply_changes;
         break;
      case MENU_LABEL_VIDEO_SHADER_PRESET_SAVE_AS:
         cbs->action_ok = action_ok_shader_preset_save_as;
         break;
      case MENU_LABEL_CHEAT_FILE_SAVE_AS:
         cbs->action_ok = action_ok_cheat_file_save_as;
         break;
      case MENU_LABEL_REMAP_FILE_SAVE_AS:
         cbs->action_ok = action_ok_remap_file_save_as;
         break;
      case MENU_LABEL_REMAP_FILE_SAVE_CORE:
         cbs->action_ok = action_ok_remap_file_save_core;
         break;
      case MENU_LABEL_REMAP_FILE_SAVE_GAME:
         cbs->action_ok = action_ok_remap_file_save_game;
         break;
      case MENU_LABEL_CONTENT_COLLECTION_LIST:
         cbs->action_ok = action_ok_content_collection_list;
         break;
      case MENU_LABEL_CORE_LIST:
         cbs->action_ok = action_ok_core_list;
         break;
      case MENU_LABEL_DISK_IMAGE_APPEND:
         cbs->action_ok = action_ok_disk_image_append_list;
         break;
      case MENU_LABEL_CONFIGURATIONS:
         cbs->action_ok = action_ok_configurations_list;
         break;
      case MENU_LABEL_SCREEN_RESOLUTION:
         cbs->action_ok = action_ok_video_resolution;
         break;
      case MENU_LABEL_UPDATE_ASSETS:
         cbs->action_ok = action_ok_update_assets;
         break;
      case MENU_LABEL_UPDATE_CORE_INFO_FILES:
         cbs->action_ok = action_ok_update_core_info_files;
         break;
      case MENU_LABEL_UPDATE_OVERLAYS:
         cbs->action_ok = action_ok_update_overlays;
         break;
      case MENU_LABEL_UPDATE_DATABASES:
         cbs->action_ok = action_ok_update_databases;
         break;
      case MENU_LABEL_UPDATE_GLSL_SHADERS:
         cbs->action_ok = action_ok_update_shaders_glsl;
         break;
      case MENU_LABEL_UPDATE_CG_SHADERS:
         cbs->action_ok = action_ok_update_shaders_cg;
         break;
      case MENU_LABEL_UPDATE_CHEATS:
         cbs->action_ok = action_ok_update_cheats;
         break;
      case MENU_LABEL_UPDATE_AUTOCONFIG_PROFILES:
         cbs->action_ok = action_ok_update_autoconfig_profiles;
         break;
      case MENU_LABEL_UPDATE_AUTOCONFIG_PROFILES_HID:
         cbs->action_ok = action_ok_update_autoconfig_profiles_hid;
         break;
      default:
         return -1;
   }

   return 0;
}

static int menu_cbs_init_bind_ok_compare_type(menu_file_list_cbs_t *cbs,
      uint32_t label_hash, uint32_t menu_label_hash, unsigned type)
{
   if (type == MENU_SETTINGS_CUSTOM_BIND_KEYBOARD ||
         type == MENU_SETTINGS_CUSTOM_BIND)
      cbs->action_ok = action_ok_lookup_setting;
   else if (type >= MENU_SETTINGS_SHADER_PARAMETER_0
         && type <= MENU_SETTINGS_SHADER_PARAMETER_LAST)
      cbs->action_ok = NULL;
   else if (type >= MENU_SETTINGS_SHADER_PRESET_PARAMETER_0
         && type <= MENU_SETTINGS_SHADER_PRESET_PARAMETER_LAST)
      cbs->action_ok = NULL;
   else if (type >= MENU_SETTINGS_CHEAT_BEGIN
         && type <= MENU_SETTINGS_CHEAT_END)
      cbs->action_ok = action_ok_cheat;
   else
   {
      switch (type)
      {
         case MENU_SETTING_ACTION_CORE_DISK_OPTIONS:
            cbs->action_ok = action_ok_push_default;
            break;
         case MENU_FILE_PLAYLIST_ENTRY:
            cbs->action_ok = action_ok_playlist_entry;
            break;
         case MENU_FILE_RPL_ENTRY:
            cbs->action_ok = action_ok_rpl_entry;
            break;
         case MENU_FILE_PLAYLIST_COLLECTION:
            cbs->action_ok = action_ok_playlist_collection;
            break;
         case MENU_FILE_CONTENTLIST_ENTRY:
            cbs->action_ok = action_ok_push_generic_list;
            break;
         case MENU_FILE_CHEAT:
            cbs->action_ok = action_ok_cheat_file_load;
            break;
         case MENU_FILE_RECORD_CONFIG:
            cbs->action_ok = action_ok_record_configfile_load;
            break;
         case MENU_FILE_REMAP:
            cbs->action_ok = action_ok_remap_file_load;
            break;
         case MENU_FILE_SHADER_PRESET:
            switch (menu_label_hash)
            {
               case MENU_LABEL_SCAN_FILE:
                  break;
               default:
                  cbs->action_ok = action_ok_shader_preset_load;
            }
            break;
         case MENU_FILE_SHADER:
            switch (menu_label_hash)
            {
               case MENU_LABEL_SCAN_FILE:
                  break;
               default:
                  cbs->action_ok = action_ok_shader_pass_load;
            }
            break;
         case MENU_FILE_IMAGE:
            switch (menu_label_hash)
            {
               case MENU_LABEL_SCAN_FILE:
                  break;
               default:
                  cbs->action_ok = action_ok_menu_wallpaper_load;
            }
            break;
         case MENU_FILE_USE_DIRECTORY:
            cbs->action_ok = action_ok_path_use_directory;
            break;
         case MENU_FILE_SCAN_DIRECTORY:
            cbs->action_ok = action_ok_path_scan_directory;
            break;
         case MENU_FILE_CONFIG:
            cbs->action_ok = action_ok_config_load;
            break;
         case MENU_FILE_DIRECTORY:
            cbs->action_ok = action_ok_directory_push;
            break;
         case MENU_FILE_CARCHIVE:
            switch (menu_label_hash)
            {
               case MENU_LABEL_DETECT_CORE_LIST:
                  cbs->action_ok = action_ok_compressed_archive_push_detect_core;
                  break;
               case MENU_LABEL_SCAN_FILE:
                  break;
               default:
                  cbs->action_ok = action_ok_compressed_archive_push;
                  break;
            }
            break;
         case MENU_FILE_CORE:
            switch (menu_label_hash)
            {
               case MENU_LABEL_DEFERRED_CORE_LIST:
                  cbs->action_ok = action_ok_core_load_deferred;
                  break;
               case MENU_LABEL_DEFERRED_CORE_LIST_SET:
                  cbs->action_ok = action_ok_core_deferred_set;
                  break;
               case MENU_LABEL_CORE_LIST:
                  cbs->action_ok = action_ok_core_load;
                  break;
               case MENU_LABEL_CORE_UPDATER_LIST:
                  cbs->action_ok = action_ok_deferred_list_stub;
                  break;
            }
            break;
         case MENU_FILE_DOWNLOAD_CORE_CONTENT:
            cbs->action_ok = action_ok_core_content_download;
            break;
         case MENU_FILE_DOWNLOAD_CORE:
            cbs->action_ok = action_ok_core_updater_download;
            break;
         case MENU_FILE_DOWNLOAD_CORE_INFO:
            break;
         case MENU_FILE_RDB:
            switch (menu_label_hash)
            {
               case MENU_LABEL_DEFERRED_DATABASE_MANAGER_LIST:
                  cbs->action_ok = action_ok_deferred_list_stub;
                  break;
               case MENU_LABEL_DATABASE_MANAGER_LIST:
               case MENU_VALUE_HORIZONTAL_MENU:
                  cbs->action_ok = action_ok_database_manager_list;
                  break;
            }
            break;
         case MENU_FILE_RDB_ENTRY:
            cbs->action_ok = action_ok_rdb_entry;
            break;
         case MENU_FILE_CURSOR:
            switch (menu_label_hash)
            {
               case MENU_LABEL_DEFERRED_DATABASE_MANAGER_LIST:
                  cbs->action_ok = action_ok_deferred_list_stub;
                  break;
               case MENU_LABEL_CURSOR_MANAGER_LIST:
                  cbs->action_ok = action_ok_cursor_manager_list;
                  break;
            }
            break;
         case MENU_FILE_FONT:
         case MENU_FILE_OVERLAY:
         case MENU_FILE_AUDIOFILTER:
         case MENU_FILE_VIDEOFILTER:
            cbs->action_ok = action_ok_set_path;
            break;
#ifdef HAVE_COMPRESSION
         case MENU_FILE_IN_CARCHIVE:
#endif
         case MENU_FILE_PLAIN:
            switch (menu_label_hash)
            {
               case MENU_LABEL_SCAN_FILE:
                  cbs->action_ok = action_ok_scan_file;
                  break;
               case MENU_LABEL_DOWNLOADED_FILE_DETECT_CORE_LIST:
               case MENU_LABEL_DETECT_CORE_LIST:
               case MENU_LABEL_DEFERRED_ARCHIVE_OPEN_DETECT_CORE:
#ifdef HAVE_COMPRESSION
                  if (type == MENU_FILE_IN_CARCHIVE)
                     cbs->action_ok = action_ok_file_load_with_detect_core_carchive;
                  else
#endif
                     cbs->action_ok = action_ok_file_load_with_detect_core;
                  break;
               case MENU_LABEL_DISK_IMAGE_APPEND:
                  cbs->action_ok = action_ok_disk_image_append;
                  break;
               default:
                  cbs->action_ok = action_ok_file_load;
                  break;
            }
            break;
         case MENU_FILE_MOVIE:
         case MENU_FILE_MUSIC:
#ifdef HAVE_FFMPEG
            switch (menu_label_hash)
            {
               case MENU_LABEL_SCAN_FILE:
                  break;
               default:
                  cbs->action_ok = action_ok_file_load_ffmpeg;
            }
#endif
            break;
         case MENU_FILE_IMAGEVIEWER:
            switch (menu_label_hash)
            {
               case MENU_LABEL_SCAN_FILE:
                  break;
               default:
                  cbs->action_ok = action_ok_file_load_imageviewer;
            }
            break;
         case MENU_SETTINGS:
         case MENU_SETTING_GROUP:
         case MENU_SETTING_SUBGROUP:
            cbs->action_ok = action_ok_push_default;
            break;
         case MENU_SETTINGS_CORE_DISK_OPTIONS_DISK_CYCLE_TRAY_STATUS:
            cbs->action_ok = action_ok_disk_cycle_tray_status;
            break;
         default:
            return -1;
      }
   }

   return 0;
}

int menu_cbs_init_bind_ok(menu_file_list_cbs_t *cbs,
      const char *path, const char *label, unsigned type, size_t idx,
      const char *elem0, const char *elem1, const char *menu_label,
      uint32_t label_hash, uint32_t menu_label_hash)
{
   if (!cbs)
      return -1;

   cbs->action_ok = action_ok_lookup_setting;

   if (menu_cbs_init_bind_ok_compare_label(cbs, label, label_hash, elem0) == 0)
      return 0;

   if (menu_cbs_init_bind_ok_compare_type(cbs, label_hash, menu_label_hash, type) == 0)
      return 0;

   return -1;
}
