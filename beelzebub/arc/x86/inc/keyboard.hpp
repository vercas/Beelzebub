#pragma once

#include <system/interrupts.hpp>

#define KEYBOARD_CODE_ESCAPED   0xE0
#define KEYBOARD_CODE_LEFT      0x4B
#define KEYBOARD_CODE_RIGHT     0x4D
#define KEYBOARD_CODE_UP        0x48
#define KEYBOARD_CODE_DOWN      0x50

#define KEYBOARD_IRQ_VECTOR     0xEF

/**
 * Whether the escape scan code has been sent and the next
 * scan code is escaped.
 */
__extern bool keyboard_escaped;

__extern volatile int breakpointEscaped;

/**
 * Initializes the keyboard support.
 */
__extern __bland void keyboard_init(void);

/**
 * Sends a command to the keyboard controller.
 *
 * @param cmd the command to send
 */
__extern __bland void keyboard_send_command(uint8_t cmd);

/**
 * ISR for the keyboard IRQ.
 *
 * @param state state of the system
 */
__extern __bland void keyboard_handler(INTERRUPT_HANDLER_ARGS);
