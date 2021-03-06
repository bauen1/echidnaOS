#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#include <stdint.h>

typedef struct {
    uint8_t version_min;
    uint8_t version_maj;
    char *oem;
    uint32_t capabilities;
    uint16_t *vid_modes;
    uint16_t vid_mem_blocks;
    uint16_t software_rev;
    char *vendor;
    char *prod_name;
    char *prod_rev;
} __attribute__((packed)) vbe_info_struct_t;

typedef struct {
    uint8_t padding[8];
    uint16_t manufacturer_id_be;
    uint16_t edid_id_code;
    uint32_t serial_num;
    uint8_t man_week;
    uint8_t man_year;
    uint8_t edid_version;
    uint8_t edid_revision;
    uint8_t video_input_type;
    uint8_t max_hor_size;
    uint8_t max_ver_size;
    uint8_t gamma_factor;
    uint8_t dpms_flags;
    uint8_t chroma_info[10];
    uint8_t est_timings1;
    uint8_t est_timings2;
    uint8_t man_res_timing;
    uint16_t std_timing_id[8];
    uint8_t det_timing_desc1[18];
    uint8_t det_timing_desc2[18];
    uint8_t det_timing_desc3[18];
    uint8_t det_timing_desc4[18];
    uint8_t unused;
    uint8_t checksum;
} __attribute__((packed)) edid_info_struct_t;

typedef struct {
    uint8_t pad0[18];
    uint16_t res_x;
    uint16_t res_y;
    uint8_t pad1[3];
    uint8_t bpp;
    uint8_t pad2[14];
    uint32_t framebuffer;
    uint8_t pad3[212];
} __attribute__((packed)) vbe_mode_info_t;

typedef struct {
    vbe_mode_info_t *vbe_mode_info;
    uint16_t mode;
} get_vbe_t;

void plot_px(int x, int y, uint32_t hex, uint8_t which_tty);
extern uint32_t *framebuffer;
extern uint8_t vga_font[4096];
extern int edid_width;
extern int edid_height;

extern int modeset_done;


#endif
