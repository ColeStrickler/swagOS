#ifndef TERMINAL_H
#define TERMINAL_H
typedef struct TerminalState
{
    uint32_t current_x;
    uint32_t current_y;
    uint32_t max_x;
    uint32_t max_y;
} TerminalState;

void init_terminal();

void terminal_print_char(char c, uint32_t color);
void terminal_print_string(const char *str, uint32_t color);
#endif