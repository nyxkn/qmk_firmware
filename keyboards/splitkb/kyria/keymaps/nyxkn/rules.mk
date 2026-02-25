# kyria defaults
OLED_ENABLE = yes
ENCODER_ENABLE = no       # Enables the use of one or more encoders
RGB_MATRIX_ENABLE = no     # Disable keyboard RGB matrix, as it is enabled by default on rev3
RGBLIGHT_ENABLE = no      # Enable keyboard RGB underglow
# rgblight seems to trigger an error and oled shows exclamation marks like !!!!!!!!

# save space
LTO_ENABLE = yes

CONSOLE_ENABLE = no
COMMAND_ENABLE = no
MOUSEKEY_ENABLE = no
EXTRAKEY_ENABLE = no

SPACE_CADET_ENABLE = no
GRAVE_ESC_ENABLE = no
MAGIC_ENABLE = no

# AVR_USE_MINIMAL_PRINTF = yes

MUSIC_ENABLE = no


# bootloader
BOOTLOADER = caterina


# rules
# SRC += configurator.c
# to save space, we can technically make key overrides with tap dance instead
KEY_OVERRIDE_ENABLE = yes
TAP_DANCE_ENABLE = yes
# AUTO_SHIFT_ENABLE = yes
# if you need to enable console, temporarily disable unicode or caps
# CONSOLE_ENABLE = yes
CAPS_WORD_ENABLE = yes
UNICODE_ENABLE = yes

RAW_ENABLE = yes
