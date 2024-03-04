#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel.h>
#include <pmm.h>
#include "video.h"
#include <panic.h>


extern KernelSettings global_Settings;


uint64_t get_pixel_location(int x, int y)
{
    struct multiboot_tag_framebuffer_common* framebuffer = global_Settings.framebuffer;
    uint64_t base = framebuffer->framebuffer_addr;
    uint32_t pitch = framebuffer->framebuffer_pitch;
    uint32_t pixel_width = framebuffer->framebuffer_bpp;
    log_hexval("bpp", pixel_width);
    uint64_t *pixel = base + (uint64_t)(y*pitch) + (uint64_t)(x*pixel_width);
    return pixel;
}


void set_pixel(int x,int y, int color) {
    struct multiboot_tag_framebuffer_common* framebuffer = global_Settings.framebuffer;
    unsigned where = x*framebuffer->framebuffer_bpp + y*framebuffer->framebuffer_pitch;
    unsigned char* screen = (unsigned char*)framebuffer->framebuffer_addr;
    log_hexval("Writing pixel to", screen);
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

    for (int y = 0; y < ymax; y += bpp)
    {
        for (int x = 0; x < xmax; x += bpp)
        {
            log_hexval("x", x);
            log_hexval("y", y);
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

    for (int i = 0; i < 10; i++)
        log_to_serial("\n");
   uint64_t total_framebuffer_size = framebuffer->framebuffer_pitch*framebuffer->framebuffer_height;
   log_hexval("framebuffer addr", framebuffer->framebuffer_addr);
   log_hexval("framebuffer size", total_framebuffer_size);
    uint64_t mapped = 0;
    while(mapped < total_framebuffer_size)
    {
        //log_hexval("mapped", mapped);
        if (!map_kernel_page(HUGEPGROUNDDOWN(framebuffer->framebuffer_addr + mapped), HUGEPGROUNDDOWN(framebuffer->framebuffer_addr + mapped)))
            panic("video_init() --> map_kernel_page() failure");
        if (!is_frame_mapped_hugepages(HUGEPGROUNDDOWN(framebuffer->framebuffer_addr + mapped), global_Settings.pml4t_kernel))
            panic("video_init() --> is_frame_mapped_hugepages() failure");
        //log_hexval("check", ((char*)HUGEPGROUNDDOWN(framebuffer->framebuffer_addr + mapped))[0]);
        mapped += HUGEPGSIZE;
    }
    log_hexval("total mapped", mapped);
    
}