/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
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

#ifndef _MENU_INPUT_H
#define _MENU_INPUT_H

#include "../input/input_driver.h"
#include "../input/keyboard_line.h"
#include "../libretro.h"

#ifdef __cplusplus
extern "C" {
#endif

enum menu_action
{
   MENU_ACTION_NOOP = 0,
   MENU_ACTION_UP,
   MENU_ACTION_DOWN,
   MENU_ACTION_LEFT,
   MENU_ACTION_RIGHT,
   MENU_ACTION_OK,
   MENU_ACTION_SEARCH,
   MENU_ACTION_SCAN,
   MENU_ACTION_CANCEL,
   MENU_ACTION_INFO,
   MENU_ACTION_SELECT,
   MENU_ACTION_START,
   MENU_ACTION_SCROLL_DOWN,
   MENU_ACTION_SCROLL_UP,
   MENU_ACTION_TOGGLE
};
 
enum menu_input_pointer_state
{
   MENU_POINTER_X_AXIS = 0,
   MENU_POINTER_Y_AXIS,
   MENU_POINTER_DELTA_X_AXIS,
   MENU_POINTER_DELTA_Y_AXIS
};

enum menu_input_mouse_state
{
   MENU_MOUSE_X_AXIS = 0,
   MENU_MOUSE_Y_AXIS
};

enum menu_input_ctl_state
{
   MENU_INPUT_CTL_MOUSE_SCROLL_DOWN = 0,
   MENU_INPUT_CTL_MOUSE_SCROLL_UP,
   MENU_INPUT_CTL_MOUSE_PTR,
   MENU_INPUT_CTL_POINTER_PTR,
   MENU_INPUT_CTL_POINTER_ACCEL_READ,
   MENU_INPUT_CTL_POINTER_ACCEL_WRITE,
   MENU_INPUT_CTL_POINTER_DRAGGING,
   MENU_INPUT_CTL_KEYBOARD_DISPLAY,
   MENU_INPUT_CTL_SET_KEYBOARD_DISPLAY,
   MENU_INPUT_CTL_KEYBOARD_BUFF_PTR,
   MENU_INPUT_CTL_KEYBOARD_LABEL,
   MENU_INPUT_CTL_SET_KEYBOARD_LABEL,
   MENU_INPUT_CTL_UNSET_KEYBOARD_LABEL,
   MENU_INPUT_CTL_KEYBOARD_LABEL_SETTING,
   MENU_INPUT_CTL_SET_KEYBOARD_LABEL_SETTING,
   MENU_INPUT_CTL_UNSET_KEYBOARD_LABEL_SETTING,
   MENU_INPUT_CTL_SEARCH_START
};

enum mouse_action
{
   MOUSE_ACTION_NONE = 0,
   MOUSE_ACTION_BUTTON_L,
   MOUSE_ACTION_BUTTON_L_TOGGLE,
   MOUSE_ACTION_BUTTON_L_SET_NAVIGATION,
   MOUSE_ACTION_BUTTON_R,
   MOUSE_ACTION_WHEEL_UP,
   MOUSE_ACTION_WHEEL_DOWN
};

enum menu_input_bind_mode
{
   MENU_INPUT_BIND_NONE,
   MENU_INPUT_BIND_SINGLE,
   MENU_INPUT_BIND_ALL
};

void menu_input_key_event(bool down, unsigned keycode, uint32_t character,
      uint16_t key_modifiers);

void menu_input_key_start_line(const char *label,
      const char *label_setting, unsigned type, unsigned idx,
      input_keyboard_line_complete_t cb);

void menu_input_st_uint_callback(void *userdata, const char *str);
void menu_input_st_hex_callback(void *userdata, const char *str);

void menu_input_st_string_callback(void *userdata, const char *str);

void menu_input_st_cheat_callback(void *userdata, const char *str);

int menu_input_bind_iterate(char *s, size_t len);

unsigned menu_input_frame(retro_input_t input, retro_input_t trigger_state);

void menu_input_post_iterate(int *ret, unsigned action);

int menu_input_set_keyboard_bind_mode(void *data, enum menu_input_bind_mode type);

int menu_input_set_input_device_bind_mode(void *data, enum menu_input_bind_mode type);

int16_t menu_input_pointer_state(enum menu_input_pointer_state state);

int16_t menu_input_mouse_state(enum menu_input_mouse_state state);

bool menu_input_ctl(enum menu_input_ctl_state state, void *data);

void menu_input_set_binds_minmax(unsigned min, unsigned max);

void menu_input_free(void);

#ifdef __cplusplus
}
#endif

#endif
