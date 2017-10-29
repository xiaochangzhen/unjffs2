/*
 * unjffs2.c
 *
 * Uncompress the files , from a given JFFS2 image.
 *
 * Copyright (C) 2017 xiaochangzhen <xiaochangzhen@yeah.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * xiaochangzhen, Hangzhou, China, 2017/10/29
 */

#define PROGRAM_NAME "unjffs2"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <zlib.h>
#include <crc32.h>
#include <lzo/lzo1x.h>
#include <linux/kdev_t.h>

#define crc32 __zlib_crc32
#include <zlib.h>
#undef crc32
#include "compr.h"

#include "mtd/jffs2-user.h"
#include "common.h"

#define likely(x)   __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#define PRINT(string, args...) do { if (verbose) {printf(string, ##args);} } while (0)
#define INFO(string,args...) printf("\033[32m"string"\033[0m",##args)
#define DBG(string,args...) printf("\033[34m""%s(%d) [%s]: "string"\033[0m",__FILE__,__LINE__,__FUNCTION__,##args)
#define ERR(string,args...) fprintf(stderr,"\033[31m""%s(%d) [%s]: "string"\033[0m",__FILE__,__LINE__,__FUNCTION__,##args)

#define DASSERT(b,action) do { if(unlikely(!(b))){ ERR("debug assertion failure (%s)\n", #b);action;} } while (0)
#define ASSERT(b,action) do { if(unlikely(!(b))){ action;} } while (0)

struct jffs2_dir {
	uint32_t pino;
	uint32_t ino;
	mode_t mode;
	int16_t uid;
	int16_t gid;
	char path[PATH_MAX];
	char name[256];
	struct jffs2_dir *next;
};

struct jffs2_file_data {

	uint32_t version; 
	uint32_t ino;
	mode_t mode;     
	uint16_t uid;     
	uint16_t gid;     
	uint32_t isize;   
	uint32_t atime;   
	uint32_t mtime;   
	uint32_t ctime;   
	uint32_t offset;  
	uint32_t csize;   
	uint32_t dsize;	  
	uint8_t compr;    
	uint8_t usercompr;
	uint8_t *data;
	struct jffs2_file_data *next;	
}; 

struct jffs2_file {
	uint32_t pino;
	uint32_t ino;
	char name[256];
	uint32_t size;
	struct jffs2_file_data *data_list;
	struct jffs2_file *next;
};

struct jffs2_dir *dir_list = NULL;
struct jffs2_file *file_list = NULL;
char output_dir[256] = "jffs2";

int target_endian = __BYTE_ORDER;
static int verbose = 1; 

static struct jffs2_dir * __unjffs2_dir_copy(struct jffs2_dir *dir, struct jffs2_raw_dirent *rd)
{
	struct jffs2_dir *p = NULL;
	jint32_t name_crc32;

	if (!dir) {
		p = malloc(sizeof(struct jffs2_dir));
		DASSERT(p, return NULL);
		memset(p, 0, sizeof(struct jffs2_dir));
	} else {
		p = dir;
	}

	if (rd) {
		p->pino = je32_to_cpu(rd->pino);
		p->ino = je32_to_cpu(rd->ino);
		memcpy(p->name, rd->name, rd->nsize);
		name_crc32 = cpu_to_je32(mtd_crc32(0, p->name, strlen(p->name)));

		DASSERT(!memcmp(&name_crc32, &rd->name_crc, sizeof(name_crc32)), free(p);return NULL);
	}

	return p;
}

static struct jffs2_dir *__unjffs2_add_dir(struct jffs2_raw_dirent *rd)
{
	DASSERT(rd, return NULL);

	struct jffs2_dir *p = NULL, *step = NULL;

	if (!dir_list) {
		dir_list = 	__unjffs2_dir_copy(NULL, rd);
		return 0;
	} else if (dir_list->ino > je32_to_cpu(rd->ino)) {
		p = __unjffs2_dir_copy(NULL, rd);
		DASSERT(p, return NULL);
		p->next = dir_list;
		dir_list = p;
		return 0;
	}

	for (step = dir_list; step != NULL; step = step->next) {
		if (step->ino == je32_to_cpu(rd->ino)) {
			__unjffs2_dir_copy(step, rd);
			break;	
		} else if (!step->next) {
			p = __unjffs2_dir_copy(NULL, rd);
			DASSERT(p, return NULL);
			step->next = p;
			break;
		} else if (step->next->ino > je32_to_cpu(rd->ino)) {
			p = __unjffs2_dir_copy(NULL, rd);
			DASSERT(p, return NULL);
			p->next = step->next;
			step->next = p;
			break;
		}
	}

	return p;
}

static int __unjffs2_add_dir_attr(struct jffs2_raw_inode *ri)
{
	struct jffs2_dir *step = NULL, *tmp = NULL;

	if (!dir_list) {
		dir_list = __unjffs2_dir_copy(NULL, NULL);
		DASSERT(dir_list, return -1);
		dir_list->ino = je32_to_cpu(ri->ino);
		dir_list->mode = jemode_to_cpu(ri->mode);
		dir_list->uid = je16_to_cpu(ri->uid);
		dir_list->gid = je16_to_cpu(ri->gid);
		return 0;	
	} else if (dir_list->ino > je32_to_cpu(ri->ino)) {
		tmp = __unjffs2_dir_copy(NULL, NULL);
		DASSERT(tmp, return -1);
		tmp->ino = je32_to_cpu(ri->ino);
		tmp->mode = jemode_to_cpu(ri->mode);
		tmp->uid = je16_to_cpu(ri->uid);
		tmp->gid = je16_to_cpu(ri->gid);
		tmp->next = dir_list;
		dir_list = tmp;
		return 0;
	}

	for (step = dir_list; step != NULL; step = step->next) {
		if (je32_to_cpu(ri->ino) == step->ino) {
			step->mode = jemode_to_cpu(ri->mode);
			step->uid = je16_to_cpu(ri->uid);
			step->gid = je16_to_cpu(ri->gid);
			break;
		} else if (step->next == NULL) {
			step->next = __unjffs2_dir_copy(NULL, NULL);
			DASSERT(step->next, return -1);
			step->next->ino = je32_to_cpu(ri->ino);
			step->next->mode = jemode_to_cpu(ri->mode);
			step->next->uid = je16_to_cpu(ri->uid);
			step->next->gid = je16_to_cpu(ri->gid);
			break;	
		} else if (step->next->ino > je32_to_cpu(ri->ino)) {
			tmp = step->next;	
			step->next = __unjffs2_dir_copy(NULL, NULL);
			DASSERT(step->next, return -1);
			step->next->ino = je32_to_cpu(ri->ino);
			step->next->mode = jemode_to_cpu(ri->mode);
			step->next->uid = je16_to_cpu(ri->uid);
			step->next->gid = je16_to_cpu(ri->gid);
			step->next->next = tmp;
			break;
		}	
	}

	DASSERT(step != NULL, return -1);

	return 0;
}


static int __unjffs2_do_add_data(struct jffs2_file *file, struct jffs2_file_data *data)
{
	struct jffs2_file_data *i = NULL, *tmp = NULL;

	if (!file->data_list) {
		file->data_list = data;	
		return 0;
	} else if (file->data_list->offset > data->offset) {
		data->next = file->data_list;
		file->data_list = data;
		return 0;
	}

	for (i = file->data_list; i != NULL; i = i->next) {
		if (i->next == NULL) {
			i->next = data;	
			break;
		} else if (i->next->offset > data->offset) {
			tmp = i->next;	
			i->next = data;
			data->next = tmp;
			break;
		}
	}

	DASSERT(i != NULL, return -1);

	return 0;
}

static struct jffs2_file* __unjffs2_file_copy(struct jffs2_file *file, struct jffs2_raw_dirent *rd)
{
	struct jffs2_file *p = NULL;
	uint32_t name_crc32;

	if (!file) {
		p = malloc(sizeof(struct jffs2_file));
		DASSERT(p, return NULL);
		memset(p, 0, sizeof(struct jffs2_file));
	} else {
		p = file;
	}

	if (rd) {
		p->pino = je32_to_cpu(rd->pino);
		p->ino = je32_to_cpu(rd->ino);
		memcpy(p->name, rd->name, rd->nsize);
		name_crc32 = mtd_crc32(0, p->name, strlen(p->name));
		DASSERT(name_crc32 == je32_to_cpu(rd->name_crc), free(p);return NULL);
	}

	return p;
}

static int __unjffs2_add_data(struct jffs2_raw_inode *ri)
{
	struct jffs2_file *i = NULL;
	struct jffs2_file_data *p = NULL;

	p = malloc(sizeof(struct jffs2_file_data));
	DASSERT(p, return -1);

	memset(p, 0, sizeof(struct jffs2_file_data));

	p->ino = je32_to_cpu(ri->ino);
	p->mode = jemode_to_cpu(ri->mode);
	p->uid = je16_to_cpu(ri->uid);
	p->gid = je16_to_cpu(ri->gid);
	p->isize = je32_to_cpu(ri->isize);
	p->atime = je32_to_cpu(ri->atime);
	p->mtime = je32_to_cpu(ri->mtime);
	p->ctime = je32_to_cpu(ri->ctime);
	p->offset = je32_to_cpu(ri->offset);
	p->csize = je32_to_cpu(ri->csize);
	p->dsize = je32_to_cpu(ri->dsize);
	p->compr = ri->compr;
	p->usercompr = ri->usercompr;
	p->data = ri->data;

	if (file_list == NULL) {
		file_list = __unjffs2_file_copy(NULL, NULL);
		file_list->ino = p->ino;
		__unjffs2_do_add_data(file_list, p);
		return 0;
	}

	for (i = file_list; i != NULL; i = i->next) {
		if (i->ino == p->ino) {
			__unjffs2_do_add_data(i, p);
			break;
		} else if (i->next == NULL) {
			i->next = __unjffs2_file_copy(NULL, NULL);
			i->next->ino = p->ino;
			__unjffs2_do_add_data(i->next, p);
			break;
		}
	}

	return 0;
}



static struct jffs2_file* __unjffs2_add_file(struct jffs2_raw_dirent *rd)
{
	DASSERT(rd, return NULL);

	struct jffs2_file *i = NULL;

	if (!file_list) {
		file_list = __unjffs2_file_copy(NULL, rd);
		return file_list;
	}

	for (i = file_list; i != NULL; i = i->next) {
		if (i->ino == je32_to_cpu(rd->ino)) {
			//TODO
			if (!i->pino) {
				__unjffs2_file_copy(i, rd);	
			}
			break;
		} else if (i->next == NULL)  {
			i->next = __unjffs2_file_copy(NULL, rd);
			break;
		}
	}

	DASSERT(i != NULL, return NULL);
	
	return i;
}

static int __unjffs2_path_dir(struct jffs2_dir *list) 
{
	struct jffs2_dir *i, *j;

	for (i = list; i != NULL; i = i->next) {
		for (j = list; j != NULL; j = j->next) {
			if (i->pino == j->ino) {
				sprintf(i->path, "%s/%s", j->path, i->name);
				break;
			} else if (i->pino == 1) {
				sprintf(i->path, "/%s", i->name);
				break;
			}
		}
	}
		
	return 0;
}

static int __unjffs2_remove_dir(const char *dirname)
{
	DIR *dir;
	struct dirent *entry;
	char path[PATH_MAX];

	ASSERT(dir = opendir(dirname), return -1);

	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
			snprintf(path, (size_t) PATH_MAX, "%s/%s", dirname, entry->d_name);
			if (entry->d_type == DT_DIR) {
				__unjffs2_remove_dir(path);
			} else {
				// delete file
				unlink(path);
			}
		}
	}

	closedir(dir);

	rmdir(dirname);

	return 0;
}

static int __unjffs2_mkdir_recursive(struct jffs2_dir *list, char *rootdir)
{
	char path[PATH_MAX];
	struct jffs2_dir *step = NULL;

	__unjffs2_remove_dir(rootdir);

	DASSERT(!mkdir(rootdir, 0755), return -1);	
	PRINT("%s\n", rootdir);

	for (step = list; step != NULL; step = step->next) {
		sprintf(path, "%s%s", rootdir, step->path);
		DASSERT(!mkdir(path, step->mode), return -1);
		PRINT("%s\n", path);
		memcpy(step->path, path, strlen(path)+1);
	}
	
	return 0;
}

static char *__unjffs2_get_path(struct jffs2_dir *dlist, uint32_t ino)
{
	struct jffs2_dir *i = NULL;

	if (ino == 1) {
		return output_dir;
	}
	
	for (i = dlist; i != NULL; i = i->next) {
		if (i->ino == ino) {
			return i->path;
		}
	}

	return NULL;
}

#ifdef CONFIG_JFFS2_ZLIB
static int __unjffs2_zlib_decompress(unsigned char *data_in, unsigned char *cpage_out,
		uint32_t srclen, uint32_t destlen)
{
	z_stream strm;
	int ret;

	strm.zalloc = (void *)0;
	strm.zfree = (void *)0;

	if (Z_OK != inflateInit(&strm)) {
		return 1;
	}
	strm.next_in = data_in;
	strm.avail_in = srclen;
	strm.total_in = 0;

	strm.next_out = cpage_out;
	strm.avail_out = destlen;
	strm.total_out = 0;

	while((ret = inflate(&strm, Z_FINISH)) == Z_OK)
		;

	inflateEnd(&strm);
	return 0;
}
#endif

static int __unjffs2_rtime_decompress(unsigned char *data_in, unsigned char *cpage_out,
		__attribute__((unused)) uint32_t srclen, uint32_t destlen)
{
	short positions[256];
	int outpos = 0;
	int pos = 0;

	memset(positions,0,sizeof(positions));

	while (outpos < destlen) {
		unsigned char value;
		int backoffs;
		int repeat;

		value = data_in[pos++];
		cpage_out[outpos++] = value; /* first the verbatim copied byte */
		repeat = data_in[pos++];
		backoffs = positions[value];

		positions[value] = outpos;
		if (repeat) {
			if (backoffs + repeat >= outpos) {
				while(repeat) {
					cpage_out[outpos++] = cpage_out[backoffs++];
					repeat--;
				}
			} else {
				memcpy(&cpage_out[outpos],&cpage_out[backoffs],repeat);
				outpos += repeat;
			}
		}
	}
	return 0;
}

#ifdef CONFIG_JFFS2_LZO
static int __unjffs2_lzo_decompress(unsigned char *data_in, unsigned char *cpage_out,
				 uint32_t srclen, uint32_t destlen)
{
	int ret;
	lzo_uint dl = destlen;

	ret = lzo1x_decompress_safe(data_in,srclen, cpage_out, &dl, NULL);

	if (ret != LZO_E_OK || dl != destlen) {
		return -1;
	}

	return 0;
}
#endif


static int	__unjffs2_uncompress(uint8_t compr, uint8_t *src, uint32_t ssize, uint8_t *dst, uint32_t dsize)
{
	switch (compr) {
		case JFFS2_COMPR_NONE:
			DASSERT(dsize >= ssize, return -1);
			memcpy(dst, src, ssize);
			break;			
#ifdef CONFIG_JFFS2_RTIME
		case JFFS2_COMPR_RTIME:
			return __unjffs2_rtime_decompress(src, dst, ssize, dsize);
#endif
#ifdef CONFIG_JFFS2_ZLIB
		case  JFFS2_COMPR_ZLIB:
			return __unjffs2_zlib_decompress(src, dst, ssize, dsize);
#endif
#ifdef CONFIG_JFFS2_LZO
		case JFFS2_COMPR_LZO:
			return	__unjffs2_lzo_decompress(src, dst, ssize, dsize);
#endif
		default:
			ERR("unknown compr=%d\n", compr);
			return -1;
	}

	return 0;
}

static int __unjffs2_write_regular_file(struct jffs2_file_data *list, char *path)
{
	struct jffs2_file_data *i = NULL;
	uint8_t *dbuf = NULL;
	int count = 0;
	int fd = -1;

	fd = open(path, O_CREAT | O_RDWR, list->mode);
	DASSERT(fd > 0, return -1);

	for (i = list; i != NULL; i = i->next) {
		dbuf = malloc(i->dsize);
		DASSERT(dbuf, return -1);
		memset(dbuf, 0, i->dsize);
		DASSERT(dbuf, return -1);
		DASSERT(!__unjffs2_uncompress(i->compr, i->data, i->csize, dbuf, i->dsize), );
		count = write(fd, dbuf, i->dsize);
		free(dbuf);
		DASSERT(count == i->dsize, close(fd);return -1);
	}

	close(fd);

	return 0;
}


static int __unjffs2_write_symlink(struct jffs2_file_data *list, char *path)
{
	DASSERT(list, return -1);
	DASSERT(list->next == NULL, return -1);

	char tarpath[PATH_MAX] = {};

	memcpy(tarpath, list->data, list->dsize);

	DASSERT(!symlink(tarpath, path), return -1);

	return 0;
}

static int __unjffs2_write_pipe(struct jffs2_file_data *list, char *path)
{
	DASSERT(list, return -1);
	DASSERT(list->next == NULL, return -1);

	DASSERT(!mkfifo(path, list->mode), return -1);

	return 0;
}


static int __unjffs2_write_dev(struct jffs2_file_data *list, char *path)
{
	DASSERT(list, return -1);
	DASSERT(list->next == NULL, return -1);

	dev_t dev;
	uint16_t tmp = je16_to_cpu(*(jint16_t *)list->data);

	DASSERT(list->dsize == sizeof(tmp), return -1);

	dev = MKDEV((tmp >> 8), (tmp & 0xff));

	DASSERT(!mknod(path, list->mode, dev), return -1);

	return 0;
}

static int __unjffs2_write_files(struct jffs2_dir *dlist, struct jffs2_file *flist)
{
	DASSERT(dlist, return -1);
	DASSERT(flist, return -1);

	struct jffs2_file *i = NULL;
	char *dir = NULL;
	char path[PATH_MAX] = {};
	
	for (i = flist; i != NULL; i = i->next) {

		dir = __unjffs2_get_path(dlist, i->pino);
		sprintf(path, "%s/%s", dir, i->name);
		PRINT("%s\n", path);
		DASSERT(dir != NULL, return -1);

		DASSERT(i->data_list, continue);
		switch (i->data_list->mode & S_IFMT) {
			case S_IFREG:
				__unjffs2_write_regular_file(i->data_list, path);
				break;
			case S_IFLNK:
				__unjffs2_write_symlink(i->data_list, path);
				break;
			case S_IFIFO:
				__unjffs2_write_pipe(i->data_list, path);
				break;
			case S_IFCHR:
			case S_IFBLK:
				__unjffs2_write_dev(i->data_list, path);
				break;
			default:
				ERR("unsported %#x\n", i->data_list->mode & S_IFMT); 
				break;
		}
	}

	return 0;
}


int unjffs2_un_check(struct jffs2_unknown_node *un, uint32_t total)
{
	ASSERT(total > sizeof(struct jffs2_unknown_node), return -1);

	uint32_t hdr_crc;

	if (je16_to_cpu(un->magic) != JFFS2_MAGIC_BITMASK) {
		return -1;
	}

	hdr_crc = mtd_crc32(0, un,
			sizeof(struct jffs2_unknown_node) - 4);

	ASSERT(hdr_crc == je32_to_cpu(un->hdr_crc), return -1);

	return 0;
}

int unjffs2_rx_check(struct jffs2_raw_xref *rx, uint32_t total)
{
	uint32_t node_crc;

	if (total < sizeof(struct jffs2_raw_xref) 
		|| total <= sizeof(struct jffs2_raw_xref)) {
		ERR("err\n");
		return -1;	
	}

	node_crc = mtd_crc32(0, rx, sizeof(*rx) - 4);

	if (node_crc != je32_to_cpu(rx->node_crc)) {
		ERR("err, %#x, %#x\n", node_crc, je32_to_cpu(rx->node_crc));
		return -1;
	}
	
	return 0;
}

int unjffs2_rd_check(struct jffs2_raw_dirent *rd, uint32_t total)
{
	jint32_t node_crc, name_crc;

	if (total < sizeof(struct jffs2_raw_dirent) 
		|| total <= sizeof(struct jffs2_raw_dirent) + rd->nsize) {
		ERR("err\n");
		return -1;	
	}

	node_crc = cpu_to_je32(mtd_crc32(0, rd, sizeof(struct jffs2_raw_dirent) - 8));
	if (memcmp(&node_crc, &rd->node_crc, sizeof(jint32_t))) {
		ERR("err, %u, %u, %u\n", je32_to_cpu(node_crc), je32_to_cpu(rd->node_crc), mtd_crc32(0, rd, sizeof(*rd) - 8));
		return -1;
	}

	if (je32_to_cpu(rd->totlen) != sizeof(*rd) + rd->nsize) {
		ERR("err\n");
		return -1;
	}

	name_crc = cpu_to_je32(mtd_crc32(0, rd->name, rd->nsize));
	if (memcmp(&name_crc, &rd->name_crc, sizeof(jint32_t))) {
		ERR("err\n");
		return -1;
	}
	
	return 0;	
}

int unjffs2_ri_check(struct jffs2_raw_inode *ri, uint32_t total)
{
	jint32_t data_crc, node_crc;

	if (total < sizeof(struct jffs2_raw_inode)
		|| total < je32_to_cpu(ri->totlen) ) {
		return -1;
	}

	node_crc = cpu_to_je32(mtd_crc32(0, ri, sizeof(*ri) - 8));
	if (memcmp(&node_crc, &ri->node_crc, sizeof(jint32_t))) {
		return -1;
	}

	if (je32_to_cpu(ri->totlen) != sizeof(*ri) + je32_to_cpu(ri->csize)) {
		return -1;
	}

	data_crc = cpu_to_je32(mtd_crc32(0, ri->data, je32_to_cpu(ri->csize)));
	if (memcmp(&data_crc, &ri->data_crc, sizeof(jint32_t))) {
		return -1;
	}
	
	return 0;	
}

int unjffs2_parse_write(char *buf, uint32_t size, char *dir)
{
	DASSERT(buf, return -1) ;
	struct jffs2_raw_xref *rx = NULL;
	struct jffs2_raw_dirent *rd = NULL;
	struct jffs2_raw_inode *ri = NULL;
	struct jffs2_unknown_node *un = NULL;
	char *point = buf;
	uint32_t offset = 0;

	while (offset < size) {

		ASSERT(size - offset > sizeof(struct jffs2_unknown_node), break);

		un = (struct jffs2_unknown_node *)point;
		if (unjffs2_un_check(un, size - offset)) {
			offset++;
			point++;
			continue;
		}

		switch (je16_to_cpu(un->nodetype)) {

			case JFFS2_NODETYPE_XREF:
				rx = (struct jffs2_raw_xref *)point;
				if (!unjffs2_rx_check(rx, size-offset)) {
					offset += je32_to_cpu(rx->totlen);
					point += je32_to_cpu(rx->totlen);
				} else {
					offset++;
					point++;
				}
				break;

			case JFFS2_NODETYPE_CLEANMARKER:
				offset += sizeof(struct jffs2_unknown_node);
				point += sizeof(struct jffs2_unknown_node);
				break;

			case JFFS2_NODETYPE_DIRENT:
				rd = (struct jffs2_raw_dirent *)point;
				if (!unjffs2_rd_check(rd, size-offset)) {
					if (rd->type == DT_DIR) {
						__unjffs2_add_dir(rd);
					} else {
						__unjffs2_add_file(rd);
					}
					offset += je32_to_cpu(rd->totlen);
					point += je32_to_cpu(rd->totlen);
				} else {
					offset++;
					point++;
				}
				break;

			case JFFS2_NODETYPE_INODE:
				ri = (struct jffs2_raw_inode *)point;
				if (!unjffs2_ri_check(ri, size-offset)) {
					if (S_ISDIR(jemode_to_cpu(ri->mode))) {
						__unjffs2_add_dir_attr(ri);
					} else {
						__unjffs2_add_data(ri);
					}
					offset += je32_to_cpu(ri->totlen);
					point += je32_to_cpu(ri->totlen);
				} else {
					offset++;
					point++;
				}
				break;

			default:
				ERR("unknown node %#x\n", je16_to_cpu(un->nodetype));
				offset++;
				point++;
		}
	}

	__unjffs2_path_dir(dir_list);

	DASSERT(!(__unjffs2_mkdir_recursive(dir_list, dir)), return -1);

	DASSERT(!__unjffs2_write_files(dir_list, file_list), return -1);

	return 0;
}

int unjffs2_usage(char *name)
{
	INFO("USAGE:\t %s [OPTION] file\n", name);	
	INFO("\t\t-d: Output to the directory (default: jffs2)\n");	
	INFO("\t\t-s: Oprate in the silence\n");	
	INFO("\t\t-h, Display this help text\n");	
	
	return 0;
}

int main(int argc, char **argv)
{
	int fd, opt;
	char *buf;
	struct stat st;

	if (argc < 2)  {
		unjffs2_usage(argv[0]);	
		return -1;
	}

	while ((opt = getopt(argc, argv, "d:sh")) > 0) {

		switch (opt) {
			case 'd':
				sprintf(output_dir, "%s", optarg);
				break;
			case 's':
				verbose = 0;
				break;
			case 'h':
			default:
				unjffs2_usage(argv[0]);	
				return -1;
		}

	}

	fd = open(argv[optind], O_RDONLY);
	DASSERT(fd > 0, return -1);

	DASSERT(fstat(fd, &st) == 0, return -1);

	buf = malloc((size_t) st.st_size);
	DASSERT(buf, return -1);

	DASSERT(read(fd, buf, st.st_size) == (ssize_t) st.st_size, return -1);

	unjffs2_parse_write(buf, st.st_size, output_dir);

	return 0;
}
