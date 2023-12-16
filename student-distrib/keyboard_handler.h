#ifndef _KEYBOARD_HANDLER_H
#define _KEYBOARD_HANDLER_H

#include "types.h"

#define BUF_SIZE        128

#define KB_PENDING      0
#define KB_PRESSED      1

extern void init_keyboard(void);
extern void keyboard_handler(void);

#endif /* _KEYBOARD_HANDLER_H */
