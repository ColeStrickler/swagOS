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
} TerminalState;

void init_terminal();

void terminal_write_char(char c, uint32_t color);
void terminal_print_string(char *fmt, uint32_t color);
void printf(const char *fmt, ...);
#endif