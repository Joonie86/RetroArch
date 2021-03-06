/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2015 - Daniel De Matteis
 *  Copyright (C) 2012-2015 - Michael Lelli
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

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include <string/string_list.h>
#include <compat/posix_string.h>
#include <file/file_path.h>
#include <retro_inline.h>

#include "menu_generic.h"

#include "../../general.h"

#include "../menu.h"
#include "../menu_animation.h"
#include "../menu_display.h"
#include "../menu_hash.h"
#include "../menu_video.h"

#include "../../gfx/drivers_font_renderer/bitmap.h"

#define RGUI_TERM_START_X(width)        (width / 21)
#define RGUI_TERM_START_Y(height)       (height / 9)
#define RGUI_TERM_WIDTH(width)          (((width - RGUI_TERM_START_X(width) - RGUI_TERM_START_X(width)) / (FONT_WIDTH_STRIDE)))
#define RGUI_TERM_HEIGHT(width, height) (((height - RGUI_TERM_START_Y(height) - RGUI_TERM_START_X(width)) / (FONT_HEIGHT_STRIDE)) - 1)

#if defined(GEKKO)|| defined(PSP)
#define HOVER_COLOR(settings)    ((3 << 0) | (10 << 4) | (3 << 8) | (7 << 12))
#define NORMAL_COLOR(settings)   0x7FFF
#define TITLE_COLOR(settings)    HOVER_COLOR(settings)
#else
#define HOVER_COLOR(settings)    (argb32_to_rgba4444(settings->menu.entry_hover_color))
#define NORMAL_COLOR(settings)   (argb32_to_rgba4444(settings->menu.entry_normal_color))
#define TITLE_COLOR(settings)    (argb32_to_rgba4444(settings->menu.title_color))
#endif

typedef struct
{
   bool force_redraw;
   char msgbox[4096];
   unsigned last_width;
   unsigned last_height;
} rgui_t;

static INLINE uint16_t argb32_to_rgba4444(uint32_t col)
{
   unsigned a = ((col >> 24) & 0xff) >> 4;
   unsigned r = ((col >> 16) & 0xff) >> 4;
   unsigned g = ((col >> 8) & 0xff)  >> 4;
   unsigned b =  (col & 0xff)        >> 4;
   return (r << 12) | (g << 8) | (b << 4) | a;
}

static void rgui_copy_glyph(uint8_t *glyph, const uint8_t *buf)
{
   int x, y;

   if (!glyph)
      return;

   for (y = 0; y < FONT_HEIGHT; y++)
   {
      for (x = 0; x < FONT_WIDTH; x++)
      {
         uint32_t col =
            ((uint32_t)buf[3 * (-y * 256 + x) + 0] << 0) |
            ((uint32_t)buf[3 * (-y * 256 + x) + 1] << 8) |
            ((uint32_t)buf[3 * (-y * 256 + x) + 2] << 16);

         uint8_t rem     = 1 << ((x + y * FONT_WIDTH) & 7);
         unsigned offset = (x + y * FONT_WIDTH) >> 3;

         if (col != 0xff)
            glyph[offset] |= rem;
      }
   }
}

static uint16_t gray_filler(unsigned x, unsigned y)
{
   unsigned col;

   x   >>= 1;
   y   >>= 1;
   col   = ((x + y) & 1) + 1;

#if defined(GEKKO) || defined(PSP)
   return (6 << 12) | (col << 8) | (col << 4) | (col << 0);
#else
   return (col << 13) | (col << 9) | (col << 5) | (12 << 0);
#endif
}

static uint16_t green_filler(unsigned x, unsigned y)
{
   unsigned col;

   x   >>= 1;
   y   >>= 1;
   col   = ((x + y) & 1) + 1;
#if defined(GEKKO) || defined(PSP)
   return (6 << 12) | (col << 8) | (col << 5) | (col << 0);
#else
   return (col << 13) | (col << 10) | (col << 5) | (12 << 0);
#endif
}

static void fill_rect(uint16_t *data, size_t pitch,
      unsigned x, unsigned y,
      unsigned width, unsigned height,
      uint16_t (*col)(unsigned x, unsigned y))
{
   unsigned i, j;

   if (!data || !col)
      return;

   for (j = y; j < y + height; j++)
      for (i = x; i < x + width; i++)
         data[j * (pitch >> 1) + i] = col(i, j);
}

static void color_rect(
      uint16_t *data, size_t pitch,
      unsigned fb_width, unsigned fb_height,
      unsigned x, unsigned y,
      unsigned width, unsigned height,
      uint16_t color)
{
   unsigned i, j;

   if (!data)
      return;

   for (j = y; j < y + height; j++)
      for (i = x; i < x + width; i++)
         if (i < fb_width && j < fb_height)
            data[j * (pitch >> 1) + i] = color;
}

static uint8_t string_walkbyte(const char **string)
{
   return *((*string)++);
}

#ifdef HAVE_UTF8
/* Does not validate the input, returns garbage if it's not UTF-8. */
static uint32_t string_walk(const char **string)
{
   uint8_t first = string_walkbyte(string);
   uint32_t ret;
   
   if (first<128)
      return first;
   
   ret = 0;
   ret = (ret<<6) | (string_walkbyte(string)    & 0x3F);
   if (first >= 0xE0)
      ret = (ret<<6) | (string_walkbyte(string) & 0x3F);
   if (first >= 0xF0)
      ret = (ret<<6) | (string_walkbyte(string) & 0x3F);
   
   if (first >= 0xF0)
      return ret | (first&31)<<18;
   if (first >= 0xE0)
      return ret | (first&15)<<12;
   return ret | (first&7)<<6;
}
#else
#define string_walk string_walkbyte
#endif

static void blit_line(uint16_t *data,
      size_t pitch, int x, int y,
      const char *message, uint16_t color)
{
   unsigned i, j;

   while (*message)
   {
      uint8_t *font_fb;
      uint32_t symbol   = string_walk(&message);

      menu_display_ctl(MENU_DISPLAY_CTL_FONT_FB, &font_fb);
      
      for (j = 0; j < FONT_HEIGHT; j++)
      {
         for (i = 0; i < FONT_WIDTH; i++)
         {
            uint8_t rem = 1 << ((i + j * FONT_WIDTH) & 7);
            int offset  = (i + j * FONT_WIDTH) >> 3;
            bool col    = (font_fb[FONT_OFFSET(symbol) + offset] & rem);

            if (!col)
               continue;

            data[(y + j) * (pitch >> 1) + (x + i)] = color;
         }
      }

      x += FONT_WIDTH_STRIDE;
   }
}

static bool init_font(menu_handle_t *menu, const uint8_t *font_bmp_buf)
{
   unsigned i;
   bool fb_font_inited  = true;
   uint8_t        *font = (uint8_t *)calloc(1, FONT_OFFSET(256));

   if (!font)
      return false;

   menu_display_ctl(MENU_DISPLAY_CTL_SET_FONT_DATA_INIT, &fb_font_inited);

   for (i = 0; i < 256; i++)
   {
      unsigned y = i / 16;
      unsigned x = i % 16;
      rgui_copy_glyph(&font[FONT_OFFSET(i)],
            font_bmp_buf + 54 + 3 * (256 * (255 - 16 * y) + 16 * x));
   }

   menu_display_ctl(MENU_DISPLAY_CTL_SET_FONT_FB, &font);

   return true;
}

static bool rguidisp_init_font(menu_handle_t *menu)
{
   const uint8_t *font_bmp_buf = NULL;
   const uint8_t *font_bin_buf = bitmap_bin;

   if (!menu)
      return false;

   if (font_bmp_buf)
      return init_font(menu, font_bmp_buf);

   if (!font_bin_buf)
      return false;

   menu_display_ctl(MENU_DISPLAY_CTL_SET_FONT_FB, &font_bin_buf);

   return true;
}

static void rgui_render_background(void)
{
   size_t pitch_in_pixels, size, fb_pitch;
   unsigned fb_width, fb_height;
   uint16_t         *fb_data  = NULL;
   uint16_t             *src  = NULL;
   uint16_t             *dst  = NULL;
   menu_handle_t        *menu = menu_driver_get_ptr();
   if (!menu)
      return;

   menu_display_ctl(MENU_DISPLAY_CTL_WIDTH,    &fb_width);
   menu_display_ctl(MENU_DISPLAY_CTL_HEIGHT,   &fb_height);
   menu_display_ctl(MENU_DISPLAY_CTL_FB_DATA,  &fb_data);
   menu_display_ctl(MENU_DISPLAY_CTL_FB_PITCH, &fb_pitch);

   pitch_in_pixels = fb_pitch >> 1;
   size            = fb_pitch * 4;
   src             = fb_data + pitch_in_pixels * fb_height;
   dst             = fb_data;

   while (dst < src)
   {
      memcpy(dst, src, size);
      dst += pitch_in_pixels * 4;
   }

   fill_rect(fb_data, fb_pitch, 5, 5, fb_width - 10, 5, green_filler);
   fill_rect(fb_data, fb_pitch, 5, fb_height - 10, fb_width - 10, 5, green_filler);

   fill_rect(fb_data, fb_pitch, 5, 5, 5, fb_height - 10, green_filler);
   fill_rect(fb_data, fb_pitch, fb_width - 10, 5, 5, fb_height - 10,
         green_filler);
}

static void rgui_set_message(const char *message)
{
   menu_handle_t    *menu = menu_driver_get_ptr();
   rgui_t           *rgui = NULL;

   if (!menu || !menu->userdata || !message || !*message)
      return;

   rgui = (rgui_t*)menu->userdata;
   strlcpy(rgui->msgbox, message, sizeof(rgui->msgbox));
   rgui->force_redraw = true;
}

static void rgui_render_messagebox(const char *message)
{
   size_t i, fb_pitch;
   int x, y;
   unsigned fb_width, fb_height;
   unsigned width, glyphs_width, height;
   uint16_t color;
   uint16_t *fb_data          = NULL;
   struct string_list *list   = NULL;
   menu_handle_t *menu        = menu_driver_get_ptr();
   settings_t *settings       = config_get_ptr();

   if (!menu || !message || !*message)
      return;

   (void)settings;

   list = string_split(message, "\n");
   if (!list)
      return;
   if (list->elems == 0)
      goto end;

   width        = 0;
   glyphs_width = 0;

   menu_display_ctl(MENU_DISPLAY_CTL_WIDTH,  &fb_width);
   menu_display_ctl(MENU_DISPLAY_CTL_HEIGHT, &fb_height);
   menu_display_ctl(MENU_DISPLAY_CTL_FB_DATA,  &fb_data);
   menu_display_ctl(MENU_DISPLAY_CTL_FB_PITCH, &fb_pitch);

   for (i = 0; i < list->size; i++)
   {
      unsigned line_width;
      char     *msg   = list->elems[i].data;
      unsigned msglen = strlen(msg);

      if (msglen > RGUI_TERM_WIDTH(fb_width))
      {
         msg[RGUI_TERM_WIDTH(fb_width) - 2] = '.';
         msg[RGUI_TERM_WIDTH(fb_width) - 1] = '.';
         msg[RGUI_TERM_WIDTH(fb_width) - 0] = '.';
         msg[RGUI_TERM_WIDTH(fb_width) + 1] = '\0';
         msglen = RGUI_TERM_WIDTH(fb_width);
      }

      line_width   = msglen * FONT_WIDTH_STRIDE - 1 + 6 + 10;
      width        = max(width, line_width);
      glyphs_width = max(glyphs_width, msglen);
   }

   height = FONT_HEIGHT_STRIDE * list->size + 6 + 10;
   x      = (fb_width  - width) / 2;
   y      = (fb_height - height) / 2;

   fill_rect(fb_data, fb_pitch, x + 5, y + 5, width - 10,
         height - 10, gray_filler);
   fill_rect(fb_data, fb_pitch, x, y, width - 5, 5, green_filler);
   fill_rect(fb_data, fb_pitch, x + width - 5, y, 5,
         height - 5, green_filler);
   fill_rect(fb_data, fb_pitch, x + 5, y + height - 5,
         width - 5, 5, green_filler);
   fill_rect(fb_data, fb_pitch, x, y + 5, 5,
         height - 5, green_filler);

   color = NORMAL_COLOR(settings);

   for (i = 0; i < list->size; i++)
   {
      const char *msg = list->elems[i].data;
      int offset_x    = FONT_WIDTH_STRIDE * (glyphs_width - strlen(msg)) / 2;
      int offset_y    = FONT_HEIGHT_STRIDE * i;

      blit_line(fb_data, fb_pitch,
            x + 8 + offset_x, y + 8 + offset_y, msg, color);
   }

end:
   string_list_free(list);
}

static void rgui_blit_cursor(menu_handle_t *menu)
{
   size_t   fb_pitch;
   unsigned fb_width, fb_height;
   uint16_t *fb_data = NULL;
   int16_t        x = menu_input_mouse_state(MENU_MOUSE_X_AXIS);
   int16_t        y = menu_input_mouse_state(MENU_MOUSE_Y_AXIS);

   menu_display_ctl(MENU_DISPLAY_CTL_WIDTH,  &fb_width);
   menu_display_ctl(MENU_DISPLAY_CTL_HEIGHT, &fb_height);
   menu_display_ctl(MENU_DISPLAY_CTL_FB_DATA,  &fb_data);
   menu_display_ctl(MENU_DISPLAY_CTL_FB_PITCH, &fb_pitch);

   color_rect(fb_data, fb_pitch, fb_width, fb_height, x, y - 5, 1, 11, 0xFFFF);
   color_rect(fb_data, fb_pitch, fb_width, fb_height, x - 5, y, 11, 1, 0xFFFF);
}

static void rgui_render(void)
{
   unsigned x, y;
   bool display_kb, msg_force;
   uint16_t hover_color, normal_color;
   size_t i, end, fb_pitch;
   unsigned fb_width, fb_height;
   int bottom;
   char title[256], title_buf[256], title_msg[64];
   char msg[PATH_MAX_LENGTH], timedate[PATH_MAX_LENGTH];
   uint16_t *fb_data              = NULL;
   rgui_t *rgui                   = NULL;
   menu_handle_t *menu            = menu_driver_get_ptr();
   driver_t *driver               = driver_get_ptr();
   settings_t *settings           = config_get_ptr();
   uint64_t *frame_count          = video_driver_get_frame_count();

   msg[0]       = '\0';
   title[0]     = '\0';
   title_buf[0] = '\0';
   title_msg[0] = '\0';
   timedate[0]  = '\0';

   (void)driver;

   if (!menu || !menu->userdata)
      return;

   rgui = (rgui_t*)menu->userdata;

   if (!rgui->force_redraw)
   {
      bool is_idle;

      menu_display_ctl(MENU_DISPLAY_CTL_MSG_FORCE, &msg_force);

      if (menu_entries_needs_refresh() && menu_driver_alive() && !msg_force)
         return;

      rarch_main_ctl(RARCH_MAIN_CTL_IS_IDLE, &is_idle);
            
      if (is_idle)
         return;

      if (!menu_display_ctl(MENU_DISPLAY_CTL_UPDATE_PENDING, NULL))
         return;
   }

   menu_display_ctl(MENU_DISPLAY_CTL_WIDTH,  &fb_width);
   menu_display_ctl(MENU_DISPLAY_CTL_HEIGHT, &fb_height);
   menu_display_ctl(MENU_DISPLAY_CTL_FB_DATA,  &fb_data);
   menu_display_ctl(MENU_DISPLAY_CTL_FB_PITCH, &fb_pitch);

   /* if the framebuffer changed size, recache the background */
   if (rgui->last_width != fb_width || rgui->last_height != fb_height)
   {
      fill_rect(fb_data, fb_pitch, 0, fb_height, fb_width, 4, gray_filler);
      rgui->last_width  = fb_width;
      rgui->last_height = fb_height;
   }

   menu_display_ctl(MENU_DISPLAY_CTL_SET_FRAMEBUFFER_DIRTY_FLAG, NULL);
   menu_animation_ctl(MENU_ANIMATION_CTL_CLEAR_ACTIVE, NULL);

   rgui->force_redraw        = false;


   if (settings->menu.pointer.enable)
   {
      bool pointer_dragged = false;
      unsigned new_val = menu_input_pointer_state(MENU_POINTER_Y_AXIS)
         / 11 - 2 + menu_entries_get_start();

      menu_input_ctl(MENU_INPUT_CTL_POINTER_PTR, &new_val);
      menu_input_ctl(MENU_INPUT_CTL_POINTER_DRAGGING, &pointer_dragged);

      if (pointer_dragged)
      {
         int16_t delta_y = menu_input_pointer_state(MENU_POINTER_DELTA_Y_AXIS);
         menu->scroll_y += delta_y;
         menu_entries_set_start(-menu->scroll_y / 11 + 2);
         if (menu->scroll_y > 0)
            menu->scroll_y = 0;
      }
   }

   if (settings->menu.mouse.enable)
   {
      unsigned new_mouse_ptr;
      bool mouse_scrolldown, mouse_scrollup;
      int16_t mouse_y = menu_input_mouse_state(MENU_MOUSE_Y_AXIS);

      menu_input_ctl(MENU_INPUT_CTL_MOUSE_SCROLL_DOWN, &mouse_scrolldown);
      menu_input_ctl(MENU_INPUT_CTL_MOUSE_SCROLL_UP,   &mouse_scrollup);

      if (mouse_scrolldown
            && (menu_entries_get_start() < menu_entries_get_end() - RGUI_TERM_HEIGHT(fb_width, fb_height)))
         menu_entries_set_start(menu_entries_get_start() + 1);

      if (mouse_scrollup && (menu_entries_get_start() > 0))
         menu_entries_set_start(menu_entries_get_start() - 1);

      new_mouse_ptr = mouse_y / 11 - 2 + menu_entries_get_start();

      menu_input_ctl(MENU_INPUT_CTL_MOUSE_PTR, &new_mouse_ptr);
   }

   /* Do not scroll if all items are visible. */
   if (menu_entries_get_end() <= RGUI_TERM_HEIGHT(fb_width, fb_height))
      menu_entries_set_start(0);;

   bottom = menu_entries_get_end() - RGUI_TERM_HEIGHT(fb_width, fb_height);
   if (menu_entries_get_start() > (unsigned)bottom)
      menu_entries_set_start(bottom);

   end = ((menu_entries_get_start() + RGUI_TERM_HEIGHT(fb_width, fb_height)) <= (menu_entries_get_end())) ?
      menu_entries_get_start() + RGUI_TERM_HEIGHT(fb_width, fb_height) : menu_entries_get_end();

   rgui_render_background();

#if 0
   RARCH_LOG("Dir is: %s\n", label);
#endif

   menu_entries_get_title(title, sizeof(title));

   menu_animation_ticker_str(title_buf, RGUI_TERM_WIDTH(fb_width) - 10,
         *frame_count / RGUI_TERM_START_X(fb_width), title, true);

   hover_color  = HOVER_COLOR(settings);
   normal_color = NORMAL_COLOR(settings);

   if (menu_entries_show_back())
      blit_line(fb_data, fb_pitch,
            RGUI_TERM_START_X(fb_width),
            RGUI_TERM_START_X(fb_width),
            menu_hash_to_str(MENU_VALUE_BACK),
            TITLE_COLOR(settings));

   blit_line(fb_data, fb_pitch,
         RGUI_TERM_START_X(fb_width) + (RGUI_TERM_WIDTH(fb_width)
            - strlen(title_buf)) * FONT_WIDTH_STRIDE / 2,
         RGUI_TERM_START_X(fb_width),
         title_buf, TITLE_COLOR(settings));

   if (menu_entries_get_core_title(title_msg, sizeof(title_msg)) == 0)
      blit_line(fb_data, fb_pitch,
            RGUI_TERM_START_X(fb_width),
            (RGUI_TERM_HEIGHT(fb_width, fb_height) * FONT_HEIGHT_STRIDE) +
            RGUI_TERM_START_Y(fb_height) + 2, title_msg, hover_color);

   if (settings->menu.timedate_enable)
   {
      menu_display_timedate(timedate, sizeof(timedate), 3);

      blit_line(fb_data, fb_pitch,
            RGUI_TERM_WIDTH(fb_width) * FONT_WIDTH_STRIDE - RGUI_TERM_START_X(fb_width),
            (RGUI_TERM_HEIGHT(fb_width, fb_height) * FONT_HEIGHT_STRIDE) +
            RGUI_TERM_START_Y(fb_height) + 2, timedate, hover_color);
   }

   x = RGUI_TERM_START_X(fb_width);
   y = RGUI_TERM_START_Y(fb_height);
   i = menu_entries_get_start();

   for (; i < end; i++, y += FONT_HEIGHT_STRIDE)
   {
      size_t selection;
      char entry_path[PATH_MAX_LENGTH];
      char entry_value[PATH_MAX_LENGTH];
      char message[PATH_MAX_LENGTH];
      char entry_title_buf[PATH_MAX_LENGTH];
      char type_str_buf[PATH_MAX_LENGTH];
      unsigned entry_spacing = menu_entry_get_spacing(i);
      bool entry_selected    = menu_entry_is_currently_selected(i);
      
      if (!menu_navigation_ctl(MENU_NAVIGATION_CTL_GET_SELECTION, &selection))
         continue;

      if (i > (selection + 100))
         continue;

      entry_path[0]      = '\0';
      entry_value[0]     = '\0';
      message[0]         = '\0';
      entry_title_buf[0] = '\0';
      type_str_buf[0]    = '\0';

      menu_entry_get_value(i, entry_value, sizeof(entry_value));
      menu_entry_get_path(i, entry_path, sizeof(entry_path));

      menu_animation_ticker_str(entry_title_buf,
            RGUI_TERM_WIDTH(fb_width) - (entry_spacing + 1 + 2),
            *frame_count / RGUI_TERM_START_X(fb_width),
            entry_path,
            entry_selected);
      menu_animation_ticker_str(type_str_buf, entry_spacing,
            *frame_count / RGUI_TERM_START_X(fb_width),
            entry_value, entry_selected);

      snprintf(message, sizeof(message), "%c %-*.*s %-*s",
            entry_selected ? '>' : ' ',
            RGUI_TERM_WIDTH(fb_width) - (entry_spacing + 1 + 2),
            RGUI_TERM_WIDTH(fb_width) - (entry_spacing + 1 + 2),
            entry_title_buf,
            entry_spacing,
            type_str_buf);

      blit_line(fb_data, fb_pitch, x, y, message,
            entry_selected ? hover_color : normal_color);
   }

#ifdef GEKKO
   const char *message_queue;

   if (msg_force)
   {
      msg_force     = false;
      message_queue = rarch_main_msg_queue_pull();

      menu_display_ctl(MENU_DISPLAY_CTL_SET_MSG_FORCE, &msg_force);
   }
   else
      message_queue = driver->current_msg;

   rgui_render_messagebox( message_queue);
#endif

   menu_input_ctl(MENU_INPUT_CTL_KEYBOARD_DISPLAY, &display_kb);

   if (display_kb)
   {
      const char *str = NULL, *label = NULL;
      menu_input_ctl(MENU_INPUT_CTL_KEYBOARD_BUFF_PTR, &str);
      menu_input_ctl(MENU_INPUT_CTL_KEYBOARD_LABEL,    &label);

      if (!str)
         str = "";
      snprintf(msg, sizeof(msg), "%s\n%s", label, str);
      rgui_render_messagebox(msg);
   }

   if (rgui->msgbox[0] != '\0')
   {
      rgui_render_messagebox(rgui->msgbox);
      rgui->msgbox[0] = '\0';
      rgui->force_redraw = true;
   }

   if (settings->menu.mouse.enable)
      rgui_blit_cursor(menu);
}

static void *rgui_init(void)
{
   size_t fb_pitch;
   unsigned fb_width, fb_height, new_font_height;
   uint16_t        *fb_data   = NULL;
   rgui_t               *rgui = NULL;
   bool                   ret = false;
   menu_handle_t        *menu = (menu_handle_t*)calloc(1, sizeof(*menu));

   if (!menu)
      return NULL;

   menu->userdata = rgui = (rgui_t*)calloc(1, sizeof(rgui_t));

   if (!rgui)
      goto error;

   /* 4 extra lines to cache  the checked background */
   fb_data = (uint16_t*)calloc(400 * (240 + 4), sizeof(uint16_t));

   if (!fb_data)
      goto error;

   fb_width                   = 320;
   fb_height                  = 240;
   fb_pitch                   = fb_width * sizeof(uint16_t);
   new_font_height            = FONT_HEIGHT_STRIDE * 2;

   menu_display_ctl(MENU_DISPLAY_CTL_SET_WIDTH,         &fb_width);
   menu_display_ctl(MENU_DISPLAY_CTL_SET_HEIGHT,        &fb_height);
   menu_display_ctl(MENU_DISPLAY_CTL_SET_FB_DATA,       fb_data);
   menu_display_ctl(MENU_DISPLAY_CTL_SET_HEADER_HEIGHT, &new_font_height);
   menu_display_ctl(MENU_DISPLAY_CTL_SET_FB_PITCH,      &fb_pitch);

   menu_entries_set_start(0);

   ret = rguidisp_init_font(menu);

   if (!ret)
      goto error;

   fill_rect(fb_data, fb_pitch, 0, fb_height,
         fb_width, 4, gray_filler);

   rgui->last_width  = fb_width;
   rgui->last_height = fb_height;

   return menu;

error:
   if (menu)
   {
      if (fb_data)
         free(fb_data);
      fb_data = NULL;
      menu_display_ctl(MENU_DISPLAY_CTL_SET_FB_DATA, &fb_data);
      if (menu->userdata)
         free(menu->userdata);
      menu->userdata = NULL;
      free(menu);
   }
   return NULL;
}

static void rgui_free(void *data)
{
   uint8_t *font_fb;
   bool fb_font_inited   = false;
   menu_handle_t  *menu  = (menu_handle_t*)data;

   if (!menu)
      return;

   if (menu->userdata)
      free(menu->userdata);
   menu->userdata = NULL;

   menu_display_ctl(MENU_DISPLAY_CTL_FONT_DATA_INIT, &fb_font_inited);
   menu_display_ctl(MENU_DISPLAY_CTL_FONT_FB, &font_fb);

   if (fb_font_inited)
      free(font_fb);

   fb_font_inited = false;

   menu_display_ctl(MENU_DISPLAY_CTL_SET_FONT_DATA_INIT, &fb_font_inited);
}

static void rgui_set_texture(void)
{
   unsigned fb_width, fb_height;
   uint16_t *fb_data          = NULL;
   bool              fb_dirty = false;
   menu_handle_t        *menu = menu_driver_get_ptr();

   if (!menu)
      return;

   menu_display_ctl(MENU_DISPLAY_CTL_GET_FRAMEBUFFER_DIRTY_FLAG, &fb_dirty);

   if (!fb_dirty)
      return;

   menu_display_ctl(MENU_DISPLAY_CTL_WIDTH, &fb_width);
   menu_display_ctl(MENU_DISPLAY_CTL_HEIGHT, &fb_height);
   menu_display_ctl(MENU_DISPLAY_CTL_FB_DATA, &fb_data);
   menu_display_ctl(MENU_DISPLAY_CTL_UNSET_FRAMEBUFFER_DIRTY_FLAG, NULL);

   video_driver_set_texture_frame(fb_data, false, fb_width, fb_height, 1.0f);
}

static void rgui_navigation_clear(bool pending_push)
{
   menu_handle_t *menu = menu_driver_get_ptr();
   if (!menu)
      return;

   menu_entries_set_start(0);
   menu->scroll_y = 0;
}

static void rgui_navigation_set(bool scroll)
{
   size_t selection;
   unsigned fb_width, fb_height;
   menu_handle_t *menu            = menu_driver_get_ptr();
   size_t end                     = menu_entries_get_end();
   if (!menu_navigation_ctl(MENU_NAVIGATION_CTL_GET_SELECTION, &selection))
      return;
   if (!menu || !scroll)
      return;

   menu_display_ctl(MENU_DISPLAY_CTL_WIDTH,  &fb_width);
   menu_display_ctl(MENU_DISPLAY_CTL_HEIGHT, &fb_height);

   if (selection < RGUI_TERM_HEIGHT(fb_width, fb_height) /2)
      menu_entries_set_start(0);
   else if (selection >= (RGUI_TERM_HEIGHT(fb_width, fb_height) /2)
         && selection < (end - RGUI_TERM_HEIGHT(fb_width, fb_height) /2))
      menu_entries_set_start(selection - RGUI_TERM_HEIGHT(fb_width, fb_height) /2);
   else if (selection >= (end - RGUI_TERM_HEIGHT(fb_width, fb_height) /2))
      menu_entries_set_start(end - RGUI_TERM_HEIGHT(fb_width, fb_height));
}

static void rgui_navigation_set_last(void)
{
   menu_handle_t *menu = menu_driver_get_ptr();
   if (menu)
      rgui_navigation_set(true);
}

static void rgui_navigation_descend_alphabet(size_t *unused)
{
   menu_handle_t *menu = menu_driver_get_ptr();
   if (menu)
      rgui_navigation_set(true);
}

static void rgui_navigation_ascend_alphabet(size_t *unused)
{
   menu_handle_t *menu = menu_driver_get_ptr();
   if (menu)
      rgui_navigation_set(true);
}

static void rgui_populate_entries(const char *path,
      const char *label, unsigned k)
{
   menu_handle_t *menu = menu_driver_get_ptr();
   if (menu)
      rgui_navigation_set(true);
}

static int rgui_environ(menu_environ_cb_t type, void *data)
{
   switch (type)
   {
      case 0:
         break;
      default:
         return -1;
   }

   return 0;
}

menu_ctx_driver_t menu_ctx_rgui = {
   rgui_set_texture,
   rgui_set_message,
   generic_menu_iterate,
   rgui_render,
   NULL,
   rgui_init,
   rgui_free,
   NULL,
   NULL,
   rgui_populate_entries,
   NULL,
   rgui_navigation_clear,
   NULL,
   NULL,
   rgui_navigation_set,
   rgui_navigation_set_last,
   rgui_navigation_descend_alphabet,
   rgui_navigation_ascend_alphabet,
   generic_menu_init_list,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   "rgui",
   MENU_VIDEO_DRIVER_GENERIC,
   rgui_environ,
};
