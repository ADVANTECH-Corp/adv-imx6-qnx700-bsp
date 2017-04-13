/*
 * $QNXLicenseC:
 * Copyright 2013,2015 QNX Software Systems.
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

#include "ipl.h"
#include "sdmmc.h"
#include "fat-fs.h"

/* Global pointer points to instance created by the client */
fat_sdmmc_t *fat_dev;

/*
 * Depend on the underlying SD/MMC driver and SoC, it could require the the buffer
 * is non-cacheable, or from external memory.
 * The client IPL is responsible for allocating proper buffer and pass them in through fat_init()
 */

/* file system information structure */
fs_info_t *fs_info;

/* common block buffer */
static unsigned char *blk_buf;

static unsigned char *blk;
static unsigned g_fat_sector = -1;
static unsigned cache_valid = 0;

/* memory length */
static int
strlen(const char *str)
{
	char *s = (char *)str;
	int len = 0;

	if (0 == str)
		return 0;

	while ('\0' != *s++)
		++len;

	return len;
}

/* reads a sector relative to the start of the block device */
static int
read_sector(unsigned blkno, void *buf, unsigned blkcnt)
{
	return sdmmc_read(fat_dev->ext, buf, blkno, blkcnt);
}

/* reads a sector relative to the start of the partition 0 */
static int
read_fsector(unsigned sector, void *buf, unsigned sect_cnt)
{
	return read_sector(sector + fs_info->fs_offset, buf, sect_cnt);
}

/* detects the type of FAT */
static int
fat_detect_type(bpb_t *bpb)
{
	bpb32_t *bpb32 = (bpb32_t *)bpb;

	if (GET_WORD(bpb->sig) != 0xaa55) {
		ser_putstr("BPB signature is wrong\n");
		return -1;
	}

	{
		int rc, bs, ns;

		bs = GET_WORD(bpb->sec_size);
		rc = GET_WORD(bpb->num_root_ents) * 32 + bs - 1;
		for (ns = 0; rc >= bs; ns++)
			rc -= bs;
		fs_info->root_dir_sectors = ns;
	}

	fs_info->fat_size = GET_WORD(bpb->num16_fat_secs);
	if (fs_info->fat_size == 0)
		fs_info->fat_size = GET_LONG(bpb32->num32_fat_secs);

	fs_info->total_sectors = GET_WORD(bpb->num16_secs);
	if (fs_info->total_sectors == 0)
		fs_info->total_sectors = GET_LONG(bpb->num32_secs);

	fs_info->number_of_fats = bpb->num_fats;
	fs_info->reserved_sectors = GET_WORD(bpb->num_rsvd_secs);

	fs_info->data_sectors = fs_info->total_sectors -
		(fs_info->reserved_sectors + fs_info->number_of_fats * fs_info->fat_size + fs_info->root_dir_sectors);

	fs_info->cluster_size = bpb->sec_per_clus;

	{
		int ds, cs, nc;

		ds = fs_info->data_sectors;
		cs = fs_info->cluster_size;
		for (nc = 0; ds >= cs; nc++)
			ds -= cs;
		fs_info->count_of_clusters = nc;
	}

	fs_info->root_entry_count = GET_WORD(bpb->num_root_ents);
	fs_info->fat1_start = fs_info->reserved_sectors;
	fs_info->fat2_start = fs_info->fat1_start + fs_info->fat_size;
	fs_info->root_dir_start = fs_info->fat2_start + fs_info->fat_size;
	fs_info->cluster2_start = fs_info->root_dir_start + ((fs_info->root_entry_count * 32) + (512 - 1)) / 512;

	if (fs_info->count_of_clusters < 4085) {
		return 12;
	} else if (fs_info->count_of_clusters < 65525) {
		return 16;
	} else {
		fs_info->root_dir_start = GET_LONG(bpb32->root_clus);
		return 32;
	}

	return -1;
}

/* reads the Master Boot Record to get partition information */
int
fat_parse_mbr(mbr_t *mbr, bpb_t *bpb)
{
	partition_t *pe;
	unsigned short sign;

	pe = (partition_t *)&(mbr->part_entry[0]);

	if ((sign = mbr->sign) != 0xaa55) {
		ser_putstr("	Error: MBR signature (");
		ser_puthex((unsigned int)sign);
		ser_putstr(") is wrong\n");
		return SDMMC_ERROR;
	}

	/*
	 * We don't want to include fancy code in IPL to elaborate all the
	 * complexity of CHS/LBA conversion to validate MBR
	 * Instead, we go ahead to assume it's MBR sector if it doesn't have
	 * obvious problems.
	 */
	if (GET_LONG(pe->part_size) == 0) {
		ser_putstr("	Error: partition 0 in MBR is invalid\n");
		return SDMMC_ERROR;
	}

	/* Assume valid MBR and valid partition 0 */
	fs_info->fs_offset = GET_LONG(pe->part_offset);

	if (fat_dev->verbose) {
		ser_putstr("Partition entry 0:\n");
		ser_putstr("        Boot Indicator: 0x");
		ser_puthex((unsigned int)mbr->part_entry[0].boot_ind);
		ser_putstr("\n");
		ser_putstr("        Partition type: 0x");
		ser_puthex((unsigned int)pe->os_type);
		ser_putstr("\n");
		ser_putstr("        Begin C_H_S:    ");
		ser_putdec((unsigned int)pe->beg_head);
		ser_putstr(" ");
		ser_putdec((unsigned int)pe->begin_sect);
		ser_putstr(" ");
		ser_putdec((unsigned int)pe->beg_cylinder);
		ser_putstr("\n");
		ser_putstr("        END C_H_S:      ");
		ser_putdec((unsigned int)pe->end_head);
		ser_putstr(" ");
		ser_putdec((unsigned int)pe->end_sect);
		ser_putstr(" ");
		ser_putdec((unsigned int)pe->end_cylinder);
		ser_putstr("\n");
		ser_putstr("        Start sector:   ");
		ser_putdec((unsigned int)GET_LONG(pe->part_offset));
		ser_putstr("\n");
		ser_putstr("        Partition size: ");
		ser_putdec((unsigned int)GET_LONG(pe->part_size));
		ser_putstr("\n");
	}

	/* read BPB structure */
	if (read_fsector(0, (unsigned char *)bpb, 1)) {
		ser_putstr("	Error: cannot read BPB\n");
		return SDMMC_ERROR;
	}

	/* detect the FAT type of partition 0 */
	if (-1 == (fs_info->fat_type = fat_detect_type(bpb))) {
		ser_putstr("	error detecting BPB type\n");
		return SDMMC_ERROR;
	}

	return SDMMC_OK;
}

/* converts a cluster number into sector numbers to the partition 0 */
static unsigned
cluster2fsector(unsigned cluster)
{
	return fs_info->cluster2_start + (cluster - 2) * fs_info->cluster_size;
}

/* gets the entry of the FAT for FAT12 */
static unsigned
get_fat_entry12(unsigned cluster)
{
	unsigned fat_sector = fs_info->fat1_start + ((cluster + (cluster / 2)) / SECTOR_SIZE);
	unsigned fat_offs = (cluster + (cluster / 2)) % SECTOR_SIZE;

	if (SDMMC_OK != read_fsector(fat_sector, blk, 2))
		return 0;

	if (cluster & 0x1)
		return GET_WORD(&blk[fat_offs]) >> 4;
	else
		return GET_WORD(&blk[fat_offs]) & 0xfff;
}

/* gets the entry of the FAT for FAT16 */
static unsigned
get_fat_entry16(unsigned cluster)
{
	unsigned fat_sector = fs_info->fat1_start + ((cluster * 2) / SECTOR_SIZE);
	unsigned fat_offs = (cluster * 2) % SECTOR_SIZE;
	unsigned char *data_buf = blk_buf;

	if (cache_valid && g_fat_sector == fat_sector) {
		return *(unsigned short *)(data_buf + fat_offs);
	}
	if (SDMMC_OK != read_fsector(fat_sector, data_buf, 1))
		return 0;

	g_fat_sector = fat_sector;

	return *(unsigned short *)(data_buf + fat_offs);
}

/* gets the entry of the FAT for FAT32 */
static unsigned
get_fat_entry32(unsigned cluster)
{
	unsigned fat_sector = fs_info->fat1_start + ((cluster * 4) / SECTOR_SIZE);
	unsigned fat_offs = (cluster * 4) % SECTOR_SIZE;
	unsigned char *data_buf = blk_buf;

	if (cache_valid && g_fat_sector == fat_sector) {
		return (*(unsigned long *)(data_buf + fat_offs)) & 0x0fffffff;
	}
	if (SDMMC_OK != read_fsector(fat_sector, data_buf, 1))
		return 0;

	g_fat_sector = fat_sector;
	return (*(unsigned long *)(data_buf + fat_offs)) & 0x0fffffff;
}

/* gets the entry of the FAT for the given cluster number */
static unsigned
fat_get_fat_entry(unsigned cluster)
{
	switch (fs_info->fat_type) {
	case TYPE_FAT12:
		return get_fat_entry12(cluster);
		break;

	case TYPE_FAT16:
		return get_fat_entry16(cluster);
		break;

	case TYPE_FAT32:
		return get_fat_entry32(cluster);
		break;
	}
	return 0;
}

/* checks for end of file condition */
static int
end_of_file (unsigned cluster)
{
	switch (fs_info->fat_type) {
	case TYPE_FAT12:
		return ((cluster == 0x0ff8) || (cluster == 0x0fff));
		break;

	case TYPE_FAT16:
		return ((cluster == 0xfff8) || (cluster == 0xffff));
		break;

	case TYPE_FAT32:
		return ((cluster == 0x0ffffff8) || (cluster == 0x0fffffff));
		break;
	}
	return 1;
}

/* copy a file from to a memory location */
static int
fat_copy_file(unsigned cluster, unsigned size, unsigned char *buf)
{
	int result, txf;
	unsigned prev_c, next_c, curr_c;
	int sz = (int)size;
	int cbytes = fs_info->cluster_size*SECTOR_SIZE;

	cache_valid = 1;
	g_fat_sector = -1;

	/*
	*Note that this impl assume the following:
	* 1) The max DMA transfer size is bigger than the max consolidate transfer size
	* Otherwise, we need to break down into smaller transfer.
	* 2) we always do at least one whole cluster transfer. This might overwrite the client buffer, but
	* since this is purely used for IPL, we don't care about that now.
	*/
	curr_c = cluster;
	while (sz > 0) {
		txf = cbytes;
		prev_c = curr_c;
		while (sz > txf) {
			//try consolidate contigus entry;
			next_c = fat_get_fat_entry(curr_c);
			if (next_c == (curr_c+1)) {
				txf +=cbytes;
				curr_c = next_c;
			} else {
				curr_c = next_c;
				break;
			}
		}

		//read the contig cluster out
		result = read_fsector(cluster2fsector(prev_c), buf, txf/SECTOR_SIZE);
		if (result != SDMMC_OK)
			return result;
		sz -= txf;
		buf += txf;
	}

	return SDMMC_OK;
}

/* copy file by name (FAT12/16) */
static int
copy_named_file_fat16(unsigned char *buf, char *name)
{
	unsigned dir_sector = fs_info->root_dir_start;
	int i, len;
	int ent = 0;

	while (dir_sector < fs_info->root_dir_start + fs_info->root_dir_sectors) {
		if (SDMMC_OK != read_fsector(dir_sector, blk, 1)) {
			ser_putstr("read_fsector failed!\n");
			return SDMMC_ERROR;
		}

		ent = 0;
		while (ent < SECTOR_SIZE / sizeof(dir_entry_t)) {
			dir_entry_t *dir_entry = (dir_entry_t *)&(blk[ent * sizeof(dir_entry_t)]);

			if (dir_entry->short_name[0] == ENT_END)
				break;

			if (dir_entry->short_name[0] == ENT_UNUSED) {
				ent++;
				continue;
			}

			len = strlen (name) < 11 ? strlen (name) : 11;
			for (i = 0; i < len; i++) {
				if (name[i] != dir_entry->short_name[i])
				break;
			}

			if ((i < len) || (dir_entry->clust_lo == 0)) {
				ent++;
				continue;
			}

			return fat_copy_file(dir_entry->clust_lo, dir_entry->size, buf);
		}

		if (ent < SECTOR_SIZE / sizeof(dir_entry_t))
			break;

		dir_sector++;
	}

	return SDMMC_ERROR;;
}

/* copy file by name (FAT32) */
static int
copy_named_file_fat32(unsigned char *buf, char *name)
{
	unsigned dir_clust = fs_info->root_dir_start;
	unsigned clust_sz = fs_info->cluster_size;
	unsigned dir_sector;
	int i, len;
	int ent = 0;

	while (!end_of_file(dir_clust)) {
		int sub_sect = 0;
		dir_sector = cluster2fsector(dir_clust);

		while (sub_sect < clust_sz) {
			if (SDMMC_OK != read_fsector(dir_sector + sub_sect, blk, 1)) {
				return SDMMC_ERROR;
			}

			ent = 0;
			while (ent < SECTOR_SIZE / sizeof(dir_entry_t)) {
				dir_entry_t *dir_entry = (dir_entry_t *)&(blk[ent * sizeof(dir_entry_t)]);

				if (dir_entry->short_name[0] == ENT_END)
					break;

				if (dir_entry->short_name[0] == ENT_UNUSED) {
					ent++;
					continue;
				}

				len = strlen (name) < 11 ? strlen (name) : 11;
				for (i = 0; i < len; i++) {
					if (name[i] != dir_entry->short_name[i])
						break;
				}

				if ((i < len) || (GET_CLUSTER(dir_entry) == 0)) {
					ent++;
					continue;
				}
				return fat_copy_file(GET_CLUSTER(dir_entry), dir_entry->size, buf);
			}

			if (ent < SECTOR_SIZE / sizeof(dir_entry_t))
				break;

			sub_sect++;
		}

		dir_clust = fat_get_fat_entry(dir_clust);
	}

	return SDMMC_ERROR;
}

/* copy file by name */
int
_fat_copy_named_file(unsigned char *buf, char *name)
{
	switch (fs_info->fat_type) {
	case TYPE_FAT12:
	case TYPE_FAT16:
		if (SDMMC_OK != copy_named_file_fat16(buf, name)) {
			ser_putstr("copy_named_file_fat16 failed!\n");
			return SDMMC_ERROR;
		}
		return SDMMC_OK;
		break;

	case TYPE_FAT32:
		if (SDMMC_OK != copy_named_file_fat32(buf, name)) {
			ser_putstr("copy_named_file_fat32 failed!\n");
			return SDMMC_ERROR;
		}
		return SDMMC_OK;
		break;
	}

	ser_putstr("Unsupport file system: ");
	ser_puthex(fs_info->fat_type);
	ser_putstr(" \n");
	return SDMMC_ERROR;
}

int
fat_copy_named_file(unsigned char *buf, char *name)
{
	mbr_t *mbr;
	bpb_t *bpb;

	blk = blk_buf;
	mbr = (mbr_t *)&blk[0];
	bpb = (bpb_t *)&blk[SECTOR_SIZE];

	/* read MBR from sector 0 */
	if (SDMMC_OK != read_sector(0, mbr, 1)) {
		ser_putstr("	Error: cannot read MBR\n");
		return SDMMC_ERROR;
	}

	/*
	 * If we fail to read the file based ont he assumption that
	 * sector 0 is MBR, we will try BPB
	 */
	if ((fat_parse_mbr(mbr, bpb) == SDMMC_OK) &&
		(_fat_copy_named_file(buf, name) == SDMMC_OK)) {
		return SDMMC_OK;
	}

	/* Invalid MBR, trying the BPB at the first sector */
	fs_info->fs_offset = 0;

	/* detect the FAT type of partition 0 */
	if (-1 == (fs_info->fat_type = fat_detect_type((bpb_t *)mbr))) {
		ser_putstr("	error detecting BPB type\n");
		return SDMMC_ERROR;
	}

	return (_fat_copy_named_file(buf, name));
}

int
fat_init(fat_sdmmc_t * dev)
{
	if (!dev || !dev->ext ||
		(dev->buf1_len < FAT_FS_INFO_BUF_SIZE) || (dev->buf2_len < FAT_COMMON_BUF_SIZE)) {
		return SDMMC_ERROR;
	}

	fs_info = (fs_info_t *)dev->buf1;
	blk_buf = dev->buf2;
	fat_dev = dev;

	return SDMMC_OK;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/fs/fat-fs.c $ $Rev: 797418 $")
#endif
