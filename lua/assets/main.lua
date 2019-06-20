local ffi = require("ffi")

ffi.cdef[[

typedef enum binocle_blend_factor {
  BINOCLE_BLEND_ZERO,             ///< (0, 0, 0, 0)
  BINOCLE_BLEND_ONE,              ///< (1, 1, 1, 1)
  BINOCLE_BLEND_SRCCOLOR,         ///< (src.r, src.g, src.b, src.a)
  BINOCLE_BLEND_ONEMINUSSRCCOLOR, ///< (1, 1, 1, 1) - (src.r, src.g, src.b, src.a)
  BINOCLE_BLEND_DSTCOLOR,         ///< (dst.r, dst.g, dst.b, dst.a)
  BINOCLE_BLEND_ONEMINUSDSTCOLOR, ///< (1, 1, 1, 1) - (dst.r, dst.g, dst.b, dst.a)
  BINOCLE_BLEND_SRCALPHA,         ///< (src.a, src.a, src.a, src.a)
  BINOCLE_BLEND_ONEMINUSSRCALPHA, ///< (1, 1, 1, 1) - (src.a, src.a, src.a, src.a)
  BINOCLE_BLEND_DSTALPHA,         ///< (dst.a, dst.a, dst.a, dst.a)
  BINOCLE_BLEND_ONEMINUSDSTALPHA  ///< (1, 1, 1, 1) - (dst.a, dst.a, dst.a, dst.a)
} binocle_blend_factor;

typedef enum binocle_blend_equation {
  BINOCLE_BLEND_ADD,     ///< Pixel = Src * SrcFactor + Dst * DstFactor
  BINOCLE_BLEND_SUBTRACT ///< Pixel = Src * SrcFactor - Dst * DstFactor
} binocle_blend_equation;

    typedef struct binocle_blend {
      binocle_blend_factor color_src_factor; ///< Source blending factor for the color channels
      binocle_blend_factor color_dst_factor; ///< Destination blending factor for the color channels
      binocle_blend_equation color_equation; ///< Blending equation for the color channels
      binocle_blend_factor alpha_src_factor; ///< Source blending factor for the alpha channel
      binocle_blend_factor alpha_dst_factor; ///< Destination blending factor for the alpha channel
      binocle_blend_equation alpha_equation; ///< Blending equation for the alpha channel
    } binocle_blend;

typedef struct binocle_image {
  /// The raw data of the image
  unsigned char *data; // NOTE: this might be better as void*
  /// The image width
  uint64_t width;
  /// The image height
  uint64_t height;
  // TODO: might as well add mipmap levels and pixel format
} binocle_image;

    typedef struct binocle_material {
      /// The blending mode
      binocle_blend blend_mode;
      /// The texture
      struct binocle_texture *texture;
      /// The shader (couple of VS/FS)
      struct binocle_shader *shader;
    } binocle_material;

typedef struct kmVec2 {
    float x;
    float y;
} kmVec2;

typedef struct kmAABB2 {
    kmVec2 min; /** The max corner of the box */
    kmVec2 max; /** The min corner of the box */
} kmAABB2;

typedef struct binocle_subtexture {
  struct binocle_texture *texture;
  kmAABB2 rect;
  char name[1024];
} binocle_subtexture;

typedef struct binocle_sprite_frame {
  binocle_subtexture *subtexture;
  kmVec2 origin;
} binocle_sprite_frame;

typedef struct binocle_sprite_animation_frame_mapping {
  char *name;
  int original_frame;
  int real_frame;
} binocle_sprite_animation_frame_mapping;

typedef struct binocle_sprite_animation {
  bool enabled;
  int frames[256];
  float delays[256];
  bool looping;
  int frames_number;
  char *name;
  binocle_sprite_animation_frame_mapping
    frame_mapping[256];
  int frame_mapping_number;
} binocle_sprite_animation;

typedef struct binocle_sprite {
  binocle_subtexture subtexture;
  struct binocle_material *material;
  kmVec2 origin;
  binocle_sprite_animation *animations; //[BINOCLE_SPRITE_MAX_ANIMATIONS];
  binocle_sprite_frame *frames; //[BINOCLE_SPRITE_MAX_FRAMES];
  int frames_number;
  bool playing;
  bool finished;
  float rate;
  int current_frame;
  binocle_sprite_animation *current_animation;
  int animations_number;
  int current_animation_id;
  int current_animation_frame;
  float timer;
} binocle_sprite;

typedef enum
{
    SDL_SCANCODE_UNKNOWN = 0,

    /**
     *  \name Usage page 0x07
     *
     *  These values are from usage page 0x07 (USB keyboard page).
     */
    /* @{ */

    SDL_SCANCODE_A = 4,
    SDL_SCANCODE_B = 5,
    SDL_SCANCODE_C = 6,
    SDL_SCANCODE_D = 7,
    SDL_SCANCODE_E = 8,
    SDL_SCANCODE_F = 9,
    SDL_SCANCODE_G = 10,
    SDL_SCANCODE_H = 11,
    SDL_SCANCODE_I = 12,
    SDL_SCANCODE_J = 13,
    SDL_SCANCODE_K = 14,
    SDL_SCANCODE_L = 15,
    SDL_SCANCODE_M = 16,
    SDL_SCANCODE_N = 17,
    SDL_SCANCODE_O = 18,
    SDL_SCANCODE_P = 19,
    SDL_SCANCODE_Q = 20,
    SDL_SCANCODE_R = 21,
    SDL_SCANCODE_S = 22,
    SDL_SCANCODE_T = 23,
    SDL_SCANCODE_U = 24,
    SDL_SCANCODE_V = 25,
    SDL_SCANCODE_W = 26,
    SDL_SCANCODE_X = 27,
    SDL_SCANCODE_Y = 28,
    SDL_SCANCODE_Z = 29,

    SDL_SCANCODE_1 = 30,
    SDL_SCANCODE_2 = 31,
    SDL_SCANCODE_3 = 32,
    SDL_SCANCODE_4 = 33,
    SDL_SCANCODE_5 = 34,
    SDL_SCANCODE_6 = 35,
    SDL_SCANCODE_7 = 36,
    SDL_SCANCODE_8 = 37,
    SDL_SCANCODE_9 = 38,
    SDL_SCANCODE_0 = 39,

    SDL_SCANCODE_RETURN = 40,
    SDL_SCANCODE_ESCAPE = 41,
    SDL_SCANCODE_BACKSPACE = 42,
    SDL_SCANCODE_TAB = 43,
    SDL_SCANCODE_SPACE = 44,

    SDL_SCANCODE_MINUS = 45,
    SDL_SCANCODE_EQUALS = 46,
    SDL_SCANCODE_LEFTBRACKET = 47,
    SDL_SCANCODE_RIGHTBRACKET = 48,
    SDL_SCANCODE_BACKSLASH = 49, /**< Located at the lower left of the return
                                  *   key on ISO keyboards and at the right end
                                  *   of the QWERTY row on ANSI keyboards.
                                  *   Produces REVERSE SOLIDUS (backslash) and
                                  *   VERTICAL LINE in a US layout, REVERSE
                                  *   SOLIDUS and VERTICAL LINE in a UK Mac
                                  *   layout, NUMBER SIGN and TILDE in a UK
                                  *   Windows layout, DOLLAR SIGN and POUND SIGN
                                  *   in a Swiss German layout, NUMBER SIGN and
                                  *   APOSTROPHE in a German layout, GRAVE
                                  *   ACCENT and POUND SIGN in a French Mac
                                  *   layout, and ASTERISK and MICRO SIGN in a
                                  *   French Windows layout.
                                  */
    SDL_SCANCODE_NONUSHASH = 50, /**< ISO USB keyboards actually use this code
                                  *   instead of 49 for the same key, but all
                                  *   OSes I've seen treat the two codes
                                  *   identically. So, as an implementor, unless
                                  *   your keyboard generates both of those
                                  *   codes and your OS treats them differently,
                                  *   you should generate SDL_SCANCODE_BACKSLASH
                                  *   instead of this code. As a user, you
                                  *   should not rely on this code because SDL
                                  *   will never generate it with most (all?)
                                  *   keyboards.
                                  */
    SDL_SCANCODE_SEMICOLON = 51,
    SDL_SCANCODE_APOSTROPHE = 52,
    SDL_SCANCODE_GRAVE = 53, /**< Located in the top left corner (on both ANSI
                              *   and ISO keyboards). Produces GRAVE ACCENT and
                              *   TILDE in a US Windows layout and in US and UK
                              *   Mac layouts on ANSI keyboards, GRAVE ACCENT
                              *   and NOT SIGN in a UK Windows layout, SECTION
                              *   SIGN and PLUS-MINUS SIGN in US and UK Mac
                              *   layouts on ISO keyboards, SECTION SIGN and
                              *   DEGREE SIGN in a Swiss German layout (Mac:
                              *   only on ISO keyboards), CIRCUMFLEX ACCENT and
                              *   DEGREE SIGN in a German layout (Mac: only on
                              *   ISO keyboards), SUPERSCRIPT TWO and TILDE in a
                              *   French Windows layout, COMMERCIAL AT and
                              *   NUMBER SIGN in a French Mac layout on ISO
                              *   keyboards, and LESS-THAN SIGN and GREATER-THAN
                              *   SIGN in a Swiss German, German, or French Mac
                              *   layout on ANSI keyboards.
                              */
    SDL_SCANCODE_COMMA = 54,
    SDL_SCANCODE_PERIOD = 55,
    SDL_SCANCODE_SLASH = 56,

    SDL_SCANCODE_CAPSLOCK = 57,

    SDL_SCANCODE_F1 = 58,
    SDL_SCANCODE_F2 = 59,
    SDL_SCANCODE_F3 = 60,
    SDL_SCANCODE_F4 = 61,
    SDL_SCANCODE_F5 = 62,
    SDL_SCANCODE_F6 = 63,
    SDL_SCANCODE_F7 = 64,
    SDL_SCANCODE_F8 = 65,
    SDL_SCANCODE_F9 = 66,
    SDL_SCANCODE_F10 = 67,
    SDL_SCANCODE_F11 = 68,
    SDL_SCANCODE_F12 = 69,

    SDL_SCANCODE_PRINTSCREEN = 70,
    SDL_SCANCODE_SCROLLLOCK = 71,
    SDL_SCANCODE_PAUSE = 72,
    SDL_SCANCODE_INSERT = 73, /**< insert on PC, help on some Mac keyboards (but
                                   does send code 73, not 117) */
    SDL_SCANCODE_HOME = 74,
    SDL_SCANCODE_PAGEUP = 75,
    SDL_SCANCODE_DELETE = 76,
    SDL_SCANCODE_END = 77,
    SDL_SCANCODE_PAGEDOWN = 78,
    SDL_SCANCODE_RIGHT = 79,
    SDL_SCANCODE_LEFT = 80,
    SDL_SCANCODE_DOWN = 81,
    SDL_SCANCODE_UP = 82,

    SDL_SCANCODE_NUMLOCKCLEAR = 83, /**< num lock on PC, clear on Mac keyboards
                                     */
    SDL_SCANCODE_KP_DIVIDE = 84,
    SDL_SCANCODE_KP_MULTIPLY = 85,
    SDL_SCANCODE_KP_MINUS = 86,
    SDL_SCANCODE_KP_PLUS = 87,
    SDL_SCANCODE_KP_ENTER = 88,
    SDL_SCANCODE_KP_1 = 89,
    SDL_SCANCODE_KP_2 = 90,
    SDL_SCANCODE_KP_3 = 91,
    SDL_SCANCODE_KP_4 = 92,
    SDL_SCANCODE_KP_5 = 93,
    SDL_SCANCODE_KP_6 = 94,
    SDL_SCANCODE_KP_7 = 95,
    SDL_SCANCODE_KP_8 = 96,
    SDL_SCANCODE_KP_9 = 97,
    SDL_SCANCODE_KP_0 = 98,
    SDL_SCANCODE_KP_PERIOD = 99,

    SDL_SCANCODE_NONUSBACKSLASH = 100, /**< This is the additional key that ISO
                                        *   keyboards have over ANSI ones,
                                        *   located between left shift and Y.
                                        *   Produces GRAVE ACCENT and TILDE in a
                                        *   US or UK Mac layout, REVERSE SOLIDUS
                                        *   (backslash) and VERTICAL LINE in a
                                        *   US or UK Windows layout, and
                                        *   LESS-THAN SIGN and GREATER-THAN SIGN
                                        *   in a Swiss German, German, or French
                                        *   layout. */
    SDL_SCANCODE_APPLICATION = 101, /**< windows contextual menu, compose */
    SDL_SCANCODE_POWER = 102, /**< The USB document says this is a status flag,
                               *   not a physical key - but some Mac keyboards
                               *   do have a power key. */
    SDL_SCANCODE_KP_EQUALS = 103,
    SDL_SCANCODE_F13 = 104,
    SDL_SCANCODE_F14 = 105,
    SDL_SCANCODE_F15 = 106,
    SDL_SCANCODE_F16 = 107,
    SDL_SCANCODE_F17 = 108,
    SDL_SCANCODE_F18 = 109,
    SDL_SCANCODE_F19 = 110,
    SDL_SCANCODE_F20 = 111,
    SDL_SCANCODE_F21 = 112,
    SDL_SCANCODE_F22 = 113,
    SDL_SCANCODE_F23 = 114,
    SDL_SCANCODE_F24 = 115,
    SDL_SCANCODE_EXECUTE = 116,
    SDL_SCANCODE_HELP = 117,
    SDL_SCANCODE_MENU = 118,
    SDL_SCANCODE_SELECT = 119,
    SDL_SCANCODE_STOP = 120,
    SDL_SCANCODE_AGAIN = 121,   /**< redo */
    SDL_SCANCODE_UNDO = 122,
    SDL_SCANCODE_CUT = 123,
    SDL_SCANCODE_COPY = 124,
    SDL_SCANCODE_PASTE = 125,
    SDL_SCANCODE_FIND = 126,
    SDL_SCANCODE_MUTE = 127,
    SDL_SCANCODE_VOLUMEUP = 128,
    SDL_SCANCODE_VOLUMEDOWN = 129,
/* not sure whether there's a reason to enable these */
/*     SDL_SCANCODE_LOCKINGCAPSLOCK = 130,  */
/*     SDL_SCANCODE_LOCKINGNUMLOCK = 131, */
/*     SDL_SCANCODE_LOCKINGSCROLLLOCK = 132, */
    SDL_SCANCODE_KP_COMMA = 133,
    SDL_SCANCODE_KP_EQUALSAS400 = 134,

    SDL_SCANCODE_INTERNATIONAL1 = 135, /**< used on Asian keyboards, see
                                            footnotes in USB doc */
    SDL_SCANCODE_INTERNATIONAL2 = 136,
    SDL_SCANCODE_INTERNATIONAL3 = 137, /**< Yen */
    SDL_SCANCODE_INTERNATIONAL4 = 138,
    SDL_SCANCODE_INTERNATIONAL5 = 139,
    SDL_SCANCODE_INTERNATIONAL6 = 140,
    SDL_SCANCODE_INTERNATIONAL7 = 141,
    SDL_SCANCODE_INTERNATIONAL8 = 142,
    SDL_SCANCODE_INTERNATIONAL9 = 143,
    SDL_SCANCODE_LANG1 = 144, /**< Hangul/English toggle */
    SDL_SCANCODE_LANG2 = 145, /**< Hanja conversion */
    SDL_SCANCODE_LANG3 = 146, /**< Katakana */
    SDL_SCANCODE_LANG4 = 147, /**< Hiragana */
    SDL_SCANCODE_LANG5 = 148, /**< Zenkaku/Hankaku */
    SDL_SCANCODE_LANG6 = 149, /**< reserved */
    SDL_SCANCODE_LANG7 = 150, /**< reserved */
    SDL_SCANCODE_LANG8 = 151, /**< reserved */
    SDL_SCANCODE_LANG9 = 152, /**< reserved */

    SDL_SCANCODE_ALTERASE = 153, /**< Erase-Eaze */
    SDL_SCANCODE_SYSREQ = 154,
    SDL_SCANCODE_CANCEL = 155,
    SDL_SCANCODE_CLEAR = 156,
    SDL_SCANCODE_PRIOR = 157,
    SDL_SCANCODE_RETURN2 = 158,
    SDL_SCANCODE_SEPARATOR = 159,
    SDL_SCANCODE_OUT = 160,
    SDL_SCANCODE_OPER = 161,
    SDL_SCANCODE_CLEARAGAIN = 162,
    SDL_SCANCODE_CRSEL = 163,
    SDL_SCANCODE_EXSEL = 164,

    SDL_SCANCODE_KP_00 = 176,
    SDL_SCANCODE_KP_000 = 177,
    SDL_SCANCODE_THOUSANDSSEPARATOR = 178,
    SDL_SCANCODE_DECIMALSEPARATOR = 179,
    SDL_SCANCODE_CURRENCYUNIT = 180,
    SDL_SCANCODE_CURRENCYSUBUNIT = 181,
    SDL_SCANCODE_KP_LEFTPAREN = 182,
    SDL_SCANCODE_KP_RIGHTPAREN = 183,
    SDL_SCANCODE_KP_LEFTBRACE = 184,
    SDL_SCANCODE_KP_RIGHTBRACE = 185,
    SDL_SCANCODE_KP_TAB = 186,
    SDL_SCANCODE_KP_BACKSPACE = 187,
    SDL_SCANCODE_KP_A = 188,
    SDL_SCANCODE_KP_B = 189,
    SDL_SCANCODE_KP_C = 190,
    SDL_SCANCODE_KP_D = 191,
    SDL_SCANCODE_KP_E = 192,
    SDL_SCANCODE_KP_F = 193,
    SDL_SCANCODE_KP_XOR = 194,
    SDL_SCANCODE_KP_POWER = 195,
    SDL_SCANCODE_KP_PERCENT = 196,
    SDL_SCANCODE_KP_LESS = 197,
    SDL_SCANCODE_KP_GREATER = 198,
    SDL_SCANCODE_KP_AMPERSAND = 199,
    SDL_SCANCODE_KP_DBLAMPERSAND = 200,
    SDL_SCANCODE_KP_VERTICALBAR = 201,
    SDL_SCANCODE_KP_DBLVERTICALBAR = 202,
    SDL_SCANCODE_KP_COLON = 203,
    SDL_SCANCODE_KP_HASH = 204,
    SDL_SCANCODE_KP_SPACE = 205,
    SDL_SCANCODE_KP_AT = 206,
    SDL_SCANCODE_KP_EXCLAM = 207,
    SDL_SCANCODE_KP_MEMSTORE = 208,
    SDL_SCANCODE_KP_MEMRECALL = 209,
    SDL_SCANCODE_KP_MEMCLEAR = 210,
    SDL_SCANCODE_KP_MEMADD = 211,
    SDL_SCANCODE_KP_MEMSUBTRACT = 212,
    SDL_SCANCODE_KP_MEMMULTIPLY = 213,
    SDL_SCANCODE_KP_MEMDIVIDE = 214,
    SDL_SCANCODE_KP_PLUSMINUS = 215,
    SDL_SCANCODE_KP_CLEAR = 216,
    SDL_SCANCODE_KP_CLEARENTRY = 217,
    SDL_SCANCODE_KP_BINARY = 218,
    SDL_SCANCODE_KP_OCTAL = 219,
    SDL_SCANCODE_KP_DECIMAL = 220,
    SDL_SCANCODE_KP_HEXADECIMAL = 221,

    SDL_SCANCODE_LCTRL = 224,
    SDL_SCANCODE_LSHIFT = 225,
    SDL_SCANCODE_LALT = 226, /**< alt, option */
    SDL_SCANCODE_LGUI = 227, /**< windows, command (apple), meta */
    SDL_SCANCODE_RCTRL = 228,
    SDL_SCANCODE_RSHIFT = 229,
    SDL_SCANCODE_RALT = 230, /**< alt gr, option */
    SDL_SCANCODE_RGUI = 231, /**< windows, command (apple), meta */

    SDL_SCANCODE_MODE = 257,    /**< I'm not sure if this is really not covered
                                 *   by any of the above, but since there's a
                                 *   special KMOD_MODE for it I'm adding it here
                                 */

    /* @} *//* Usage page 0x07 */

    /**
     *  \name Usage page 0x0C
     *
     *  These values are mapped from usage page 0x0C (USB consumer page).
     */
    /* @{ */

    SDL_SCANCODE_AUDIONEXT = 258,
    SDL_SCANCODE_AUDIOPREV = 259,
    SDL_SCANCODE_AUDIOSTOP = 260,
    SDL_SCANCODE_AUDIOPLAY = 261,
    SDL_SCANCODE_AUDIOMUTE = 262,
    SDL_SCANCODE_MEDIASELECT = 263,
    SDL_SCANCODE_WWW = 264,
    SDL_SCANCODE_MAIL = 265,
    SDL_SCANCODE_CALCULATOR = 266,
    SDL_SCANCODE_COMPUTER = 267,
    SDL_SCANCODE_AC_SEARCH = 268,
    SDL_SCANCODE_AC_HOME = 269,
    SDL_SCANCODE_AC_BACK = 270,
    SDL_SCANCODE_AC_FORWARD = 271,
    SDL_SCANCODE_AC_STOP = 272,
    SDL_SCANCODE_AC_REFRESH = 273,
    SDL_SCANCODE_AC_BOOKMARKS = 274,

    /* @} *//* Usage page 0x0C */

    /**
     *  \name Walther keys
     *
     *  These are values that Christian Walther added (for mac keyboard?).
     */
    /* @{ */

    SDL_SCANCODE_BRIGHTNESSDOWN = 275,
    SDL_SCANCODE_BRIGHTNESSUP = 276,
    SDL_SCANCODE_DISPLAYSWITCH = 277, /**< display mirroring/dual display
                                           switch, video mode switch */
    SDL_SCANCODE_KBDILLUMTOGGLE = 278,
    SDL_SCANCODE_KBDILLUMDOWN = 279,
    SDL_SCANCODE_KBDILLUMUP = 280,
    SDL_SCANCODE_EJECT = 281,
    SDL_SCANCODE_SLEEP = 282,

    SDL_SCANCODE_APP1 = 283,
    SDL_SCANCODE_APP2 = 284,

    /* @} *//* Walther keys */

    /**
     *  \name Usage page 0x0C (additional media keys)
     *
     *  These values are mapped from usage page 0x0C (USB consumer page).
     */
    /* @{ */

    SDL_SCANCODE_AUDIOREWIND = 285,
    SDL_SCANCODE_AUDIOFASTFORWARD = 286,

    /* @} *//* Usage page 0x0C (additional media keys) */

    /* Add any other keys here. */

    SDL_NUM_SCANCODES = 512 /**< not a key, just marks the number of scancodes
                                 for array bounds */
} SDL_Scancode;

typedef enum binocle_input_keyboard_key {
  KEY_UNKNOWN = SDL_SCANCODE_UNKNOWN,
  KEY_A = SDL_SCANCODE_A,
  KEY_B = SDL_SCANCODE_B,
  KEY_C = SDL_SCANCODE_C,
  KEY_D = SDL_SCANCODE_D,
  KEY_E = SDL_SCANCODE_E,
  KEY_F = SDL_SCANCODE_F,
  KEY_G = SDL_SCANCODE_G,
  KEY_H = SDL_SCANCODE_H,
  KEY_I = SDL_SCANCODE_I,
  KEY_J = SDL_SCANCODE_J,
  KEY_K = SDL_SCANCODE_K,
  KEY_L = SDL_SCANCODE_L,
  KEY_M = SDL_SCANCODE_M,
  KEY_N = SDL_SCANCODE_N,
  KEY_O = SDL_SCANCODE_O,
  KEY_P = SDL_SCANCODE_P,
  KEY_Q = SDL_SCANCODE_Q,
  KEY_R = SDL_SCANCODE_R,
  KEY_S = SDL_SCANCODE_S,
  KEY_T = SDL_SCANCODE_T,
  KEY_U = SDL_SCANCODE_U,
  KEY_V = SDL_SCANCODE_V,
  KEY_W = SDL_SCANCODE_W,
  KEY_X = SDL_SCANCODE_X,
  KEY_Y = SDL_SCANCODE_Y,
  KEY_Z = SDL_SCANCODE_Z,
  KEY_1 = SDL_SCANCODE_1,
  KEY_2 = SDL_SCANCODE_2,
  KEY_3 = SDL_SCANCODE_3,
  KEY_4 = SDL_SCANCODE_4,
  KEY_5 = SDL_SCANCODE_5,
  KEY_6 = SDL_SCANCODE_6,
  KEY_7 = SDL_SCANCODE_7,
  KEY_8 = SDL_SCANCODE_8,
  KEY_9 = SDL_SCANCODE_9,
  KEY_0 = SDL_SCANCODE_0,
  KEY_RETURN = SDL_SCANCODE_RETURN,
  KEY_ESCAPE = SDL_SCANCODE_ESCAPE,
  KEY_BACKSPACE = SDL_SCANCODE_BACKSPACE,
  KEY_TAB = SDL_SCANCODE_TAB,
  KEY_SPACE = SDL_SCANCODE_SPACE,
  KEY_MINUS = SDL_SCANCODE_MINUS,
  KEY_EQUALS = SDL_SCANCODE_EQUALS,
  KEY_LEFTBRACKET = SDL_SCANCODE_LEFTBRACKET,
  KEY_RIGHTBRACKET = SDL_SCANCODE_RIGHTBRACKET,
  KEY_BACKSLASH = SDL_SCANCODE_BACKSLASH,
  KEY_NONUSHASH = SDL_SCANCODE_NONUSHASH,
  KEY_SEMICOLON = SDL_SCANCODE_SEMICOLON,
  KEY_APOSTROPHE = SDL_SCANCODE_APOSTROPHE,
  KEY_GRAVE = SDL_SCANCODE_GRAVE,
  KEY_COMMA = SDL_SCANCODE_COMMA,
  KEY_PERIOD = SDL_SCANCODE_PERIOD,
  KEY_SLASH = SDL_SCANCODE_SLASH,
  KEY_CAPSLOCK = SDL_SCANCODE_CAPSLOCK,
  KEY_F1 = SDL_SCANCODE_F1,
  KEY_F2 = SDL_SCANCODE_F2,
  KEY_F3 = SDL_SCANCODE_F3,
  KEY_F4 = SDL_SCANCODE_F4,
  KEY_F5 = SDL_SCANCODE_F5,
  KEY_F6 = SDL_SCANCODE_F6,
  KEY_F7 = SDL_SCANCODE_F7,
  KEY_F8 = SDL_SCANCODE_F8,
  KEY_F9 = SDL_SCANCODE_F9,
  KEY_F10 = SDL_SCANCODE_F10,
  KEY_F11 = SDL_SCANCODE_F11,
  KEY_F12 = SDL_SCANCODE_F12,
  KEY_PRINTSCREEN = SDL_SCANCODE_PRINTSCREEN,
  KEY_SCROLLLOCK = SDL_SCANCODE_SCROLLLOCK,
  KEY_PAUSE = SDL_SCANCODE_PAUSE,
  KEY_INSERT = SDL_SCANCODE_INSERT,
  KEY_HOME = SDL_SCANCODE_HOME,
  KEY_PAGEUP = SDL_SCANCODE_PAGEUP,
  KEY_DELETE = SDL_SCANCODE_DELETE,
  KEY_END = SDL_SCANCODE_END,
  KEY_PAGEDOWN = SDL_SCANCODE_PAGEDOWN,
  KEY_RIGHT = SDL_SCANCODE_RIGHT,
  KEY_LEFT = SDL_SCANCODE_LEFT,
  KEY_DOWN = SDL_SCANCODE_DOWN,
  KEY_UP = SDL_SCANCODE_UP,
  KEY_NUMLOCKCLEAR = SDL_SCANCODE_NUMLOCKCLEAR,
  KEY_KEYPAD_DIVIDE = SDL_SCANCODE_KP_DIVIDE,
  KEY_KEYPAD_MULTIPLY = SDL_SCANCODE_KP_MULTIPLY,
  KEY_KEYPAD_MINUS = SDL_SCANCODE_KP_MINUS,
  KEY_KEYPAD_PLUS = SDL_SCANCODE_KP_PLUS,
  KEY_KEYPAD_ENTER = SDL_SCANCODE_KP_ENTER,
  KEY_KEYPAD_1 = SDL_SCANCODE_KP_1,
  KEY_KEYPAD_2 = SDL_SCANCODE_KP_2,
  KEY_KEYPAD_3 = SDL_SCANCODE_KP_3,
  KEY_KEYPAD_4 = SDL_SCANCODE_KP_4,
  KEY_KEYPAD_5 = SDL_SCANCODE_KP_5,
  KEY_KEYPAD_6 = SDL_SCANCODE_KP_6,
  KEY_KEYPAD_7 = SDL_SCANCODE_KP_7,
  KEY_KEYPAD_8 = SDL_SCANCODE_KP_8,
  KEY_KEYPAD_9 = SDL_SCANCODE_KP_9,
  KEY_KEYPAD_0 = SDL_SCANCODE_KP_0,
  KEY_KEYPAD_PERIOD = SDL_SCANCODE_KP_PERIOD,
  KEY_NONUSBACKSLASH = SDL_SCANCODE_NONUSBACKSLASH,
  KEY_APPLICATION = SDL_SCANCODE_APPLICATION,
  KEY_POWER = SDL_SCANCODE_POWER,
  KEY_KEYPAD_EQUALS = SDL_SCANCODE_KP_EQUALS,
  KEY_F13 = SDL_SCANCODE_F13,
  KEY_F14 = SDL_SCANCODE_F14,
  KEY_F15 = SDL_SCANCODE_F15,
  KEY_F16 = SDL_SCANCODE_F16,
  KEY_F17 = SDL_SCANCODE_F17,
  KEY_F18 = SDL_SCANCODE_F18,
  KEY_F19 = SDL_SCANCODE_F19,
  KEY_F20 = SDL_SCANCODE_F20,
  KEY_F21 = SDL_SCANCODE_F21,
  KEY_F22 = SDL_SCANCODE_F22,
  KEY_F23 = SDL_SCANCODE_F23,
  KEY_F24 = SDL_SCANCODE_F24,
  KEY_EXECUTE_EX = SDL_SCANCODE_EXECUTE,
  KEY_HELP = SDL_SCANCODE_HELP,
  KEY_MENU = SDL_SCANCODE_MENU,
  KEY_SELECT = SDL_SCANCODE_SELECT,
  KEY_STOP = SDL_SCANCODE_STOP,
  KEY_AGAIN = SDL_SCANCODE_AGAIN,
  KEY_UNDO = SDL_SCANCODE_UNDO,
  KEY_CUT = SDL_SCANCODE_CUT,
  KEY_COPY = SDL_SCANCODE_COPY,
  KEY_PASTE = SDL_SCANCODE_PASTE,
  KEY_FIND = SDL_SCANCODE_FIND,
  KEY_MUTE = SDL_SCANCODE_MUTE,
  KEY_VOLUMEUP = SDL_SCANCODE_VOLUMEUP,
  KEY_VOLUMEDOWN = SDL_SCANCODE_VOLUMEDOWN,
  KEY_KEYPAD_COMMA = SDL_SCANCODE_KP_COMMA,
  KEY_KEYPAD_EQUALSAS400 = SDL_SCANCODE_KP_EQUALSAS400,
  KEY_INTERNATIONAL1 = SDL_SCANCODE_INTERNATIONAL1,
  KEY_INTERNATIONAL2 = SDL_SCANCODE_INTERNATIONAL2,
  KEY_INTERNATIONAL3 = SDL_SCANCODE_INTERNATIONAL3,
  KEY_INTERNATIONAL4 = SDL_SCANCODE_INTERNATIONAL4,
  KEY_INTERNATIONAL5 = SDL_SCANCODE_INTERNATIONAL5,
  KEY_INTERNATIONAL6 = SDL_SCANCODE_INTERNATIONAL6,
  KEY_INTERNATIONAL7 = SDL_SCANCODE_INTERNATIONAL7,
  KEY_INTERNATIONAL8 = SDL_SCANCODE_INTERNATIONAL8,
  KEY_INTERNATIONAL9 = SDL_SCANCODE_INTERNATIONAL9,
  KEY_LANG1 = SDL_SCANCODE_LANG1,
  KEY_LANG2 = SDL_SCANCODE_LANG2,
  KEY_LANG3 = SDL_SCANCODE_LANG3,
  KEY_LANG4 = SDL_SCANCODE_LANG4,
  KEY_LANG5 = SDL_SCANCODE_LANG5,
  KEY_LANG6 = SDL_SCANCODE_LANG6,
  KEY_LANG7 = SDL_SCANCODE_LANG7,
  KEY_LANG8 = SDL_SCANCODE_LANG8,
  KEY_LANG9 = SDL_SCANCODE_LANG9,
  KEY_ALTERASE = SDL_SCANCODE_ALTERASE,
  KEY_SYSREQ = SDL_SCANCODE_SYSREQ,
  KEY_CANCEL = SDL_SCANCODE_CANCEL,
  KEY_CLEAR = SDL_SCANCODE_CLEAR,
  KEY_PRIOR = SDL_SCANCODE_PRIOR,
  KEY_RETURN2 = SDL_SCANCODE_RETURN2,
  KEY_SEPARATOR = SDL_SCANCODE_SEPARATOR,
  KEY_OUT = SDL_SCANCODE_OUT,
  KEY_OPER = SDL_SCANCODE_OPER,
  KEY_CLEARAGAIN = SDL_SCANCODE_CLEARAGAIN,
  KEY_CRSEL = SDL_SCANCODE_CRSEL,
  KEY_EXSEL = SDL_SCANCODE_EXSEL,
  KEY_KEYPAD_00 = SDL_SCANCODE_KP_00,
  KEY_KEYPAD_000 = SDL_SCANCODE_KP_000,
  KEY_THOUSANDSSEPARATOR = SDL_SCANCODE_THOUSANDSSEPARATOR,
  KEY_DECIMALSEPARATOR = SDL_SCANCODE_DECIMALSEPARATOR,
  KEY_CURRENCYUNIT = SDL_SCANCODE_CURRENCYUNIT,
  KEY_CURRENCYSUBUNIT = SDL_SCANCODE_CURRENCYSUBUNIT,
  KEY_KEYPAD_LEFTPAREN = SDL_SCANCODE_KP_LEFTPAREN,
  KEY_KEYPAD_RIGHTPAREN = SDL_SCANCODE_KP_RIGHTPAREN,
  KEY_KEYPAD_LEFTBRACE = SDL_SCANCODE_KP_LEFTBRACE,
  KEY_KEYPAD_RIGHTBRACE = SDL_SCANCODE_KP_RIGHTBRACE,
  KEY_KEYPAD_TAB = SDL_SCANCODE_KP_TAB,
  KEY_KEYPAD_BACKSPACE = SDL_SCANCODE_KP_BACKSPACE,
  KEY_KEYPAD_A = SDL_SCANCODE_KP_A,
  KEY_KEYPAD_B = SDL_SCANCODE_KP_B,
  KEY_KEYPAD_C = SDL_SCANCODE_KP_C,
  KEY_KEYPAD_D = SDL_SCANCODE_KP_D,
  KEY_KEYPAD_E = SDL_SCANCODE_KP_E,
  KEY_KEYPAD_F = SDL_SCANCODE_KP_F,
  KEY_KEYPAD_XOR = SDL_SCANCODE_KP_XOR,
  KEY_KEYPAD_POWER = SDL_SCANCODE_KP_POWER,
  KEY_KEYPAD_PERCENT = SDL_SCANCODE_KP_PERCENT,
  KEY_KEYPAD_LESS = SDL_SCANCODE_KP_LESS,
  KEY_KEYPAD_GREATER = SDL_SCANCODE_KP_GREATER,
  KEY_KEYPAD_AMPERSAND = SDL_SCANCODE_KP_AMPERSAND,
  KEY_KEYPAD_DBLAMPERSAND = SDL_SCANCODE_KP_DBLAMPERSAND,
  KEY_KEYPAD_VERTICALBAR = SDL_SCANCODE_KP_VERTICALBAR,
  KEY_KEYPAD_DBLVERTICALBAR = SDL_SCANCODE_KP_DBLVERTICALBAR,
  KEY_KEYPAD_COLON = SDL_SCANCODE_KP_COLON,
  KEY_KEYPAD_HASH = SDL_SCANCODE_KP_HASH,
  KEY_KEYPAD_SPACE = SDL_SCANCODE_KP_SPACE,
  KEY_KEYPAD_AT = SDL_SCANCODE_KP_AT,
  KEY_KEYPAD_EXCLAM = SDL_SCANCODE_KP_EXCLAM,
  KEY_KEYPAD_MEMSTORE = SDL_SCANCODE_KP_MEMSTORE,
  KEY_KEYPAD_MEMRECALL = SDL_SCANCODE_KP_MEMRECALL,
  KEY_KEYPAD_MEMCLEAR = SDL_SCANCODE_KP_MEMCLEAR,
  KEY_KEYPAD_MEMADD = SDL_SCANCODE_KP_MEMADD,
  KEY_KEYPAD_MEMSUBTRACT = SDL_SCANCODE_KP_MEMSUBTRACT,
  KEY_KEYPAD_MEMMULTIPLY = SDL_SCANCODE_KP_MEMMULTIPLY,
  KEY_KEYPAD_MEMDIVIDE = SDL_SCANCODE_KP_MEMDIVIDE,
  KEY_KEYPAD_PLUSMINUS = SDL_SCANCODE_KP_PLUSMINUS,
  KEY_KEYPAD_CLEAR = SDL_SCANCODE_KP_CLEAR,
  KEY_KEYPAD_CLEARENTRY = SDL_SCANCODE_KP_CLEARENTRY,
  KEY_KEYPAD_BINARY = SDL_SCANCODE_KP_BINARY,
  KEY_KEYPAD_OCTAL = SDL_SCANCODE_KP_OCTAL,
  KEY_KEYPAD_DECIMAL = SDL_SCANCODE_KP_DECIMAL,
  KEY_KEYPAD_HEXADECIMAL = SDL_SCANCODE_KP_HEXADECIMAL,
  KEY_LEFT_CTRL = SDL_SCANCODE_LCTRL,
  KEY_LEFT_SHIFT = SDL_SCANCODE_LSHIFT,
  KEY_LEFT_ALT = SDL_SCANCODE_LALT,
  KEY_LEFT_GUI = SDL_SCANCODE_LGUI,
  KEY_RIGHT_CTRL = SDL_SCANCODE_RCTRL,
  KEY_RIGHT_SHIFT = SDL_SCANCODE_RSHIFT,
  KEY_RIGHT_ALT = SDL_SCANCODE_RALT,
  KEY_RIGHT_GUI = SDL_SCANCODE_RGUI,
  KEY_MODE = SDL_SCANCODE_MODE,
  KEY_AUDIONEXT = SDL_SCANCODE_AUDIONEXT,
  KEY_AUDIOPREV = SDL_SCANCODE_AUDIOPREV,
  KEY_AUDIOSTOP = SDL_SCANCODE_AUDIOSTOP,
  KEY_AUDIOPLAY = SDL_SCANCODE_AUDIOPLAY,
  KEY_AUDIOMUTE = SDL_SCANCODE_AUDIOMUTE,
  KEY_MEDIASELECT = SDL_SCANCODE_MEDIASELECT,
  KEY_WWW = SDL_SCANCODE_WWW,
  KEY_MAIL = SDL_SCANCODE_MAIL,
  KEY_CALCULATOR = SDL_SCANCODE_CALCULATOR,
  KEY_COMPUTER = SDL_SCANCODE_COMPUTER,
  KEY_AC_SEARCH = SDL_SCANCODE_AC_SEARCH,
  KEY_AC_HOME = SDL_SCANCODE_AC_HOME,
  KEY_AC_BACK = SDL_SCANCODE_AC_BACK,
  KEY_AC_FORWARD = SDL_SCANCODE_AC_FORWARD,
  KEY_AC_STOP = SDL_SCANCODE_AC_STOP,
  KEY_AC_REFRESH = SDL_SCANCODE_AC_REFRESH,
  KEY_AC_BOOKMARKS = SDL_SCANCODE_AC_BOOKMARKS,
  KEY_BRIGHTNESSDOWN = SDL_SCANCODE_BRIGHTNESSDOWN,
  KEY_BRIGHTNESSUP = SDL_SCANCODE_BRIGHTNESSUP,
  KEY_DISPLAYSWITCH = SDL_SCANCODE_DISPLAYSWITCH,
  KEY_KBDILLUMTOGGLE = SDL_SCANCODE_KBDILLUMTOGGLE,
  KEY_KBDILLUMDOWN = SDL_SCANCODE_KBDILLUMDOWN,
  KEY_KBDILLUMUP = SDL_SCANCODE_KBDILLUMUP,
  KEY_EJECT = SDL_SCANCODE_EJECT,
  KEY_SLEEP = SDL_SCANCODE_SLEEP,
  KEY_APP1 = SDL_SCANCODE_APP1,
  KEY_APP2 = SDL_SCANCODE_APP2,
  KEY_MAX
} binocle_input_keyboard_key;

    struct binocle_window binocle_window_new(uint32_t width, uint32_t height, char *title);
    char *binocle_sdl_assets_dir();
    struct binocle_image binocle_image_load(const char *filename);
    struct binocle_texture binocle_texture_from_image(struct binocle_image image);
    struct binocle_material binocle_material_new();
    struct binocle_sprite binocle_sprite_from_material(struct binocle_material *material);
    struct binocle_shader binocle_shader_load_from_file(char *vert_filename,
                                                 char *frag_filename);
    void binocle_sprite_draw(binocle_sprite *sprite, struct binocle_gd *gd, int64_t x, int64_t y, kmAABB2 *viewport, float rotation,
                         kmVec2 *scale, struct binocle_camera *camera);
    bool binocle_input_is_key_pressed(struct binocle_input *input, binocle_input_keyboard_key key);
]]

io.write("Begin of main.lua\n");

local c_assets_dir = ffi.C.binocle_sdl_assets_dir();
local assets_dir = ffi.string(c_assets_dir)
io.write(assets_dir .. "\n")


local image_filename = assets_dir .. "wabbit_alpha.png"
local c_str = ffi.new("char[?]", #image_filename)
ffi.copy(c_str, image_filename)
local image = ffi.C.binocle_image_load(c_str)
local texture = ffi.C.binocle_texture_from_image(image)
io.write("texture: " .. tostring(texture) .. "\n")

local vert = assets_dir .. "default.vert"
local c_vert = ffi.new("char[?]", #vert)
ffi.copy(c_vert, vert)

local frag = assets_dir .. "default.frag"
local c_frag = ffi.new("char[?]", #vert)
ffi.copy(c_frag, frag)

local shader = ffi.C.binocle_shader_load_from_file(c_vert, c_frag)
local material = ffi.C.binocle_material_new()

material.texture = texture;
material.shader = shader;
io.write("material: " .. tostring(material) .. "\n")
io.write("material.texture : " .. tostring(material.texture) .. "\n")
io.write("material.shader : " .. tostring(material.shader) .. "\n")

local player = ffi.C.binocle_sprite_from_material(material);
if player == nil or player == nullptr then
    io.write("player is nil")
else
    io.write("player: " .. tostring(player) .. "\n")
end

local player_x = 100
local player_y = 100

local scale = ffi.new("kmVec2")
scale.x = 1.0
scale.y = 1.0
io.write("scale: " .. tostring(scale) .. "\n")
io.write("scale.x: " .. tostring(scale.x) .. "\n")
io.write("scale.y: " .. tostring(scale.y) .. "\n")

io.write("gd: " .. tostring(gd) .. "\n")
io.write("viewport: " .. tostring(viewport) .. "\n")
io.write("camera: " .. tostring(camera) .. "\n")

function on_update(dt)
    --[[
    io.write(dt)
    io.write("player: " .. tostring(player) .. "\n")
    io.write("gd: " .. tostring(gd) .. "\n")
    io.write("viewport: " .. tostring(viewport) .. "\n")
    io.write("scale: " .. tostring(scale) .. "\n")
    io.write("camera: " .. tostring(camera) .. "\n")
    ]]
    --io.write(tostring(scale.y))

    if ffi.C.binocle_input_is_key_pressed(input, ffi.C.KEY_RIGHT) then
        player_x = player_x + 100 * dt
    elseif ffi.C.binocle_input_is_key_pressed(input, ffi.C.KEY_LEFT) then
        player_x = player_x - 100 * dt
    end

    if ffi.C.binocle_input_is_key_pressed(input, ffi.C.KEY_UP) then
        player_y = player_y + 100 * dt
    elseif ffi.C.binocle_input_is_key_pressed(input, ffi.C.KEY_DOWN) then
        player_y = player_y - 100 * dt
    end

    ffi.C.binocle_sprite_draw(player, gd, player_x, player_y, viewport, 0, scale, camera)
end

io.write("End of main.lua\n");