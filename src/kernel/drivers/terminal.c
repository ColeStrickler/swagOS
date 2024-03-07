#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <video.h>
#include <kernel.h>
#include "terminal.h"
#include <serial.h>
extern KernelSettings global_Settings;




void init_terminal()
{
    global_Settings.TerminalDriver.current_x = 0;
    global_Settings.TerminalDriver.current_y = 0;
    global_Settings.TerminalDriver.max_x = global_Settings.framebuffer->common.framebuffer_width;
    global_Settings.TerminalDriver.max_y = global_Settings.framebuffer->common.framebuffer_height;
}

uint32_t char_code_to_fontcode(char c)
{
    return c - 32; 
}


void terminal_print_char(char c, uint32_t color)
{
    TerminalState* driver = &global_Settings.TerminalDriver;

    if (driver->current_x + 16 > driver->max_x)
    {
        driver->current_y += 16;
        driver->current_x = 0;
    }
    draw_character(driver->current_x, driver->current_y, color, char_code_to_fontcode(c));
    if (driver->current_x + 16 > driver->max_x)
    {
        driver->current_y += 16;
        driver->current_x = 0;
    }
    else
        driver->current_x += 16;
    
}


void terminal_print_string(const char* str, uint32_t color)
{
    for (int i = 0; i < strlen(str); i++)
	{
		terminal_print_char(str[i], color);
	}
}




