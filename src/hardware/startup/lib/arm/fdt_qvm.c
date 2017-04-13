/*
 * $QNXLicenseC:
 * Copyright 2008, QNX Software Systems.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as
 * contributors under the License or as licensors under other terms.
 * Please review this entire file for other proprietary rights or license
 * notices, as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

#include "startup.h"
#include <libfdt.h>
#include <libfdt_private.h>
#include <sys/hwinfo.h>

struct intr_funcs {
	int				(*probe)(const void *fdt, int node, int cells);
	int				(*toirq)(const fdtintr_t intr, int *irq, int *type);
	int				(*fromirq)(int irq, int type, fdtintr_t intr);
};
extern int intr_fdt_probe(const void *fdt, int node, int cells, struct intr_funcs **funcs);

// This should be in some more hardware specific code, but for now
// we will include it here

static int intr_arm_probe(const void *fdt, int node, int cells) {
	if(cells < 3) {
		return 0;
	}
	if(fdt_node_check_compatible(fdt, node, "arm,cortex-a15-gic")) {
		if(fdt_node_check_compatible(fdt, node, "arm,cortex-a7-gic")) {
			return 0;
		}
	}
	return 1;
}

static int intr_arm_toirq(const fdtintr_t intr, int *irq, int *type) {
	*irq = intr[1] + 16;
	if(!intr[0]) {
		*irq += 16;
	}
	*type = (int)(intr[2] & 0x0fu);
	return 0;
}

static int intr_arm_fromirq(int irq, int type, fdtintr_t intr) {
	if(irq < 16) {
		// SGI's not allowed
		return -1;
	}
	if(irq < 32) {
		// PPI's
		intr[0] = 1;
		intr[1] = irq - 16;
	} else {
		// SPI's
		intr[0] = 0;
		intr[1] = irq - 32;
	}
	intr[2] = (int)((unsigned)type & 0xfu);
	return 3;
}

static struct intr_funcs intr_arm_funcs = {
	.probe = intr_arm_probe,
	.toirq = intr_arm_toirq,
	.fromirq = intr_arm_fromirq,
};

// End of hardware specific code, the next routine should scan down a table
// of supported interrupt controllers, but for now we will just hard code
// the arm cortex controller above

int intr_fdt_probe(const void *fdt, int node, int cells, struct intr_funcs **funcs) {
	if(intr_arm_funcs.probe(fdt, node, cells)) {
		*funcs = &intr_arm_funcs;
		return 1;
	}
	return 0;
}

#ifndef FDT_MAX_DEPTH
#define FDT_MAX_DEPTH	16
#endif
#ifndef MAX_NREG
#define MAX_NREG		16
#endif
#ifndef MAX_NINTR
#define MAX_NINTR		8
#endif

void
qvm_init(void)
{
	if(fdt && fdt_node_offset_by_compatible(fdt, -1, "qvm,hypervisor") > 0) {
		in_hvc = 1;
	}
}

void
qvm_update(void)
{
	int					node, depth;
	int					addr_cells[FDT_MAX_DEPTH];
	int					size_cells[FDT_MAX_DEPTH];
	int					intr_root = -1;
	struct intr_funcs	*funcs = 0;

	if(!fdt) {
		return;
	}

	// Find the root inerrupt controller as all qvm devices will be connected to it.
	while((intr_root = fdt_node_offset_by_prop_value(fdt, intr_root, "interrupt-controller", 0, 0)) > 0) {
		// Looking for the root (processor) controller
		if(fdt_parent_offset(fdt, intr_root) == 0) {
			int						cells;

			if(fdt_get_int(fdt, intr_root, 0, "#interrupt-cells", &cells) == 0) {
				if(intr_fdt_probe(fdt, intr_root, cells, &funcs)) {
					break;
				}
			}
		}
	}

	// Scan through the complete fdt looking for qvm,* compatible nodes
	for(node = 0, depth = 0; (node >= 0) && (depth >= 0); node = fdt_next_node(fdt, node, &depth)) {
		int			off;
		int			has_reg = 0;
		int			has_intr = 0;
		int			is_qvm = 0;
		const char	*compatible = 0;
		const char	*compatible_end = 0;

		if(depth < 0 || depth >= FDT_MAX_DEPTH) {
			continue;
		}
		addr_cells[depth] = 2;
		size_cells[depth] = 2;

		for(off = fdt_first_property_offset(fdt, node); off >= 0; off = fdt_next_property_offset(fdt, off)) {
			const void			*prop;
			const char			*name;
			int					len;

			prop = fdt_getprop_by_offset(fdt, off, &name, &len);
			if(!strcmp(name, "compatible")) {
				compatible = prop;
				compatible_end = compatible + len;
				if(depth > 0 && len > 0 && compatible_end[-1] == '\0') {
					const char			*p;

					for(p = compatible; p < compatible_end; p += strlen(p) + 1) {
						if(!strncmp(p, "qvm,", 4) && p[4] != '\0') {
							is_qvm = 1;
							break;
						}
					}
				}
			} else if(!strcmp(name, "#address-cells")) {
				if(len == sizeof(fdt32_t)) {
					addr_cells[depth] = fdt32_to_cpu(*(fdt32_t *)prop);
				}
			} else if(!strcmp(name, "#size-cells")) {
				if(len == sizeof(fdt32_t)) {
					size_cells[depth] = fdt32_to_cpu(*(fdt32_t *)prop);
				}
			} else if(!strcmp(name, "reg")) {
				has_reg = 1;
			} else if(!strcmp(name, "interrupts")) {
				has_intr = 1;
			}
		}
		if(is_qvm) {
			char		buff[64];
			const char	*name = fdt_get_name(fdt, node, 0);
			const char	*p;

			// remove trailing @xxxx and use node name as hwinfo name
			if((p = strchr(name, '@')) && p - name < sizeof buff - 1) {
				memcpy(buff, name, p - name);
				buff[p - name] = '\0';
				name = buff;
			}
			hwi_add_device(HWI_ITEM_BUS_UNKNOWN, HWI_ITEM_DEVCLASS_QVM, name, hwi_devclass_NONE);
	        if(debug_flag >= 2) {
	            kprintf("virtual device %s:", name);
	        }

			// Add memory addresses
			if(has_reg) {
				int			ac = addr_cells[depth - 1];
				int			sc = size_cells[depth - 1];
				uint64_t	addr[MAX_NREG], size[MAX_NREG];
				int			i, nreg;

				nreg = fdt_get_reg64_cells(fdt, node, 0, MAX_NREG, addr, size, ac, sc);
				for(i = 0; i < nreg; i++) {
					hwi_add_location(addr[i], size[i], 0, hwi_find_as(addr[i], 1));
			        if(debug_flag >= 2) {
			            kprintf(" %L:%L", addr[i], size[i]);
			        }
				}
			}

			// Add translated interrupts
			if(has_intr) {
				int			controller;
				fdtintr_t	intr[MAX_NINTR];
				int			i, nintr;

				nintr = fdt_get_intr(fdt, node, 0, intr, MAX_NINTR, &controller);
				if(controller == intr_root) {
					for(i = 0; i < nintr; i++) {
						int			irq, type;

						if(funcs->toirq(intr[i], &irq, &type) >= 0) {
							hwi_add_irq(irq);
					        if(debug_flag >= 2) {
					            kprintf(" irq=%d", irq);
					        }
						}
					}
				}
			}

			// Add all compatibles
			for(p = compatible; p < compatible_end; p += strlen(p) + 1) {
				hwi_tag				*tag;
				unsigned			string_idx = add_string(p);

				tag = hwi_alloc_tag(HWI_TAG_INFO(compatible));
				tag->dll.name = string_idx;
		        if(debug_flag >= 2) {
		            kprintf(" %s", p);
		        }
			}
	        if(debug_flag >= 2) {
	            kprintf("\n");
	        }
		}
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/fdt_qvm.c $ $Rev: 810394 $")
#endif
