#define NO              0

#define SHIFT           (1 << 0)
#define CTL             (1 << 1)
#define ALT             (1 << 2)

#define CAPSLOCK        (1 << 3)
#define NUMLOCK         (1 << 4)
#define SCROLLLOCK      (1 << 5)

#define E0ESC           (1 << 6)

/* Special keycodes */
#define KEY_HOME        0xe0
#define KEY_END         0xe1
#define KEY_UP          0xe2
#define KEY_DN          0xe3
#define KEY_LF          0xe4
#define KEY_RT          0xe5
#define KEY_PGUP        0xe6
#define KEY_PGDN        0xe7
#define KEY_INS         0xe8
#define KEY_DEL         0xe9

/* C('A') == Control-A */
#define C(x) (x - '@')

static uint8_t shiftcode[256] =
{
    [0x1D] CTL,
    [0x2A] SHIFT,
    [0x36] SHIFT,
    [0x38] ALT,
    [0x9D] CTL,
    [0xB8] ALT
};

static uint8_t togglecode[256] =
{
    [0x3A] CAPSLOCK,
    [0x45] NUMLOCK,
    [0x46] SCROLLLOCK
};

static uint8_t normalmap[256] =
{
    NO,   0x1B, '1',  '2',  '3',  '4',  '5',  '6',  /* 0x00 */
    '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  /* 0x10 */
    'o',  'p',  '[',  ']',  '\n', NO,   'a',  's',
    'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  /* 0x20 */
    '\'', '`',  NO,   '\\', 'z',  'x',  'c',  'v',
    'b',  'n',  'm',  ',',  '.',  '/',  NO,   '*',  /* 0x30 */
    NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
    NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  /* 0x40 */
    '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
    '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   /* 0x50 */
    [0x9c] '\n',      /* KP_Enter */
    [0xb5] '/',       /* KP_Div */
    [0xc8] KEY_UP,    [0xd0] KEY_DN,
    [0xc9] KEY_PGUP,  [0xd1] KEY_PGDN,
    [0xcb] KEY_LF,    [0xcd] KEY_RT,
    [0x97] KEY_HOME,  [0xcf] KEY_END,
    [0xd2] KEY_INS,   [0xd3] KEY_DEL
};

static uint8_t shiftmap[256] =
{
    NO,   033,  '!',  '@',  '#',  '$',  '%',  '^',  /* 0x00 */
    '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  /* 0x10 */
    'O',  'P',  '{',  '}',  '\n', NO,   'A',  'S',
    'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  /* 0x20 */
    '"',  '~',  NO,   '|',  'Z',  'X',  'C',  'V',
    'B',  'N',  'M',  '<',  '>',  '?',  NO,   '*',  /* 0x30 */
    NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
    NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  /* 0x40 */
    '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
    '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   /* 0x50 */
    [0x9c] '\n',      /* KP_Enter */
    [0xb5] '/',       /* KP_Div */
    [0xc8] KEY_UP,    [0xd0] KEY_DN,
    [0xc9] KEY_PGUP,  [0xd1] KEY_PGDN,
    [0xcb] KEY_LF,    [0xcd] KEY_RT,
    [0x97] KEY_HOME,  [0xcf] KEY_END,
    [0xd2] KEY_INS,   [0xd3] KEY_DEL
};

static uint8_t ctlmap[256] =
{
    NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
    NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
    C('Q'),  C('W'),  C('E'),  C('R'),  C('T'),  C('Y'),  C('U'),  C('I'),
    C('O'),  C('P'),  NO,      NO,      '\r',    NO,      C('A'),  C('S'),
    C('D'),  C('F'),  C('G'),  C('H'),  C('J'),  C('K'),  C('L'),  NO,
    NO,      NO,      NO,      C('\\'), C('Z'),  C('X'),  C('C'),  C('V'),
    C('B'),  C('N'),  C('M'),  NO,      NO,      C('/'),  NO,      NO,
    [0x9c] '\r',      /* KP_Enter */
    [0xb5] C('/'),    /* KP_Div */
    [0xc8] KEY_UP,    [0xd0] KEY_DN,
    [0xc9] KEY_PGUP,  [0xd1] KEY_PGDN,
    [0xcb] KEY_LF,    [0xcd] KEY_RT,
    [0x97] KEY_HOME,  [0xcf] KEY_END,
    [0xd2] KEY_INS,   [0xd3] KEY_DEL
};
