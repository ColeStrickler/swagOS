#ifndef TERMINAL_H
#define TERMINAL_H





typedef struct TerminalState
{
    uint32_t current_x;
    uint32_t current_y;
    uint32_t max_x;
    uint32_t max_y;
    char* terminal_buf;
    uint32_t terminal_buf_size;
    uint32_t buffer_write_location;
    uint32_t color;
} TerminalState;

void terminal_set_color(uint8_t r, uint8_t g, uint8_t b);

void init_terminal();

uint32_t terminal_buf_location_to_ypixel();

uint32_t terminal_buf_location_to_xpixel();

void terminal_reset();

void terminal_write_char(char c, uint32_t color);
void terminal_print_string(char *fmt, uint32_t color);
void printf(const char *fmt, ...);
#endif