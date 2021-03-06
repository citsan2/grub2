/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRUB_KERNEL_HEADER
#define GRUB_KERNEL_HEADER	1

#include <grub/types.h>
#include <grub/symbol.h>

enum
{
  OBJ_TYPE_ELF,
  OBJ_TYPE_MEMDISK,
  OBJ_TYPE_CONFIG
};

/* The module header.  */
struct grub_module_header
{
  /* The type of object.  */
  grub_uint8_t type;
  /* The size of object (including this header).  */
  grub_uint32_t size;
};

/* "gmim" (GRUB Module Info Magic).  */
#define GRUB_MODULE_MAGIC 0x676d696d

struct grub_module_info32
{
  /* Magic number so we know we have modules present.  */
  grub_uint32_t magic;
  /* The offset of the modules.  */
  grub_uint32_t offset;
  /* The size of all modules plus this header.  */
  grub_uint32_t size;
};

struct grub_module_info64
{
  /* Magic number so we know we have modules present.  */
  grub_uint32_t magic;
  grub_uint32_t padding;
  /* The offset of the modules.  */
  grub_uint64_t offset;
  /* The size of all modules plus this header.  */
  grub_uint64_t size;
};

#if GRUB_TARGET_SIZEOF_VOID_P == 8
#define grub_module_info grub_module_info64
#else
#define grub_module_info grub_module_info32
#endif

extern grub_addr_t grub_arch_modules_addr (void);

extern void EXPORT_FUNC(grub_module_iterate) (int (*hook) (struct grub_module_header *));

grub_addr_t grub_modules_get_end (void);

/* The start point of the C code.  */
void grub_main (void);

/* The machine-specific initialization. This must initialize memory.  */
void grub_machine_init (void);

/* The machine-specific finalization.  */
void EXPORT_FUNC(grub_machine_fini) (void);

/* The machine-specific prefix initialization.  */
void grub_machine_set_prefix (void);

/* Register all the exported symbols. This is automatically generated.  */
void grub_register_exported_symbols (void);

#if ! defined (ASM_FILE) && !defined (GRUB_MACHINE_EMU)
extern char grub_prefix[];
#endif

#endif /* ! GRUB_KERNEL_HEADER */
