#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel.h>
#include <pmm.h>

#include <panic.h>
#include <font.h>


extern KernelSettings global_Settings;




uint64_t get_pixel_index(int x, int y)
{
    struct multiboot_tag_framebuffer_common* framebuffer = global_Settings.framebuffer;
    uint32_t pitch = framebuffer->framebuffer_pitch;
    uint32_t pixel_width = framebuffer->framebuffer_bpp/8;
    uint64_t pixel_index = pitch*y + pixel_width*x;
    return pixel_index;
}

void draw_character(int x, int y, uint32_t color, enum FONT_BITMAP_KEY key)
{
    if (key >= 97)
        return;
    for (int i = x; i < x+8; i++)
    {
        for (int j = y; j < y+16; j++)
        {
            if ((BITMAP_GLOBAL[key][j-y] >> (8-(i-x))) & 1)
                set_pixel(i, j, color);
        }
    }
}




void set_pixel(int x,int y, int color) {
    struct multiboot_tag_framebuffer_common* framebuffer = global_Settings.framebuffer;
    struct multiboot_tag_framebuffer* fb = global_Settings.framebuffer;
    uint64_t where = get_pixel_index(x, y);
    unsigned char* screen = (unsigned char*)framebuffer->framebuffer_addr;
    screen[where] = color & 255;              // BLUE
    screen[where + 1] = (color >> 8) & 255;   // GREEN
    screen[where + 2] = (color >> 16) & 255;  // RED
}

void clear_screen(uint8_t r, uint8_t g, uint8_t b)
{
    struct multiboot_tag_framebuffer_common* framebuffer = global_Settings.framebuffer;
    uint32_t xmax = framebuffer->framebuffer_width;
    uint32_t ymax = framebuffer->framebuffer_height;
    uint32_t bpp = framebuffer->framebuffer_bpp;

    for (int y = 0; y < ymax; y++)
    {
        for (int x = 0; x < xmax; x++)
        {
            set_pixel(x, y, RGB_COLOR(r, g, b));
        }
    }
}


void draw_rect(uint32_t top_left_x, uint32_t top_left_y, uint32_t width, uint32_t height, uint8_t r, uint8_t g, uint8_t b)
{
    struct multiboot_tag_framebuffer_common* framebuffer = global_Settings.framebuffer;
    uint32_t xmax = framebuffer->framebuffer_width;
    uint32_t ymax = framebuffer->framebuffer_height;

    if (top_left_x >= xmax || top_left_y >= ymax)
        return;

    for (int x = top_left_x; x < top_left_x + width; x++)
    {
        for (int y = top_left_y; y < top_left_y + height; y++)
        {
            if (x >= xmax || y >= ymax)
                continue;
            set_pixel(x, y, RGB_COLOR(r, g, b));
        }
    }
}





void video_init()
{

    /*
        We need the frame buffer to be mapped to the kernel's page table

        Because this memory is in MULTIBOOT_MEMORY_RESERVED region we do not care about altering the page frame bitmap
        and we just direct map its physical address
    */
   struct multiboot_tag_framebuffer_common* framebuffer = global_Settings.framebuffer;

    if (framebuffer->framebuffer_type != 1)
    {
        log_hexval("Found framebuffer type", framebuffer->type);
        panic("video_init() --> we currently only support direct RGB mode frame buffering.");
    }

    uint64_t total_framebuffer_size = framebuffer->framebuffer_pitch*framebuffer->framebuffer_height;
    uint64_t framebuffer_addr = framebuffer->framebuffer_addr;
    uint64_t mapped = 0;
    while(mapped < total_framebuffer_size)
    {
        if (!map_kernel_page(HUGEPGROUNDDOWN(framebuffer_addr), HUGEPGROUNDDOWN(framebuffer_addr), ALLOC_TYPE_DM_IO))
            panic("video_init() --> map_kernel_page() failure");
        if (!is_frame_mapped_hugepages(HUGEPGROUNDDOWN(framebuffer_addr), global_Settings.pml4t_kernel))
            panic("video_init() --> is_frame_mapped_hugepages() failure");
        //log_hexval("check", ((char*)HUGEPGROUNDDOWN(framebuffer->framebuffer_addr + mapped))[0]);
        mapped += HUGEPGSIZE;
        framebuffer_addr += HUGEPGSIZE;
    }
}