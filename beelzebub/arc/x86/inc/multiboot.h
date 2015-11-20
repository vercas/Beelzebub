#pragma once

#include <metaprogramming.h>

#define MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED 0
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB     1
#define MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT	2

#define VBE_MODE_INFO ((vbe_info_t *)((uintptr_t)multiboot_info->vbe_mode_info))

/**
 * The root multiboot v1 info table.
 * 
 * Contains the address and size of both the memory map and the module list.
 */
typedef struct multiboot_info {
	uint32_t flags;
	uint32_t mem_lower;
	uint32_t mem_upper;
	uint32_t boot_device;
	uint32_t cmdline;
	uint32_t mods_count;
	uint32_t mods_addr;
	uint32_t aout_tabsz;
	uint32_t aout_strsz;
	uint32_t aout_addr;
	uint32_t aout_resv;
	uint32_t mmap_length;
	uint32_t mmap_addr;
	uint32_t drives_length;
	uint32_t config_table;
	uint32_t boot_loader_name;
	uint32_t apm_table;

	uint32_t vbe_control_info;
	uint32_t vbe_mode_info;
	uint32_t vbe_mode;
	uint32_t vbe_interface_seg;
	uint16_t vbe_interface_off;
	uint16_t vbe_interface_len;

	uint64_t framebuffer_addr;
	uint32_t framebuffer_pitch;
	uint32_t framebuffer_width;
	uint32_t framebuffer_height;
	uint8_t framebuffer_bpp;
	uint8_t framebuffer_type;

	union
	{
		struct
		{
			uint32_t framebuffer_palette_addr;
			uint16_t framebuffer_palette_num_colors;
		};
		struct
		{
			uint8_t framebuffer_red_field_position;
			uint8_t framebuffer_red_mask_size;
			uint8_t framebuffer_green_field_position;
			uint8_t framebuffer_green_mask_size;
			uint8_t framebuffer_blue_field_position;
			uint8_t framebuffer_blue_mask_size;
		};
	};
} __packed multiboot_info_t;

/**
 * Video info table.
 */
typedef struct vbe_info {
	uint16_t attributes;
	uint8_t winA, winB;
	uint16_t granularity;
	uint16_t winsize;
	uint16_t segmentA, segmentB;
	uint32_t realFctPtr;
	uint16_t pitch;
	uint16_t Xres, Yres;
	uint8_t Wchar, Ychar, planes, bpp, banks;
	uint8_t memory_model, bank_size, image_pages;
	uint8_t reserved0;
	uint8_t red_mask, red_position;
	uint8_t green_mask, green_position;
	uint8_t blue_mask, blue_position;
	uint8_t rsv_mask, rsv_position;
	uint8_t directcolor_attributes;
	uint32_t physbase;
	uint32_t reserved1;
	uint16_t reserved2;
} vbe_info_t;

/**
 * An entry in the muliboot memory map.
 * 
 * The size field contains the size of the structure, except for the field itself.
 */
typedef struct multiboot_mmap {
	uint32_t size;
	uint64_t address;
	uint64_t length;
	uint32_t type;
} __packed multiboot_mmap_t;

/**
 * An entry in the multiboot module list.
 * 
 * Represents a module that has been loaded from disk into memory.
 */
typedef struct multiboot_mod {
	uint32_t start;
	uint32_t end;
	uint32_t cmdline;
	uint32_t pad;
} __packed multiboot_mod_t;

/**
 * Pointer to the multiboot info table.
 */
extern multiboot_info_t * multiboot_info;

/**
 * Parses the multiboot info table and adds the gathered data to the
 * Jegudiel info structures.
 */
void multiboot_parse();
