#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <video.h>
#include <kernel.h>
#include "terminal.h"
#include <serial.h>
#include <vmm.h>
#include <string.h>
#include <panic.h>
extern KernelSettings global_Settings;




void init_terminal()
{
    log_to_serial("init_terminal()\n");
    TerminalState* driver = &global_Settings.TerminalDriver;
    driver->current_x = 0;
    driver->current_y = 0;
    driver->max_x = global_Settings.framebuffer->common.framebuffer_width;
    driver->max_y = global_Settings.framebuffer->common.framebuffer_height;


    /*
        Characters are only 8bits wide, but we will allow 16 bits per character
        so that the letters are not squashing into each other
    */
    driver->terminal_buf_size = (driver->max_x * driver->max_y * (global_Settings.framebuffer->common.framebuffer_bpp/8)) / (2*16*16);
    char* terminal_buf = (char*)kalloc(driver->terminal_buf_size);
    driver->buffer_write_location = 0;
    log_hexval("terminal_buf", terminal_buf);
    log_hexval("terminal_buf size", driver->terminal_buf_size);
    if (terminal_buf == NULL)
        panic("init_terminal() --> could not allocate memory for terminal driver buffer.");
    driver->terminal_buf = terminal_buf;
}

uint8_t char_code_to_fontcode(char c)
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



void terminal_scroll_down()
{
    TerminalState* driver = &global_Settings.TerminalDriver;
    uint32_t row_size_in_chars = (driver->max_x*(global_Settings.framebuffer->common.framebuffer_bpp/8)) / (32);
    //log_hexval("row size", row_size_in_chars);
    for (int i = row_size_in_chars; i < driver->terminal_buf_size; i++)
    {
        //log_hexval("i", i);
        //log_hexval("driver->terminal_buf[i-row_size_in_chars] ==> ", driver->terminal_buf[i]);
        driver->terminal_buf[i-row_size_in_chars] = driver->terminal_buf[i];
    }
    //log_to_serial("here!\n");
    for (int i = 0; i < row_size_in_chars; i++)
        driver->terminal_buf[driver->terminal_buf_size - i - 1] = ' ';
    driver->buffer_write_location = driver->terminal_buf_size -row_size_in_chars- 1;
}



uint32_t terminal_buf_location_to_ypixel()
{
    // each character takes up 16x16 space, each pixel is 4 bytes
    TerminalState* driver = &global_Settings.TerminalDriver;
    uint32_t x = (driver->buffer_write_location * 2 * (global_Settings.framebuffer->common.framebuffer_bpp/8)) % driver->max_x;
    uint32_t y = 16*((driver->buffer_write_location*2*(global_Settings.framebuffer->common.framebuffer_bpp/8))-x) / (driver->max_x);
    return y;
}

uint32_t terminal_buf_location_to_xpixel()
{
    TerminalState* driver = &global_Settings.TerminalDriver;
    uint32_t x = (driver->buffer_write_location * 2*(global_Settings.framebuffer->common.framebuffer_bpp/8)) % driver->max_x;
    return x;
}




void terminal_print_buffer(uint32_t color)
{

    clear_screen(0x00, 0x00, 0x00);
    TerminalState* driver = &global_Settings.TerminalDriver;
    
    uint32_t og_write_loc = driver->buffer_write_location;
    driver->buffer_write_location = 0;
    uint32_t x = 0;
    uint32_t y = 0;
    for (driver->buffer_write_location = 0;driver->buffer_write_location < og_write_loc; driver->buffer_write_location++)
    {
        uint32_t x = terminal_buf_location_to_xpixel();
        uint32_t y = terminal_buf_location_to_ypixel();
        //log_hexval("terminal_print_buffer x", x);
        //log_hexval("terminal_print_buffer y", y);
        log_char_to_serial(driver->terminal_buf[driver->buffer_write_location]);
        uint8_t fontcode = char_code_to_fontcode(driver->terminal_buf[driver->buffer_write_location]);
       // log_hexval("terminal_print_buffer fontcode", fontcode);
        draw_character(x, y, color, fontcode);
    }
}

void terminal_write_char(char c, uint32_t color)
{
    //log_to_serial("terminal_write_char()\n");
    TerminalState* driver = &global_Settings.TerminalDriver;

   // log_hexval("driver->buffer_write_location",driver->buffer_write_location);
   // log_hexval("driver->terminal_buf_size", driver->terminal_buf_size);
    if (driver->buffer_write_location >= driver->terminal_buf_size)
    {
        log_to_serial("Scroll_down()");
        log_char_to_serial(driver->terminal_buf[0]);
        terminal_scroll_down();
        driver->terminal_buf[driver->buffer_write_location] = c;
        driver->buffer_write_location++;
        terminal_print_buffer(color);
    }
    else
    {
        uint32_t x = terminal_buf_location_to_xpixel();
        uint32_t y = terminal_buf_location_to_ypixel();
        draw_character(x, y, color, char_code_to_fontcode(c));
        driver->terminal_buf[driver->buffer_write_location] = c;
        driver->buffer_write_location++;
    }
    
}