#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <video.h>
#include <stdarg.h>
#include <kernel.h>
#include "terminal.h"
#include <serial.h>
#include <vmm.h>
#include <string.h>
#include <panic.h>
#include <spinlock.h>
extern KernelSettings global_Settings;
Spinlock terminal_lock;
#define TERMINAL_COLOR global_Settings.TerminalDriver.color
#define OG_COLOR 0xaa, 0x61, 0x3e



void terminal_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    log_hexval("B", b);
    global_Settings.TerminalDriver.color = RGB_COLOR(r,g,b);
}

void init_terminal()
{
    init_Spinlock(&terminal_lock, "TERMINAL");
    TerminalState* driver = &global_Settings.TerminalDriver;
    driver->current_x = 0;
    driver->current_y = 0;
    driver->max_x = global_Settings.framebuffer->common.framebuffer_width;
    driver->max_y = global_Settings.framebuffer->common.framebuffer_height;


    /*
        Characters are only 8bits wide, but we will allow 16 bits per character
        so that the letters are not squashing into each other
    */
    driver->terminal_buf_size = (driver->max_x * driver->max_y) /  (16*16);
    char* terminal_buf = (char*)kalloc(driver->terminal_buf_size);
    //log_hexval("terminal buf", terminal_buf);
    driver->buffer_write_location = 0;
    if (terminal_buf == NULL)
        panic("init_terminal() --> could not allocate memory for terminal driver buffer.");
    driver->terminal_buf = terminal_buf;
    terminal_set_color(OG_COLOR);
}

uint8_t char_code_to_fontcode(char c)
{
    return c - 32; 
}



void terminal_scroll_down()
{
    TerminalState* driver = &global_Settings.TerminalDriver;
    // total bytes per row /  ()
    uint32_t row_size_in_chars = (driver->max_x) / (16);
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
    // 16 pixels per character
    uint32_t x = (driver->buffer_write_location * 16) % driver->max_x;
    uint32_t y = 16 * ((driver->buffer_write_location*16)-x) / (driver->max_x);
    return y;
}

uint32_t terminal_buf_location_to_xpixel()
{
    TerminalState* driver = &global_Settings.TerminalDriver;
    // 16 pixels per character
    uint32_t x = (driver->buffer_write_location * 16) % driver->max_x;
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

void handle_endline()
{
    TerminalState* driver = &global_Settings.TerminalDriver;
    uint32_t row_size_in_chars = (driver->max_x) / (16);
    driver->buffer_write_location += row_size_in_chars - (driver->buffer_write_location % row_size_in_chars);
}

void terminal_write_char(char c, uint32_t color)
{
    //log_to_serial("terminal_write_char()\n");
    TerminalState* driver = &global_Settings.TerminalDriver;
    if (c == '\n')
    {
        handle_endline();
        if (driver->buffer_write_location >= driver->terminal_buf_size)
        {
            terminal_scroll_down();
            terminal_print_buffer(color);
        }
        return;
    }
    
        
    if (driver->buffer_write_location >= driver->terminal_buf_size)
    {
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

void terminal_print_string(char* str, uint32_t color)
{
    uint32_t i = 0;
    while(str[i])
    {
        terminal_write_char(str[i], color);
        i++;
    }
}




void handle_format_int(int arg, uint32_t color)
{
    char buf[13];
    bool use_negative = false;
    buf[12] = 0x00;
    uint16_t buf_write_loc = 0;
    if (arg == 0)
    {
        terminal_write_char('0', color);
        return;
    }

    if (arg < 0)
    {
        use_negative = true;
        arg *= -1;
    }

    buf_write_loc = 11;
    while (arg != 0) {
        buf[buf_write_loc--] = (arg % 10) + '0';
        arg /= 10;
    }
    if (use_negative)
        buf[buf_write_loc] = '-';
    else
        buf_write_loc++;
        
    terminal_print_string(&buf[buf_write_loc], color);
}

void handle_format_long(long arg, uint32_t color)
{
    char buf[14];
    bool use_negative = false;
    buf[13] = 0x00;
    uint16_t buf_write_loc = 0;
    if (arg == 0)
    {
        terminal_write_char('0', color);
        return;
    }

    buf_write_loc = 12;
    while (arg != 0) {
        buf[buf_write_loc--] = (arg % 10) + '0';
        arg /= 10;
    }

        buf_write_loc++;
        
    terminal_print_string(&buf[buf_write_loc], color);
}



void printf(const char* fmt, ...)
{
    //acquire_Spinlock(&terminal_lock);
    uint32_t i = 0;
    va_list args;
    va_start(args, fmt);
    while (fmt[i])
    {
        //char_to_serial(fmt[i]);
        //log_to_serial("\n");
        char c = fmt[i];
        if (c == 0x0)
            break;
        switch (c)
        {
            case '%':
            {
                i++;
                c = fmt[i];
                switch (c)
                {
                    case 'd':
                    {
                        int arg = va_arg(args, int);
                        handle_format_int(arg, TERMINAL_COLOR);
                        i++;
                        break;
                    }
                    case 'u':
                    {
                        long arg = va_arg(args, long);
                        handle_format_long(arg, TERMINAL_COLOR);
                        i++;
                        break;
                    }
                    case 's':
                    {
                        char* arg = va_arg(args, char*);
                        

                        terminal_print_string(arg, TERMINAL_COLOR);
                        i++;
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            default:
            {
                //log_to_serial("default\n");
                //log_char_to_serial(c);
                //log_to_serial("\n");
                terminal_write_char(c, TERMINAL_COLOR);
                i++;
                break;
            }
        }
    }
    //release_Spinlock(&terminal_lock);
}


