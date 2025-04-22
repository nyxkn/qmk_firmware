/* Copyright 2022 Thomas Baart <thomas@splitkb.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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

#pragma once

#ifdef RGBLIGHT_ENABLE
#    define RGBLIGHT_EFFECT_BREATHING
#    define RGBLIGHT_EFFECT_RAINBOW_MOOD
#    define RGBLIGHT_EFFECT_RAINBOW_SWIRL
#    define RGBLIGHT_EFFECT_SNAKE
#    define RGBLIGHT_EFFECT_KNIGHT
#    define RGBLIGHT_EFFECT_CHRISTMAS
#    define RGBLIGHT_EFFECT_STATIC_GRADIENT
#    define RGBLIGHT_EFFECT_RGB_TEST
#    define RGBLIGHT_EFFECT_ALTERNATING
#    define RGBLIGHT_EFFECT_TWINKLE
#    define RGBLIGHT_HUE_STEP 8
#    define RGBLIGHT_SAT_STEP 8
#    define RGBLIGHT_VAL_STEP 8
#    ifndef RGBLIGHT_LIMIT_VAL
#        define RGBLIGHT_LIMIT_VAL 150
#    endif
#endif

// ========================================
// nyxkn

// space saving
#undef LOCKING_SUPPORT_ENABLE
#undef LOCKING_RESYNC_ENABLE
#define NO_MUSIC_MODE
/* #define LAYER_STATE_8BIT */
#define LAYER_STATE_16BIT

#define RGBLIGHT_SLEEP

/* #define ONESHOT_TAP_TOGGLE 0 */
/* #define TAPPING_TOGGLE 2 */

// in case you pressed and forgot about it
#define ONESHOT_TIMEOUT 5000

// < tap, > hold
/* #define TAPPING_TERM 200 */
#define TAPPING_TERM 180

// hold after tap forces hold function (replacing keyrepeat)
/* #define TAPPING_FORCE_HOLD */

// holding mod and tapping another key activates mod even if faster than tapping_term
// requires careful typing or you'll find accidental mod activations
/* #define PERMISSIVE_HOLD */

// releasing mod without having pressed anything else sends tap keycode
// can be problematic with mods if you want to just cancel the hold, or when using with mouse
/* #define RETRO_TAPPING */

// autoshift is problematic for hjkl holding
#define AUTO_SHIFT_TIMEOUT 175

#define NO_AUTO_SHIFT_SPECIAL
#define NO_AUTO_SHIFT_NUMERIC

// allow shifted key to be repeated
/* #define AUTO_SHIFT_REPEAT */
/* #define AUTO_SHIFT_NO_AUTO_REPEAT */

// on a mod-tap, retro shift produces shifted version of tap keycode when released without other presses
// if mod-tap is held longer than value, don't send tap at all (allows cancel or usage with mouse)
// doesn't seem to work?
#define RETRO_SHIFT
/* #define RETRO_SHIFT 500 */

#define UNICODE_SELECTED_MODES UNICODE_MODE_LINUX
