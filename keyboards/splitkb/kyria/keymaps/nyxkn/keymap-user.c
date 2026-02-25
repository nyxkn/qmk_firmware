/* Copyright 2019 Thomas Baart <thomas@splitkb.com>
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

#include QMK_KEYBOARD_H

#include "action_layer.h"
#include "process_tap_dance.h"
#include "quantum.h"

#ifdef RAW_ENABLE
#    include "raw_hid.h"
#    include <string.h>
#endif

#ifdef CONSOLE_ENABLE
#    include "print.h"
#endif

enum layers {
    _COLEMAK = 0,
    _QWERTY,
    _DEFAULT_DVORAK,
    /* _DEFAULT_COLEMAK_DH, */
    _DEFAULT_NAV,
    _DEFAULT_SYM,
    _DEFAULT_FUNCTION,
    _ADJUST,
    _GAME,
    _ARROWS,
    _NUM,
    _FUN,
    _SYM,
    _NAV,
    _WASD,
    _NUM_LH,
};

// custom keycodes
/* enum my_keycodes { */
/*     LAYER_LOCK = SAFE_RANGE, */
/* }; */

// save space
#ifndef MAGIC_ENABLE
uint16_t keycode_config(uint16_t keycode) {
    return keycode;
}
#endif

#ifndef MAGIC_ENABLE
uint8_t mod_config(uint8_t mod) {
    return mod;
}
#endif

// ================================================================================
// RAW HID
// ================================================================================
void raw_hid_receive(uint8_t *data, uint8_t length) {
    // Your code goes here
    // `data` is a pointer to the buffer containing the received HID report
    // `length` is the length of the report - always `RAW_EPSIZE`

    // prepare response
    uint8_t response[length];
    memset(response, 0, length);
    response[0] = 'X';
    /* strcpy((char*)response, "test"); */

    // if the last character is a null terminator, we're correctly dealing with a string
    if (data[length - 1] == '\0') {
        if (strcmp((char *)data, "to_layer_game") == 0) {
            layer_move(_GAME);
            raw_hid_send(response, length);
        } else if (strcmp((char *)data, "to_layer_default") == 0) {
            layer_move(_COLEMAK);
            raw_hid_send(response, length);
        } else if (strcmp((char *)data, "to_layer_numpad_lh") == 0) {
            layer_move(_NUM_LH);
            raw_hid_send(response, length);
        } else {
            // Handle unknown command
        }
    } else {
        // error? or process as other type of data
        // Handle error: not null-terminated
        /* printf("Error: Missing null terminator\n"); */
        return;
    }

    /* if (data[0] == 'g') { */
    /*     layer_move(_GAME); */
    /*     raw_hid_send(response, length); */
    /* } */
    /* if (data[0] == 'd') { */
    /*     layer_move(_COLEMAK); */
    /*     raw_hid_send(response, length); */
    /* } */

    // hmm, why can we not match this successfully?
    /* const char* target = "game"; */
    /* if (strncmp((char*)data, target, strlen(target)) == 0) { */
    /*     strcpy((char*)response, "ok"); */
    /* } */
}

// ================================================================================
// TAP DANCE CORE
// ================================================================================

typedef enum {
    TD_NONE,
    TD_UNKNOWN,
    TD_SINGLE_TAP,
    TD_SINGLE_HOLD,
    TD_DOUBLE_TAP,
    TD_DOUBLE_HOLD,
    TD_DOUBLE_SINGLE_TAP, // Send two single taps
    TD_TRIPLE_TAP,
    TD_TRIPLE_HOLD
} td_status_t;

typedef struct {
    bool        is_press_action;
    td_status_t state;
} td_tap_t;

td_status_t cur_dance(tap_dance_state_t *state);

/* Return an integer that corresponds to what kind of tap dance should be executed.
 *
 * How to figure out tap dance state: interrupted and pressed.
 *
 * Interrupted: If the state of a dance is "interrupted", that means that another key has been hit
 *  under the tapping term. This is typically indicitive that you are trying to "tap" the key.
 *
 * Pressed: Whether or not the key is still being pressed. If this value is true, that means the tapping term
 *  has ended, but the key is still being pressed down. This generally means the key is being "held".
 *
 * One thing that is currenlty not possible with qmk software in regards to tap dance is to mimic the "permissive hold"
 *  feature. In general, advanced tap dances do not work well if they are used with commonly typed letters.
 *  For example "A". Tap dances are best used on non-letter keys that are not hit while typing letters.
 *
 * Good places to put an advanced tap dance:
 *  z,q,x,j,k,v,b, any function key, home/end, comma, semi-colon
 *
 * Criteria for "good placement" of a tap dance key:
 *  Not a key that is hit frequently in a sentence
 *  Not a key that is used frequently to double tap, for example 'tab' is often double tapped in a terminal, or
 *    in a web form. So 'tab' would be a poor choice for a tap dance.
 *  Letters used in common words as a double. For example 'p' in 'pepper'. If a tap dance function existed on the
 *    letter 'p', the word 'pepper' would be quite frustating to type.
 *
 * For the third point, there does exist the 'TD_DOUBLE_SINGLE_TAP', however this is not fully tested
 *
 */
td_status_t cur_dance(tap_dance_state_t *state) {
    if (state->count == 1) {
        if (state->interrupted || !state->pressed) return TD_SINGLE_TAP;
        // Key has not been interrupted, but the key is still held. Means you want to send a 'HOLD'.
        else
            return TD_SINGLE_HOLD;
    } else if (state->count == 2) {
        // TD_DOUBLE_SINGLE_TAP is to distinguish between typing "pepper", and actually wanting a double tap
        // action when hitting 'pp'. Suggested use case for this return value is when you want to send two
        // keystrokes of the key, and not the 'double tap' action/macro.
        if (state->interrupted)
            return TD_DOUBLE_SINGLE_TAP;
        else if (state->pressed)
            return TD_DOUBLE_HOLD;
        else
            return TD_DOUBLE_TAP;
    }

    // Assumes no one is trying to type the same letter three times (at least not quickly).
    // If your tap dance key is 'KC_W', and you want to type "www." quickly - then you will need to add
    // an exception here to return a 'TD_TRIPLE_SINGLE_TAP', and define that enum just like 'TD_DOUBLE_SINGLE_TAP'
    if (state->count == 3) {
        if (state->interrupted || !state->pressed)
            return TD_TRIPLE_TAP;
        else
            return TD_TRIPLE_HOLD;
    } else
        return TD_UNKNOWN;
}

// Simplified version with only single/double tap and hold
// for debugging, but actually this is all we need anyway
/* td_status_t cur_dance(tap_dance_state_t *state) { */
/*     if (state->count == 1) { */
/*         if (state->interrupted || !state->pressed) return TD_SINGLE_TAP; */
/*         else return TD_SINGLE_HOLD; */
/*     } */

/*     if (state->count == 2) return TD_DOUBLE_SINGLE_TAP; */
/*     else return TD_UNKNOWN; // Any number higher than the maximum state value you return above */
/* } */

// fast dance version. tap is triggered by user process on key release instead of waiting until term expire
td_status_t fast_dance_status(tap_dance_state_t *state) {
    // dance finished - term expired or key interrupted - key being held
    // single tap has already been handled by user process, and reset will be called skipping finished
    if (state->pressed) {
        if (state->count == 1) {
            // this case forces the hold to only happen if not interrupted
            // meaning that if interrupted, we won't send a hold and a tap will be sent at term end by reset
            // comment this if you want hold to happen even if interrupted - like in permissive hold
            /* if (!state->interrupted) { */
            /*     return TD_SINGLE_HOLD; */
            /* } */

            return TD_SINGLE_HOLD;
        } else if (state->count == 2) {
            return TD_DOUBLE_HOLD;
        } else if (state->count == 3) {
            return TD_TRIPLE_HOLD;
        }
    }

    return TD_UNKNOWN;
}

typedef struct {
    /* bool is_press_action; */
    td_status_t status;
    uint16_t    tap_kc;
    /* tap_dance_state_t state; */
} td_user_data_t;

#define ACTION_TAP_DANCE_FN_ADVANCED_NYX(user_fn_on_each_tap, user_fn_on_dance_finished, user_fn_on_dance_reset, tap_kc) \
    {                                                                                                                    \
        .fn        = {user_fn_on_each_tap, user_fn_on_dance_finished, user_fn_on_dance_reset},                           \
        .user_data = (void *)&((td_user_data_t){TD_NONE, tap_kc}),                                                       \
    }

// tap hold version. to have a different key on tap and on hold, taking repeat into account for both
// e.g. colon on tap and semicolon on hold

typedef struct {
    uint16_t tap;
    uint16_t hold;
    uint16_t held;
} tap_dance_tap_hold_t;

void tap_dance_tap_hold_finished(tap_dance_state_t *state, void *user_data) {
    tap_dance_tap_hold_t *tap_hold = (tap_dance_tap_hold_t *)user_data;

    if (state->pressed) {
        if (state->count == 1
#ifndef PERMISSIVE_HOLD
            && !state->interrupted
#endif
        ) {
            register_code16(tap_hold->hold);
            tap_hold->held = tap_hold->hold;
        } else {
            register_code16(tap_hold->tap);
            tap_hold->held = tap_hold->tap;
        }
    }
}

void tap_dance_tap_hold_reset(tap_dance_state_t *state, void *user_data) {
    tap_dance_tap_hold_t *tap_hold = (tap_dance_tap_hold_t *)user_data;

    if (tap_hold->held) {
        unregister_code16(tap_hold->held);
        tap_hold->held = 0;
    }
}

#define ACTION_TAP_DANCE_TAP_HOLD(tap, hold)                                        \
    {                                                                               \
        .fn        = {NULL, tap_dance_tap_hold_finished, tap_dance_tap_hold_reset}, \
        .user_data = (void *)&((tap_dance_tap_hold_t){tap, hold, 0}),               \
    }

// ================================================================================
// TAP DANCE FUNCTIONS
// ================================================================================

// we have two types of tap dance
// if you want a snappy tap response on key release, then you can only intercept a tap, hold and tap+hold
// if you need multiple taps, then the first tap necessarily has to wait for tapping_term to expire

// Tap dance enums
enum {
    CT_CLN,
    TD_TL3,
    TD_TL2,
    TD_TR2,
    TD_TR3,
    /* TD_A, */
};

// declaring here is optional, but makes it so that we have a nice overview of things in here
/* void td_tl3_finished(tap_dance_state_t *state, void *user_data); */
/* void td_tl3_reset(tap_dance_state_t *state, void *user_data); */
/* static td_status_t td_tl3_state; */

/* void td_tr2_finished(tap_dance_state_t *state, void *user_data); */
/* void td_tr2_reset(tap_dance_state_t *state, void *user_data); */
/* static td_status_t td_tr2_state; */

void td_tl3_finished(tap_dance_state_t *state, void *user_data);
void td_tl3_reset(tap_dance_state_t *state, void *user_data);

void td_tl2_finished(tap_dance_state_t *state, void *user_data);
void td_tl2_reset(tap_dance_state_t *state, void *user_data);

void td_tr2_finished(tap_dance_state_t *state, void *user_data);
void td_tr2_reset(tap_dance_state_t *state, void *user_data);

void td_tr3_finished(tap_dance_state_t *state, void *user_data);
void td_tr3_reset(tap_dance_state_t *state, void *user_data);

// if you don't declare functions above, you have to move this down below the functions
tap_dance_action_t tap_dance_actions[] = {
    /* [TD_TL3] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, td_tl3_finished, td_tl3_reset), */
    /* [TD_TR2] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, td_tr2_finished, td_tr2_reset), */
    /* [CT_CLN] = ACTION_TAP_DANCE_TAP_HOLD(KC_COLN, KC_SCLN), */
    [TD_TL3] = ACTION_TAP_DANCE_FN_ADVANCED_NYX(NULL, td_tl3_finished, td_tl3_reset, KC_SPC),
    /* [TD_TL2] = ACTION_TAP_DANCE_FN_ADVANCED_NYX(NULL, td_tl2_finished, td_tl2_reset, RCTL(KC_BSPC)), */
    [TD_TL2] = ACTION_TAP_DANCE_FN_ADVANCED_NYX(NULL, td_tl2_finished, td_tl2_reset, KC_BSPC),
    [TD_TR2] = ACTION_TAP_DANCE_FN_ADVANCED_NYX(NULL, td_tr2_finished, td_tr2_reset, KC_ESC),
    [TD_TR3] = ACTION_TAP_DANCE_FN_ADVANCED_NYX(NULL, td_tr3_finished, td_tr3_reset, KC_SPC),
};

// ================================================================================

/* void td_tl3_finished(tap_dance_state_t *state, void *user_data) { */
/*     td_tl3_state = cur_dance(state); */
/*     switch (td_tl3_state) { */
/*         case TD_SINGLE_TAP: */
/*             /\* set_oneshot_layer(_NAV, ONESHOT_START); *\/ */
/*             tap_code(KC_SPC); */
/*             break; */
/*         case TD_SINGLE_HOLD: */
/*             layer_on(_NAV); */
/*             break; */
/*         case TD_DOUBLE_TAP: */
/*             layer_on(_NAV); */
/*             break; */
/*         default: */
/*             break; */
/*     } */
/* } */

/* void td_tl3_reset(tap_dance_state_t *state, void *user_data) { */
/*     switch (td_tl3_state) { */
/*         case TD_SINGLE_TAP: */
/*             /\* clear_oneshot_layer_state(ONESHOT_PRESSED); *\/ */
/*             break; */
/*         case TD_SINGLE_HOLD: */
/*             layer_off(_NAV); */
/*             break; */
/*         case TD_DOUBLE_TAP: */
/*             break; */
/*         default: */
/*             break; */
/*     } */
/*     td_tl3_state = TD_NONE; */
/* } */

// ================================================================================

/* void td_tr2_finished(tap_dance_state_t *state, void *user_data) { */
/*     td_tr2_state = cur_dance(state); */
/*     switch (td_tr2_state) { */
/*         case TD_SINGLE_TAP: */
/*             set_oneshot_layer(_SYM, ONESHOT_START); */
/*             break; */
/*         case TD_SINGLE_HOLD: */
/*             layer_on(_SYM); */
/*             break; */
/*         case TD_DOUBLE_TAP: */
/*             layer_on(_SYM); */
/*             break; */
/*         default: */
/*             break; */
/*     } */
/* } */

/* void td_tr2_reset(tap_dance_state_t *state, void *user_data) { */
/*     switch (td_tr2_state) { */
/*         case TD_SINGLE_TAP: */
/*             clear_oneshot_layer_state(ONESHOT_PRESSED); */
/*             break; */
/*         case TD_SINGLE_HOLD: */
/*             layer_off(_SYM); */
/*             break; */
/*         case TD_DOUBLE_TAP: */
/*             break; */
/*         default: */
/*             break; */
/*     } */
/*     td_tr2_state = TD_NONE; */
/* } */

// ================================================================================

// tap is necessarily a basic keycode
// hold can be a keycode, a layer activation, or a modifier hold
void td_tl3_finished(tap_dance_state_t *state, void *user_data) {
    td_user_data_t *td_user_data = (td_user_data_t *)user_data;
    td_user_data->status         = fast_dance_status(state);

    switch (td_user_data->status) {
        case TD_SINGLE_HOLD:
            layer_on(_NAV);
            break;
        default:
            break;
    }
}

void td_tl3_reset(tap_dance_state_t *state, void *user_data) {
    td_user_data_t *td_user_data = (td_user_data_t *)user_data;

    switch (td_user_data->status) {
        case TD_SINGLE_TAP:
            tap_code16(td_user_data->tap_kc);
            break;
        case TD_SINGLE_HOLD:
            layer_off(_NAV);
            break;
        default:
            break;
    }
    td_user_data->status = TD_NONE;
}

// ================================================================================

void td_tl2_finished(tap_dance_state_t *state, void *user_data) {
    td_user_data_t *td_user_data = (td_user_data_t *)user_data;
    td_user_data->status         = fast_dance_status(state);

    switch (td_user_data->status) {
        case TD_SINGLE_HOLD:
            layer_on(_NUM);
            break;
        default:
            break;
    }
}

void td_tl2_reset(tap_dance_state_t *state, void *user_data) {
    td_user_data_t *td_user_data = (td_user_data_t *)user_data;

    switch (td_user_data->status) {
        case TD_SINGLE_TAP:
            tap_code16(td_user_data->tap_kc);
            break;
        case TD_SINGLE_HOLD:
            layer_off(_NUM);
            break;
        default:
            break;
    }
    td_user_data->status = TD_NONE;
}

// ================================================================================

void td_tr2_finished(tap_dance_state_t *state, void *user_data) {
    td_user_data_t *td_user_data = (td_user_data_t *)user_data;
    td_user_data->status         = fast_dance_status(state);

    switch (td_user_data->status) {
        case TD_SINGLE_HOLD:
            layer_on(_SYM);
            break;
        default:
            break;
    }
}

void td_tr2_reset(tap_dance_state_t *state, void *user_data) {
    td_user_data_t *td_user_data = (td_user_data_t *)user_data;

    switch (td_user_data->status) {
        case TD_SINGLE_TAP:
            tap_code16(td_user_data->tap_kc);
            break;
        case TD_SINGLE_HOLD:
            layer_off(_SYM);
            break;
        default:
            break;
    }
    td_user_data->status = TD_NONE;
}

// ================================================================================

void td_tr3_finished(tap_dance_state_t *state, void *user_data) {
    td_user_data_t *td_user_data = (td_user_data_t *)user_data;
    td_user_data->status         = fast_dance_status(state);

    switch (td_user_data->status) {
        case TD_SINGLE_HOLD:
            register_mods(MOD_BIT(KC_RSFT));
            break;
        default:
            break;
    }
}

void td_tr3_reset(tap_dance_state_t *state, void *user_data) {
    td_user_data_t *td_user_data = (td_user_data_t *)user_data;

    switch (td_user_data->status) {
        case TD_SINGLE_TAP:
            tap_code16(td_user_data->tap_kc);
            break;
        case TD_SINGLE_HOLD:
            unregister_mods(MOD_BIT(KC_RSFT));
            break;
        default:
            break;
    }
    td_user_data->status = TD_NONE;
}

// ================================================================================
// PROCESS
// ================================================================================

/* bool get_custom_auto_shifted_key(uint16_t keycode, keyrecord_t *record) { */
/*     return false; */
/*     // enables autoshift on all mod-tap and layer tap keys */
/*     /\* if (IS_RETRO(keycode)) { *\/ */
/*     /\*     return true; *\/ */
/*     /\* } *\/ */
/*     /\* return false; *\/ */

/*     /\* switch (keycode) { *\/ */
/*     /\*     case KC_DOT: *\/ */
/*     /\*         return true; *\/ */
/*     /\*     default: *\/ */
/*     /\*         return false; *\/ */
/*     /\* } *\/ */
/* } */

/* bool get_auto_shifted_key(uint16_t keycode, keyrecord_t *record) { */
/*     if (IS_RETRO(keycode)) { */
/*         return true; */
/*     } */
/*     switch (keycode) { */
/* #    ifndef NO_AUTO_SHIFT_ALPHA */
/*         case KC_A ... KC_Z: */
/* #    endif */
/* #    ifndef NO_AUTO_SHIFT_NUMERIC */
/*         case KC_1 ... KC_0: */
/* #    endif */
/* #    ifndef NO_AUTO_SHIFT_SPECIAL */
/*         case KC_TAB: */
/*         case KC_MINUS ... KC_SLASH: */
/*         case KC_NONUS_BACKSLASH: */
/* #    endif */
/*             return true; */
/*     } */
/*     return get_custom_auto_shifted_key(keycode, record); */
/* } */

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    tap_dance_action_t *action;
    tap_dance_state_t  *state;

#ifdef CONSOLE_ENABLE
    uprintf("KL: kc: 0x%04X, col: %2u, row: %2u, pressed: %u, time: %5u, int: %u, count: %u\n", keycode, record->event.key.col, record->event.key.row, record->event.pressed, record->event.time, record->tap.interrupted, record->tap.count);
#endif

    switch (keycode) {
            /* case EXAMPLE1: */
            /*     if (record->event.pressed) { */
            /*         // Do something when pressed */
            /*     } else { */
            /*         // Do something else when release */
            /*     } */
            /*     return false; // Skip all further processing of this key */

            /* case EXAMPLE2: */
            /*     // Play a tone when enter is pressed */
            /*     if (record->event.pressed) { */
            /*         PLAY_SONG(tone_qwerty); */
            /*     } */
            /*     return true; // Let QMK send the enter press/release events */

        case KC_TRNS:
        case KC_NO:
            /* Always cancel one-shot layer when a non-layer key gets pressed (KC_NO or KC_TRNS) */
            if (record->event.pressed && is_oneshot_layer_active()) {
                clear_oneshot_layer_state(ONESHOT_OTHER_KEY_PRESSED);
            }
            return true;

        /* case QK_BOOT: */
        /*     /\* Don't allow firmware reset from oneshot layer state *\/ */
        /*     if (record->event.pressed && is_oneshot_layer_active()) { */
        /*         clear_oneshot_layer_state(ONESHOT_OTHER_KEY_PRESSED); */
        /*         return false; */
        /*     } */
        /*     return true; */

        /* case TD(CT_CLN):  // list all tap dance keycodes with tap-hold configurations */
        /*     action = &tap_dance_actions[TD_INDEX(keycode)]; */
        /*     if (!record->event.pressed && action->state.count && !action->state.finished) { */
        /*         tap_dance_tap_hold_t *tap_hold = (tap_dance_tap_hold_t *)action->user_data; */
        /*         tap_code16(tap_hold->tap); */
        /*         // this sends a tap but still allows subsequent hold */
        /*         // this allows for repeating of the tap key with tap+hold */
        /*         // if we prevent hold then repeating of the tap key becomes impossible */
        /*     } */
        /*     return true; */

        // all dances that make use of fast dance
        // this gets called on key release. happens before dance finish or reset
        case TD(TD_TL3):
        case TD(TD_TL2):
        case TD(TD_TR2):
        case TD(TD_TR3):
            // somehow tap_dance_get doesn't exit? but docs use it
            // action = tap_dance_get(QK_TAP_DANCE_GET_INDEX(keycode));
            action = &tap_dance_actions[TD_INDEX(keycode)];
            state  = tap_dance_get_state(QK_TAP_DANCE_GET_INDEX(keycode));

            if (!record->event.pressed && state->count && !state->finished) {
                td_user_data_t *td_user_data = (td_user_data_t *)action->user_data;

                // !! these next two blocks are mutually exclusive

                // choose whether you want to end dance with tap
                {
                    td_user_data->status = TD_SINGLE_TAP;
                    /* reset_tap_dance(&(action->state)); */
                    reset_tap_dance(state);
                }

                // or if you want to take repeating into account
                /* { */
                /*     // send kc here and let dance continue onto the holds */
                /*     tap_code16(td_user_data->tap_kc); */
                /* } */
            }
            return true;

        default:
            return true; // Process all other keycodes normally
    }
}

// default implementation
// underscore seems to always break the caps. but we can use minus instead which we set to become shifted
bool caps_word_press_user(uint16_t keycode) {
    /* uprintf("kc: 0x%04X \n", keycode); */
    /* uprintf("num: 0x%04X \n", (TAP_DANCE | TD_TL2)); */

    switch (keycode) {
        // Keycodes that continue Caps Word, with shift applied.
        case KC_A ... KC_Z:
            /* case KC_MINS: */
            add_weak_mods(MOD_BIT(KC_LSFT)); // Apply shift to next key.
            return true;

        // Keycodes that continue Caps Word, without shifting.
        case KC_1 ... KC_0:
        case KC_BSPC:
        case KC_DEL:
        case KC_UNDS:
        case QK_TAP_DANCE | TD_TL2:
        case QK_TAP_DANCE | TD_TR2:
            return true;

        // anything else deactivates caps word
        default:
            return false; // Deactivate Caps Word.
    }
}

// ================================================================================
// KEY OVERRIDE
// ================================================================================

/* const key_override_t delete_key_override = ko_make_basic(MOD_MASK_SHIFT, KC_BSPACE, KC_DELETE); */
const key_override_t comma_key_override = ko_make_basic(MOD_MASK_SHIFT, KC_COMMA, KC_QUESTION);
const key_override_t dot_key_override   = ko_make_basic(MOD_MASK_SHIFT, KC_DOT, KC_EXCLAIM);
const key_override_t colon_key_override = ko_make_basic(MOD_MASK_SHIFT, KC_COLON, KC_SEMICOLON);

// This globally defines all key overrides to be used
const key_override_t *key_overrides[] = {
    &comma_key_override,
    &dot_key_override,
    &colon_key_override,
};

/* const key_override_t **key_overrides = (const key_override_t *[]){ */
/*     &comma_key_override, &dot_key_override, &colon_key_override, */
/*     NULL // Null terminate the array of overrides! */
/* }; */

// ================================================================================
// OLED
// ================================================================================

/* The default OLED and rotary encoder code can be found at the bottom of qmk_firmware/keyboards/splitkb/kyria/rev1/rev1.c
 * These default settings can be overriden by your own settings in your keymap.c
 * For your convenience, here's a copy of those settings so that you can uncomment them if you wish to apply your own modifications.
 * DO NOT edit the rev1.c file; instead override the weakly defined default functions by your own.
 */

#ifdef OLED_ENABLE
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_180;
}
bool oled_task_user(void) {
    if (is_keyboard_master()) {
        /* // QMK Logo and version information */
        /* // clang-format off */
        /* static const char PROGMEM qmk_logo[] = { */
        /*     0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94, */
        /*     0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4, */
        /*     0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,0}; */
        /* // clang-format on */
        /* oled_write_P(qmk_logo, false); */
        /* oled_write_P(PSTR("Kyria rev1.0\n\n"), false); */

        // Host Keyboard Layer Status
        int layer_number = get_highest_layer(layer_state | default_layer_state);

        oled_write_P(PSTR("Layer: "), false);
        switch (layer_number) {
            /* case _DEFAULT_QWERTY: */
            /*     oled_write_P(PSTR("QWERTY\n"), false); */
            /*     break; */
            case _DEFAULT_DVORAK:
                oled_write_P(PSTR("Dvorak\n"), false);
                break;
            /* case _DEFALT_COLEMAK_DH: */
            /*     oled_write_P(PSTR("Colemak-DH\n"), false); */
            /*     break; */
            case _DEFAULT_NAV:
                oled_write_P(PSTR("Nav\n"), false);
                break;
            case _DEFAULT_SYM:
                oled_write_P(PSTR("Sym\n"), false);
                break;
            case _DEFAULT_FUNCTION:
                oled_write_P(PSTR("Function\n"), false);
                break;
            case _ADJUST:
                oled_write_P(PSTR("Adjust\n"), false);
                break;

            // nyxkn
            case _COLEMAK:
                oled_write_ln_P(PSTR("Colemak"), false);
                break;
            case _QWERTY:
                oled_write_ln_P(PSTR("Qwerty"), false);
                break;
            case _GAME:
                oled_write_ln_P(PSTR("Game"), false);
                break;
            case _ARROWS:
                oled_write_ln_P(PSTR("Arrows"), false);
                break;
            case _NUM:
                oled_write_ln_P(PSTR("Numpad"), false);
                break;
            case _FUN:
                oled_write_ln_P(PSTR("Functions"), false);
                break;
            case _SYM:
                oled_write_ln_P(PSTR("Symbols"), false);
                break;
            case _NAV:
                oled_write_ln_P(PSTR("Navigation"), false);
                break;
            case _WASD:
                oled_write_ln_P(PSTR("Wasd"), false);
                break;
            case _NUM_LH:
                oled_write_ln_P(PSTR("LH Numpad"), false);
                break;

            default:
                oled_write_P(PSTR("Undefined\n"), false);
        }

        oled_write_ln_P(PSTR(""), false);
        oled_write_P(get_mods() & MOD_MASK_CTRL ? PSTR("CTRL ") : PSTR("     "), false);
        oled_write_P(get_mods() & MOD_MASK_SHIFT ? PSTR("SHFT ") : PSTR("     "), false);
        oled_write_P(get_mods() & MOD_MASK_GUI ? PSTR("GUI ") : PSTR("    "), false);
        oled_write_P(get_mods() & MOD_MASK_ALT ? PSTR("ALT ") : PSTR("    "), false);

        oled_write_ln_P(PSTR(""), false);
        // Write host Keyboard LED Status to OLEDs
        led_t led_usb_state = host_keyboard_led_state();
        oled_write_P(led_usb_state.num_lock ? PSTR("NUM ") : PSTR("    "), false);
        oled_write_P(led_usb_state.caps_lock ? PSTR("CAP ") : PSTR("    "), false);
        oled_write_P(led_usb_state.scroll_lock ? PSTR("SCR ") : PSTR("    "), false);
    } else {
        // our left oled seems to be dead
        // in any case if you have to test it, make sure to flash left side as well

        // clang-format off
        static const char PROGMEM kyria_logo[] = {
            0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,128,128,192,224,240,112,120, 56, 60, 28, 30, 14, 14, 14,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7, 14, 14, 14, 30, 28, 60, 56,120,112,240,224,192,128,128,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,192,224,240,124, 62, 31, 15,  7,  3,  1,128,192,224,240,120, 56, 60, 28, 30, 14, 14,  7,  7,135,231,127, 31,255,255, 31,127,231,135,  7,  7, 14, 14, 30, 28, 60, 56,120,240,224,192,128,  1,  3,  7, 15, 31, 62,124,240,224,192,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,240,252,255, 31,  7,  1,  0,  0,192,240,252,254,255,247,243,177,176, 48, 48, 48, 48, 48, 48, 48,120,254,135,  1,  0,  0,255,255,  0,  0,  1,135,254,120, 48, 48, 48, 48, 48, 48, 48,176,177,243,247,255,254,252,240,192,  0,  0,  1,  7, 31,255,252,240,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,255,255,255,  0,  0,  0,  0,  0,254,255,255,  1,  1,  7, 30,120,225,129,131,131,134,134,140,140,152,152,177,183,254,248,224,255,255,224,248,254,183,177,152,152,140,140,134,134,131,131,129,225,120, 30,  7,  1,  1,255,255,254,  0,  0,  0,  0,  0,255,255,255,  0,  0,  0,  0,255,255,  0,  0,192,192, 48, 48,  0,  0,240,240,  0,  0,  0,  0,  0,  0,240,240,  0,  0,240,240,192,192, 48, 48, 48, 48,192,192,  0,  0, 48, 48,243,243,  0,  0,  0,  0,  0,  0, 48, 48, 48, 48, 48, 48,192,192,  0,  0,  0,  0,  0,
            0,  0,  0,255,255,255,  0,  0,  0,  0,  0,127,255,255,128,128,224,120, 30,135,129,193,193, 97, 97, 49, 49, 25, 25,141,237,127, 31,  7,255,255,  7, 31,127,237,141, 25, 25, 49, 49, 97, 97,193,193,129,135, 30,120,224,128,128,255,255,127,  0,  0,  0,  0,  0,255,255,255,  0,  0,  0,  0, 63, 63,  3,  3, 12, 12, 48, 48,  0,  0,  0,  0, 51, 51, 51, 51, 51, 51, 15, 15,  0,  0, 63, 63,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 48, 48, 63, 63, 48, 48,  0,  0, 12, 12, 51, 51, 51, 51, 51, 51, 63, 63,  0,  0,  0,  0,  0,
            0,  0,  0,  0, 15, 63,255,248,224,128,  0,  0,  3, 15, 63,127,255,239,207,141, 13, 12, 12, 12, 12, 12, 12, 12, 30,127,225,128,  0,  0,255,255,  0,  0,128,225,127, 30, 12, 12, 12, 12, 12, 12, 12, 13,141,207,239,255,127, 63, 15,  3,  0,  0,128,224,248,255, 63, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  3,  7, 15, 62,124,248,240,224,192,128,  1,  3,  7, 15, 30, 28, 60, 56,120,112,112,224,224,225,231,254,248,255,255,248,254,231,225,224,224,112,112,120, 56, 60, 28, 30, 15,  7,  3,  1,128,192,224,240,248,124, 62, 15,  7,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  3,  7, 15, 14, 30, 28, 60, 56,120,112,112,112,224,224,224,224,224,224,224,224,224,224,224,224,224,224,224,224,112,112,112,120, 56, 60, 28, 30, 14, 15,  7,  3,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
        };
        // clang-format on
        oled_write_raw_P(kyria_logo, sizeof(kyria_logo));
    }
    return false;
}
#endif

#ifdef ENCODER_ENABLE
bool encoder_update_user(uint8_t index, bool clockwise) {
    if (index == 0) {
        // Volume control
        if (clockwise) {
            tap_code(KC_VOLU);
        } else {
            tap_code(KC_VOLD);
        }
    } else if (index == 1) {
        // Page up/Page down
        if (clockwise) {
            tap_code(KC_PGDN);
        } else {
            tap_code(KC_PGUP);
        }
    }
    return false;
}
#endif

// ================================================================================
// FILE END
// ================================================================================
