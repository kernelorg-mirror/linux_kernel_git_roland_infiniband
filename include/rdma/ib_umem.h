/*
 * Copyright (c) 2007 Cisco Systems.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef IB_UMEM_H
#define IB_UMEM_H

#include <linux/list.h>
#include <linux/mmu_notifier.h>
#include <linux/rbtree.h>
#include <linux/scatterlist.h>
#include <linux/workqueue.h>

struct ib_ucontext;
struct ib_umem_odp;

struct ib_umem {
	struct ib_ucontext     *context;
	size_t			length;
	unsigned long		address;
	int			page_size;
	int                     writable;
	int                     hugetlb;
	struct work_struct	work;
	struct pid             *pid;
	struct mm_struct       *mm;
	unsigned long		diff;
	struct ib_umem_odp     *odp_data;
	struct sg_table sg_head;
	int             nmap;
	int             npages;
};

/* Returns the offset of the umem start relative to the first page. */
static inline int ib_umem_offset(struct ib_umem *umem)
{
	return umem->address & ((unsigned long)umem->page_size - 1);
}

/* Returns the first page of an ODP umem. */
static inline unsigned long ib_umem_start(struct ib_umem *umem)
{
	return umem->address - ib_umem_offset(umem);
}

/* Returns the address of the page after the last one of an ODP umem. */
static inline unsigned long ib_umem_end(struct ib_umem *umem)
{
	return PAGE_ALIGN(umem->address + umem->length);
}

static inline size_t ib_umem_num_pages(struct ib_umem *umem)
{
	return (ib_umem_end(umem) - ib_umem_start(umem)) >> PAGE_SHIFT;
}

struct ib_ummunotify_range {
	unsigned long		start;
	unsigned long		end;
	struct rb_node		node;
};

#ifdef CONFIG_INFINIBAND_USER_MEM

struct ib_ummunotify_context {
	struct mmu_notifier	mmu_notifier;
	void		      (*callback)(struct ib_ummunotify_context *,
					  struct ib_ummunotify_range *);
	struct mm_struct       *mm;
	struct rb_root		reg_tree;
	spinlock_t		lock;
};

struct ib_umem *ib_umem_get(struct ib_ucontext *context, unsigned long addr,
			    size_t size, int access, int dmasync);
void ib_umem_release(struct ib_umem *umem);
int ib_umem_page_count(struct ib_umem *umem);
int ib_umem_copy_from(void *dst, struct ib_umem *umem, size_t offset,
		      size_t length);

void ib_ummunotify_register_range(struct ib_ummunotify_context *context,
				  struct ib_ummunotify_range *range);
void ib_ummunotify_unregister_range(struct ib_ummunotify_context *context,
				    struct ib_ummunotify_range *range);

int ib_ummunotify_init_context(struct ib_ummunotify_context *context,
			       void (*callback)(struct ib_ummunotify_context *,
						struct ib_ummunotify_range *));
void ib_ummunotify_cleanup_context(struct ib_ummunotify_context *context);

static inline void ib_ummunotify_clear_range(struct ib_ummunotify_range *range)
{
	RB_CLEAR_NODE(&range->node);
}

static inline void ib_ummunotify_clear_context(struct ib_ummunotify_context *context)
{
	context->mm = NULL;
}

static inline int ib_ummunotify_context_used(struct ib_ummunotify_context *context)
{
	return !!context->mm;
}

#else /* CONFIG_INFINIBAND_USER_MEM */

#include <linux/err.h>

struct ib_ummunotify_context;

static inline struct ib_umem *ib_umem_get(struct ib_ucontext *context,
					  unsigned long addr, size_t size,
					  int access, int dmasync) {
	return ERR_PTR(-EINVAL);
}
static inline void ib_umem_release(struct ib_umem *umem) { }
static inline int ib_umem_page_count(struct ib_umem *umem) { return 0; }
static inline int ib_umem_copy_from(void *dst, struct ib_umem *umem, size_t offset,
		      		    size_t length) {
	return -EINVAL;
}

static inline void ib_ummunotify_register_range(struct ib_ummunotify_context *context,
						struct ib_ummunotify_range *range) { }
static inline void ib_ummunotify_unregister_range(struct ib_ummunotify_context *context,
						  struct ib_ummunotify_range *range) { }

static inline int ib_ummunotify_init_context(struct ib_ummunotify_context *context,
					     void (*callback)(struct ib_ummunotify_context *,
							      struct ib_ummunotify_range *)) { return 0; }
static inline void ib_ummunotify_cleanup_context(struct ib_ummunotify_context *context) { }

static inline void ib_ummunotify_clear_range(struct ib_ummunotify_range *range) { }

static inline void ib_ummunotify_clear_context(struct ib_ummunotify_context *context) { }

static inline int ib_ummunotify_context_used(struct ib_ummunotify_context *context) { return 0; }

#endif /* CONFIG_INFINIBAND_USER_MEM */

#endif /* IB_UMEM_H */
