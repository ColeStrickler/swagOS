#include <ps2_keyboard.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <serial.h>
#include <kernel.h>

extern KernelSettings global_Settings;

static enum pressed_codes
{
    PRESSED_INVALID1
    ,PRESSED_ESC
    ,PRESSED_1
    ,PRESSED_2
    ,PRESSED_3
    ,PRESSED_4
    ,PRESSED_5
    ,PRESSED_6
    ,PRESSED_7
    ,PRESSED_8
    ,PRESSED_9
    ,PRESSED_0
    ,PRESSED_DASH
    ,PRESSED_EQUALS
    ,PRESSED_BACKSPACE
    ,PRESSED_TAB
    ,PRESSED_Q
    ,PRESSED_W
    ,PRESSED_E
    ,PRESSED_R
    ,PRESSED_T
    ,PRESSED_Z
    ,PRESSED_U
    ,PRESSED_I
    ,PRESSED_O
    ,PRESSED_P
    ,PRESSED_LEFT_SQUARE_BRACKET
    ,PRESSED_RIGHT_SQUARE_BRACKET
    ,PRESSED_ENTER
    ,PRESSED_CTRL
    ,PRESSED_A
    ,PRESSED_S
    ,PRESSED_D
    ,PRESSED_F
    ,PRESSED_G
    ,PRESSED_H
    ,PRESSED_J
    ,PRESSED_K
    ,PRESSED_L
    ,PRESSED_SEMICOLON
    ,PRESSED_QUOTE
    ,PRESSED_TICK
    ,PRESSED_LEFT_SHIFT
    ,PRESSED_BACKSLASH
    ,PRESSED_Y
    ,PRESSED_X
    ,PRESSED_C
    ,PRESSED_V
    ,PRESSED_B
    ,PRESSED_N
    ,PRESSED_M
    ,PRESSED_COMMA
    ,PRESSED_DOT
    ,PRESSED_SLASH
    ,PRESSED_RIGHT_SHIFT
    ,PRESSED_STAR
    ,PRESSED_ALT
    ,PRESSED_SPACE
    ,PRESSED_CAPS_LOCK
    ,PRESSED_F1
    ,PRESSED_F2
    ,PRESSED_F3
    ,PRESSED_F4
    ,PRESSED_F5
    ,PRESSED_F6
    ,PRESSED_F7
    ,PRESSED_F8
    ,PRESSED_F9
    ,PRESSED_F10
    ,PRESSED_NUM_LOCK
    ,PRESSED_SCROLL_LOCK
    ,PRESSED_KEY_7
    ,PRESSED_KEY_8
    ,PRESSED_KEY_9
    ,PRESSED_KEY_MINUS
    ,PRESSED_KEY_4
    ,PRESSED_KEY_5
    ,PRESSED_KEY_6
    ,PRESSED_KEY_PLUS
    ,PRESSED_KEY_1
    ,PRESSED_KEY_2
    ,PRESSED_KEY_3
    ,PRESSED_KEY_0
    ,PRESSED_KEY_DOT
    ,PRESSED_INVALID2
    ,PRESSED_INVALID3
    ,PRESSED_INVALID4
    ,PRESSED_F11
    ,PRESSED_F12
    ,PRESSED_INVALID5
};

static enum released_codes
{
    RELEASED_INVALID1
    ,RELEASED_ESC
    ,RELEASED_1
    ,RELEASED_2
    ,RELEASED_3
    ,RELEASED_4
    ,RELEASED_5
    ,RELEASED_6
    ,RELEASED_7
    ,RELEASED_8
    ,RELEASED_9
    ,RELEASED_0
    ,RELEASED_DASH
    ,RELEASED_EQUALS
    ,RELEASED_BACKSPACE
    ,RELEASED_TAB
    ,RELEASED_Q
    ,RELEASED_W
    ,RELEASED_E
    ,RELEASED_R
    ,RELEASED_T
    ,RELEASED_Z
    ,RELEASED_U
    ,RELEASED_I
    ,RELEASED_O
    ,RELEASED_P
    ,RELEASED_LEFT_SQUARE_BRACKET
    ,RELEASED_RIGHT_SQUARE_BRACKET
    ,RELEASED_ENTER
    ,RELEASED_CTRL
    ,RELEASED_A
    ,RELEASED_S
    ,RELEASED_D
    ,RELEASED_F
    ,RELEASED_G
    ,RELEASED_H
    ,RELEASED_J
    ,RELEASED_K
    ,RELEASED_L
    ,RELEASED_SEMICOLON
    ,RELEASED_QUOTE
    ,RELEASED_TICK
    ,RELEASED_LEFT_SHIFT
    ,RELEASED_BACKSLASH
    ,RELEASED_Y
    ,RELEASED_X
    ,RELEASED_C
    ,RELEASED_V
    ,RELEASED_B
    ,RELEASED_N
    ,RELEASED_M
    ,RELEASED_COMMA
    ,RELEASED_DOT
    ,RELEASED_SLASH
    ,RELEASED_RIGHT_SHIFT
    ,RELEASED_STAR
    ,RELEASED_ALT
    ,RELEASED_SPACE
    ,RELEASED_CAPS_LOCK
    ,RELEASED_F1
    ,RELEASED_F2
    ,RELEASED_F3
    ,RELEASED_F4
    ,RELEASED_F5
    ,RELEASED_F6
    ,RELEASED_F7
    ,RELEASED_F8
    ,RELEASED_F9
    ,RELEASED_F10
    ,RELEASED_NUM_LOCK
    ,RELEASED_SCROLL_LOCK
    ,RELEASED_KEY_7
    ,RELEASED_KEY_8
    ,RELEASED_KEY_9
    ,RELEASED_KEY_MINUS
    ,RELEASED_KEY_4
    ,RELEASED_KEY_5
    ,RELEASED_KEY_6
    ,RELEASED_KEY_PLUS
    ,RELEASED_KEY_1
    ,RELEASED_KEY_2
    ,RELEASED_KEY_3
    ,RELEASED_KEY_0
    ,RELEASED_KEY_DOT
    ,RELEASED_INVALID2
    ,RELEASED_INVALID3
    ,RELEASED_INVALID4
    ,RELEASED_F11
    ,RELEASED_F12
    ,RELEASED_INVALID5
};

void keyboard_driver_init()
{
	unsigned char key = 0;
    // let's clear any junk currently in the keyboard port
    while(((key = inb(0x64)) & 1) == 1){
		log_hexval("key", key);
        inb(0x60);
    }
    // Unmask the keyboard IRQ in the IOAPIC
    set_irq(0x01, 0x01, 0x21, 0, 0, false);
}

char shift_key_to_ascii(uint8_t key){
    return shifted_qwertz[key];
}

bool is_ascii(char c)
{
    if (c >= 0x20 && c <= 0x7e)
        return true;
    return false;
}



bool scancode_to_char(uint8_t scancode, char* outchar)
{
    if (scancode & 0x80)
    {
        scancode &= ~0x80;
        switch(scancode)
        {
            case RELEASED_LEFT_SHIFT:
            {
                 global_Settings.KeyboardDriver.shift_pressed = false;
                 break;
            }
            case RELEASED_RIGHT_SHIFT:
            {
                 global_Settings.KeyboardDriver.shift_pressed = false;
                 break;
            }
            default:
                break;
        }
        return false;
    }
    else
    {
        switch(scancode)
        {
            case PRESSED_ENTER:
            {
                log_to_serial("[ENTER]");
            }
            case PRESSED_LEFT_SHIFT:
            {
                 global_Settings.KeyboardDriver.shift_pressed = true;
                 break;
            }
            case PRESSED_RIGHT_SHIFT:
            {
                global_Settings.KeyboardDriver.shift_pressed = true;
                break;
            }
            default:
            {
                if (global_Settings.KeyboardDriver.shift_pressed)
                    *outchar = shifted_qwertz[scancode];
                else
                    *outchar = qwertz[scancode];
                break;
            }
        }
        
    }       
    return true;
}


void keyboard_irq_handler()
{
    uint8_t scancode = inb(0x60);
    char c;
    if(scancode_to_char(scancode, &c) && is_ascii(c))
        log_char_to_serial(c);
}

