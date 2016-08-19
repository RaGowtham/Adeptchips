/*
 * Remote Processor Framework
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 * Copyright (C) 2011 Google, Inc.
 *
 * Ohad Ben-Cohen <ohad@wizery.com>
 * Brian Swetland <swetland@google.com>
 * Mark Grosen <mgrosen@ti.com>
 * Fernando Guzman Lugo <fernando.lugo@ti.com>
 * Suman Anna <s-anna@ti.com>
 * Robert Tivy <rtivy@ti.com>
 * Armando Uribe De Leon <x0095078@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt)    "%s: " fmt, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/string.h>
#include <linux/debugfs.h>
#include <linux/remoteproc.h>
#include <linux/iommu.h>
#include <linux/idr.h>
#include <linux/elf.h>
#include <linux/crc32.h>
#include <linux/virtio_ids.h>
#include <linux/virtio_ring.h>
#include <linux/of.h>
#include <linux/vmalloc.h>
#include <asm/byteorder.h>

#include "remoteproc_internal.h"

static DEFINE_MUTEX(rproc_list_mutex);
static LIST_HEAD(rproc_list);

typedef int (*rproc_handle_resources_t)(struct rproc *rproc,
				struct resource_table *table, int len);
typedef int (*rproc_handle_resource_t)(struct rproc *rproc,
				 void *, int offset, int avail);

/* Unique indices for remoteproc devices */
static DEFINE_IDA(rproc_dev_index);

static const char * const rproc_crash_names[] = {
	[RPROC_MMUFAULT]	= "mmufault",
	[RPROC_WATCHDOG]	= "watchdog",
	[RPROC_EXCEPTION]	= "device exception",
};

/* translate rproc_crash_type to string */
static const char *rproc_crash_to_string(enum rproc_crash_type type)
{
	if (type < ARRAY_SIZE(rproc_crash_names))
		return rproc_crash_names[type];
	return "unknown";
}

/*
 * This is the IOMMU fault handler we register with the IOMMU API
 * (when relevant; not all remote processors access memory through
 * an IOMMU).
 *
 * IOMMU core will invoke this handler whenever the remote processor
 * will try to access an unmapped device address.
 */
static int rproc_iommu_fault(struct iommu_domain *domain, struct device *dev,
			     unsigned long iova, int flags, void *token)
{
	struct rproc *rproc = token;

	dev_err(dev, "iommu fault: da 0x%lx flags 0x%x\n", iova, flags);

	rproc_report_crash(rproc, RPROC_MMUFAULT);

	/*
	 * Let the iommu core know we're not really handling this fault;
	 * we just used it as a recovery trigger.
	 */
	return -ENOSYS;
}

static int rproc_enable_iommu(struct rproc *rproc)
{
	struct iommu_domain *domain;
	struct device *dev = rproc->dev.parent;
	int ret;

	if (!rproc->has_iommu) {
		dev_dbg(dev, "iommu not present\n");
		return 0;
	}

	domain = iommu_domain_alloc(dev->bus);
	if (!domain) {
		dev_err(dev, "can't alloc iommu domain\n");
		return -ENOMEM;
	}

	iommu_set_fault_handler(domain, rproc_iommu_fault, rproc);

	ret = iommu_attach_device(domain, dev);
	if (ret) {
		dev_err(dev, "can't attach iommu device: %d\n", ret);
		goto free_domain;
	}

	rproc->domain = domain;

	return 0;

free_domain:
	iommu_domain_free(domain);
	return ret;
}

static void rproc_disable_iommu(struct rproc *rproc)
{
	struct iommu_domain *domain = rproc->domain;
	struct device *dev = rproc->dev.parent;

	if (!domain)
		return;

	iommu_detach_device(domain, dev);
	iommu_domain_free(domain);
}

/**
 * rproc_da_to_va() - lookup the kernel virtual address for a remoteproc address
 * @rproc: handle of a remote processor
 * @da: remoteproc device address to translate
 * @len: length of the memory region @da is pointing to
 * @flags: flags to pass onto platform implementations for aiding translations
 *
 * Some remote processors will ask us to allocate them physically contiguous
 * memory regions (which we call "carveouts"), and map them to specific
 * device addresses (which are hardcoded in the firmware). They may also have
 * dedicated memory regions internal to the processors, and use them either
 * exclusively or alongside carveouts.
 *
 * They may then ask us to copy objects into specific device addresses (e.g.
 * code/data sections) or expose us certain symbols in other device address
 * (e.g. their trace buffer).
 *
 * This function is a helper function with which we can go over the allocated
 * carveouts and translate specific device addresses to kernel virtual addresses
 * so we can access the referenced memory. This function also allows to perform
 * translations on the internal remoteproc memory regions through a platform
 * implementation specific da_to_va ops, if present. The @flags field is passed
 * onto these ops to aid the translation within the ops implementation. The
 * @flags field is to be passed as a combination of the RPROC_FLAGS_xxx type
 * and the pertinent flags value for that type.
 *
 * The function returns a valid kernel address on success or NULL on failure.
 *
 * Note: phys_to_virt(iommu_iova_to_phys(rproc->domain, da)) will work too,
 * but only on kernel direct mapped RAM memory. Instead, we're just using
 * here the output of the DMA API for the carveouts, which should be more
 * correct.
 */
void *rproc_da_to_va(struct rproc *rproc, u64 da, int len, u32 flags)
{
	struct rproc_mem_entry *carveout;
	void *ptr = NULL;

	if (rproc->ops->da_to_va) {
		ptr = rproc->ops->da_to_va(rproc, da, len, flags);
		if (ptr)
			goto out;
	}

	list_for_each_entry(carveout, &rproc->carveouts, node) {
		int offset = da - carveout->da;

		/* try next carveout if da is too small */
		if (offset < 0)
			continue;

		/* try next carveout if da is too large */
		if (offset + len > carveout->len)
			continue;

		ptr = carveout->va + offset;

		break;
	}

out:
	return ptr;
}
EXPORT_SYMBOL(rproc_da_to_va);

int rproc_alloc_vring(struct rproc_vdev *rvdev, int i)
{
	struct rproc *rproc = rvdev->rproc;
	struct device *dev = &rproc->dev;
	struct rproc_vring *rvring = &rvdev->vring[i];
	struct fw_rsc_vdev *rsc;
	dma_addr_t dma;
	void *va;
	int ret, size, notifyid;

	/* actual size of vring (in bytes) */
	size = PAGE_ALIGN(vring_size(rvring->len, rvring->align));

	/*
	 * Allocate non-cacheable memory for the vring. In the future
	 * this call will also configure the IOMMU for us
	 */
	va = dma_alloc_coherent(dev->parent, size, &dma, GFP_KERNEL);
	if (!va) {
		dev_err(dev->parent, "dma_alloc_coherent failed\n");
		return -EINVAL;
	}

	/*
	 * Assign an rproc-wide unique index for this vring
	 * TODO: assign a notifyid for rvdev updates as well
	 * TODO: support predefined notifyids (via resource table)
	 */
	ret = idr_alloc(&rproc->notifyids, rvring, 0, 0, GFP_KERNEL);
	if (ret < 0) {
		dev_err(dev, "idr_alloc failed: %d\n", ret);
		dma_free_coherent(dev->parent, size, va, dma);
		return ret;
	}
	notifyid = ret;

	dev_dbg(dev, "vring%d: va %p dma %pad size %x idr %d\n", i, va,
		&dma, size, notifyid);

	rvring->va = va;
	rvring->dma = dma;
	rvring->notifyid = notifyid;

	/*
	 * Let the rproc know the notifyid and da of this vring.
	 * Not all platforms use dma_alloc_coherent to automatically
	 * set up the iommu. In this case the device address (da) will
	 * hold the physical address and not the device address.
	 */
	rsc = (void *)rproc->table_ptr + rvdev->rsc_offset;
	rsc->vring[i].da = dma;
	rsc->vring[i].notifyid = notifyid;
	return 0;
}

static int
rproc_parse_vring(struct rproc_vdev *rvdev, struct fw_rsc_vdev *rsc, int i)
{
	struct rproc *rproc = rvdev->rproc;
	struct device *dev = &rproc->dev;
	struct fw_rsc_vdev_vring *vring = &rsc->vring[i];
	struct rproc_vring *rvring = &rvdev->vring[i];

	dev_dbg(dev, "vdev rsc: vring%d: da %x, qsz %d, align %d\n",
		i, vring->da, vring->num, vring->align);

	/* make sure reserved bytes are zeroes */
	if (vring->reserved) {
		dev_err(dev, "vring rsc has non zero reserved bytes\n");
		return -EINVAL;
	}

	/* verify queue size and vring alignment are sane */
	if (!vring->num || !vring->align) {
		dev_err(dev, "invalid qsz (%d) or alignment (%d)\n",
			vring->num, vring->align);
		return -EINVAL;
	}

	rvring->len = vring->num;
	rvring->align = vring->align;
	rvring->rvdev = rvdev;

	return 0;
}

void rproc_free_vring(struct rproc_vring *rvring)
{
	int size = PAGE_ALIGN(vring_size(rvring->len, rvring->align));
	struct rproc *rproc = rvring->rvdev->rproc;
	int idx = rvring->rvdev->vring - rvring;
	struct fw_rsc_vdev *rsc;

	dma_free_coherent(rproc->dev.parent, size, rvring->va, rvring->dma);
	idr_remove(&rproc->notifyids, rvring->notifyid);

	/* reset resource entry info */
	rsc = (void *)rproc->table_ptr + rvring->rvdev->rsc_offset;
	rsc->vring[idx].da = 0;
	rsc->vring[idx].notifyid = -1;
}

/**
 * rproc_pa_to_da() - lookup the rproc device address for a physical address
 * @rproc: handle of a remote processor
 * @pa: physical address of the buffer to translate
 * @da: device address to return
 *
 * Communication clients of remote processors usually would need a means to
 * convert a host buffer pointer to an equivalent device virtual address pointer
 * that the code running on the remote processor can operate on. These buffer
 * pointers can either be from the physically contiguous memory regions (or
 * "carveouts") or can be some memory-mapped Device IO memory. This function
 * provides a means to translate a given physical address to its associated
 * device address.
 *
 * The function looks through both the carveouts and the device memory mappings
 * since both of them are stored in separate lists.
 *
 * Returns 0 on success, or an appropriate error code otherwise. The translated
 * device address is returned through the appropriate function argument.
 */
int rproc_pa_to_da(struct rproc *rproc, phys_addr_t pa, u64 *da)
{
	int ret = -EINVAL;
	struct rproc_mem_entry *maps = NULL;

	if (!rproc || !da)
		return -EINVAL;

	if (mutex_lock_interruptible(&rproc->lock))
		return -EINTR;

	if (rproc->state == RPROC_RUNNING || rproc->state == RPROC_SUSPENDED) {
		/* Look in the mappings first */
		list_for_each_entry(maps, &rproc->mappings, node) {
			if (pa >= maps->dma && pa < (maps->dma + maps->len)) {
				*da = maps->da + (pa - maps->dma);
				ret = 0;
				goto exit;
			}
		}
		/* If not, check in the carveouts */
		list_for_each_entry(maps, &rproc->carveouts, node) {
			if (pa >= maps->dma && pa < (maps->dma + maps->len)) {
				*da = maps->da + (pa - maps->dma);
				ret = 0;
				break;
			}
		}
	}
exit:
	mutex_unlock(&rproc->lock);
	return ret;
}
EXPORT_SYMBOL(rproc_pa_to_da);

/**
 * rproc_handle_vdev() - handle a vdev fw resource
 * @rproc: the remote processor
 * @rsc: the vring resource descriptor
 * @avail: size of available data (for sanity checking the image)
 *
 * This resource entry requests the host to statically register a virtio
 * device (vdev), and setup everything needed to support it. It contains
 * everything needed to make it possible: the virtio device id, virtio
 * device features, vrings information, virtio config space, etc...
 *
 * Before registering the vdev, the vrings are allocated from non-cacheable
 * physically contiguous memory. Currently we only support two vrings per
 * remote processor (temporary limitation). We might also want to consider
 * doing the vring allocation only later when ->find_vqs() is invoked, and
 * then release them upon ->del_vqs().
 *
 * Note: @da is currently not really handled correctly: we dynamically
 * allocate it using the DMA API, ignoring requested hard coded addresses,
 * and we don't take care of any required IOMMU programming. This is all
 * going to be taken care of when the generic iommu-based DMA API will be
 * merged. Meanwhile, statically-addressed iommu-based firmware images should
 * use RSC_DEVMEM resource entries to map their required @da to the physical
 * address of their base CMA region (ouch, hacky!).
 *
 * Returns 0 on success, or an appropriate error code otherwise
 */
static int rproc_handle_vdev(struct rproc *rproc, struct fw_rsc_vdev *rsc,
			     int offset, int avail)
{
	struct device *dev = &rproc->dev;
	struct rproc_vdev *rvdev;
	int i, ret;

	/* make sure resource isn't truncated */
	if (sizeof(*rsc) + rsc->num_of_vrings * sizeof(struct fw_rsc_vdev_vring)
			+ rsc->config_len > avail) {
		dev_err(dev, "vdev rsc is truncated\n");
		return -EINVAL;
	}

	/* make sure reserved bytes are zeroes */
	if (rsc->reserved[0] || rsc->reserved[1]) {
		dev_err(dev, "vdev rsc has non zero reserved bytes\n");
		return -EINVAL;
	}

	dev_dbg(dev, "vdev rsc: id %d, dfeatures %x, cfg len %d, %d vrings\n",
		rsc->id, rsc->dfeatures, rsc->config_len, rsc->num_of_vrings);

	/* we currently support only two vrings per rvdev */
	if (rsc->num_of_vrings > ARRAY_SIZE(rvdev->vring)) {
		dev_err(dev, "too many vrings: %d\n", rsc->num_of_vrings);
		return -EINVAL;
	}

	rvdev = kzalloc(sizeof(*rvdev), GFP_KERNEL);
	if (!rvdev)
		return -ENOMEM;

	rvdev->rproc = rproc;

	/* parse the vrings */
	for (i = 0; i < rsc->num_of_vrings; i++) {
		ret = rproc_parse_vring(rvdev, rsc, i);
		if (ret)
			goto free_rvdev;
	}

	/* remember the resource offset*/
	rvdev->rsc_offset = offset;

	list_add_tail(&rvdev->node, &rproc->rvdevs);

	/* it is now safe to add the virtio device */
	ret = rproc_add_virtio_dev(rvdev, rsc->id);
	if (ret)
		goto remove_rvdev;

	return 0;

remove_rvdev:
	list_del(&rvdev->node);
free_rvdev:
	kfree(rvdev);
	return ret;
}

/**
 * rproc_handle_last_trace() - setup a buffer to capture the trace snapshot
 *				before recovery
 * @rproc: the remote processor
 * @trace: the trace resource descriptor
 * @count: the index of the trace under process
 *
 * The last trace is allocated and the contents of the trace buffer are
 * copied during a recovery cleanup. Once, the contents get copied, the
 * trace buffers are cleaned up for re-use.
 *
 * It might also happen that the remoteproc binary changes between the
 * time that it was loaded and the time that it crashed. In this case,
 * the trace descriptors might have changed too. The last traces are
 * re-built as required in this case.
 *
 * Returns 0 on success, or an appropriate error code otherwise
 */
static int rproc_handle_last_trace(struct rproc *rproc,
				   struct rproc_mem_entry *trace, int count)
{
	struct rproc_mem_entry *trace_last, *tmp_trace;
	struct device *dev = &rproc->dev;
	char name[15];
	int i = 0;
	bool new_trace = false;

	if (!rproc || !trace)
		return -EINVAL;

	/* we need a new trace in this case */
	if (count > rproc->num_last_traces) {
		new_trace = true;
		/*
		 * make sure snprintf always null terminates, even if truncating
		 */
		snprintf(name, sizeof(name), "trace%d_last", (count - 1));
		trace_last = kzalloc(sizeof(*trace_last), GFP_KERNEL);
		if (!trace_last) {
			dev_err(dev, "kzalloc failed for trace%d_last\n",
				count);
			return -ENOMEM;
		}
	} else {
		/* try to reuse buffers here */
		list_for_each_entry_safe(trace_last, tmp_trace,
					 &rproc->last_traces, node) {
			if (++i == count)
				break;
		}

		/* if we can reuse the trace, copy buffer and exit */
		if (trace_last->len == trace->len)
			goto copy_and_exit;

		/* can reuse the trace struct but not the buffer */
		vfree(trace_last->va);
		trace_last->va = NULL;
		trace_last->len = 0;
	}

	trace_last->len = trace->len;
	trace_last->va = vmalloc(sizeof(u32) * trace_last->len);
	if (!trace_last->va) {
		dev_err(dev, "vmalloc failed for trace%d_last\n", count);
		if (!new_trace) {
			list_del(&trace_last->node);
			rproc->num_last_traces--;
		}
		kfree(trace_last);
		return -ENOMEM;
	}

	/* create the debugfs entry */
	if (new_trace) {
		trace_last->priv = rproc_create_trace_file(name, rproc,
				trace_last);
		if (!trace_last->priv) {
			dev_err(dev, "trace%d_last create debugfs failed\n",
				count);
			vfree(trace_last->va);
			kfree(trace_last);
			return -EINVAL;
		}

		/* add it to the trace list */
		list_add_tail(&trace_last->node, &rproc->last_traces);
		rproc->num_last_traces++;
	}

copy_and_exit:
	/* copy the trace to last trace */
	memcpy(trace_last->va, trace->va, trace->len);

	return 0;
}

/**
 * rproc_handle_trace() - handle a shared trace buffer resource
 * @rproc: the remote processor
 * @rsc: the trace resource descriptor
 * @avail: size of available data (for sanity checking the image)
 *
 * In case the remote processor dumps trace logs into memory,
 * export it via debugfs.
 *
 * Currently, the 'da' member of @rsc should contain the device address
 * where the remote processor is dumping the traces. Later we could also
 * support dynamically allocating this address using the generic
 * DMA API (but currently there isn't a use case for that).
 *
 * Returns 0 on success, or an appropriate error code otherwise
 */
static int rproc_handle_trace(struct rproc *rproc, struct fw_rsc_trace *rsc,
			      int offset, int avail)
{
	struct rproc_mem_entry *trace;
	struct device *dev = &rproc->dev;
	void *ptr;
	char name[15];

	if (sizeof(*rsc) > avail) {
		dev_err(dev, "trace rsc is truncated\n");
		return -EINVAL;
	}

	/* make sure reserved bytes are zeroes */
	if (rsc->reserved) {
		dev_err(dev, "trace rsc has non zero reserved bytes\n");
		return -EINVAL;
	}

	/* what's the kernel address of this resource ? */
	ptr = rproc_da_to_va(rproc, rsc->da, rsc->len, RPROC_FLAGS_NONE);
	if (!ptr) {
		dev_err(dev, "erroneous trace resource entry\n");
		return -EINVAL;
	}

	trace = kzalloc(sizeof(*trace), GFP_KERNEL);
	if (!trace)
		return -ENOMEM;

	/* set the trace buffer dma properties */
	trace->len = rsc->len;
	trace->va = ptr;

	/* make sure snprintf always null terminates, even if truncating */
	snprintf(name, sizeof(name), "trace%d", rproc->num_traces);

	/* create the debugfs entry */
	trace->priv = rproc_create_trace_file(name, rproc, trace);
	if (!trace->priv) {
		trace->va = NULL;
		kfree(trace);
		return -EINVAL;
	}

	list_add_tail(&trace->node, &rproc->traces);

	rproc->num_traces++;

	dev_dbg(dev, "%s added: va %p, da 0x%x, len 0x%x\n", name, ptr,
		rsc->da, rsc->len);

	return 0;
}

/**
 * rproc_handle_devmem() - handle devmem resource entry
 * @rproc: remote processor handle
 * @rsc: the devmem resource entry
 * @avail: size of available data (for sanity checking the image)
 *
 * Remote processors commonly need to access certain on-chip peripherals.
 *
 * Some of these remote processors access memory via an iommu device,
 * and might require us to configure their iommu before they can access
 * the on-chip peripherals they need.
 *
 * This resource entry is a request to map such a peripheral device.
 *
 * These devmem entries will contain the physical address of the device in
 * the 'pa' member. If a specific device address is expected, then 'da' will
 * contain it (currently this is the only use case supported). 'len' will
 * contain the size of the physical region we need to map.
 *
 * Currently we just "trust" those devmem entries to contain valid physical
 * addresses, but this is going to change: we want the implementations to
 * tell us ranges of physical addresses the firmware is allowed to request,
 * and not allow firmwares to request access to physical addresses that
 * are outside those ranges.
 */
static int rproc_handle_devmem(struct rproc *rproc, struct fw_rsc_devmem *rsc,
			       int offset, int avail)
{
	struct rproc_mem_entry *mapping;
	struct device *dev = &rproc->dev;
	int ret;

	/* no point in handling this resource without a valid iommu domain */
	if (!rproc->domain)
		return -EINVAL;

	if (sizeof(*rsc) > avail) {
		dev_err(dev, "devmem rsc is truncated\n");
		return -EINVAL;
	}

	/* make sure reserved bytes are zeroes */
	if (rsc->reserved) {
		dev_err(dev, "devmem rsc has non zero reserved bytes\n");
		return -EINVAL;
	}

	mapping = kzalloc(sizeof(*mapping), GFP_KERNEL);
	if (!mapping)
		return -ENOMEM;

	ret = iommu_map(rproc->domain, rsc->da, rsc->pa, rsc->len, rsc->flags);
	if (ret) {
		dev_err(dev, "failed to map devmem: %d\n", ret);
		goto out;
	}

	/*
	 * We'll need this info later when we'll want to unmap everything
	 * (e.g. on shutdown).
	 *
	 * We can't trust the remote processor not to change the resource
	 * table, so we must maintain this info independently.
	 */
	mapping->dma = rsc->pa;
	mapping->da = rsc->da;
	mapping->len = rsc->len;
	list_add_tail(&mapping->node, &rproc->mappings);

	dev_dbg(dev, "mapped devmem pa 0x%x, da 0x%x, len 0x%x\n",
		rsc->pa, rsc->da, rsc->len);

	return 0;

out:
	kfree(mapping);
	return ret;
}

/**
 * rproc_handle_carveout() - handle phys contig memory allocation requests
 * @rproc: rproc handle
 * @rsc: the resource entry
 * @avail: size of available data (for image validation)
 *
 * This function will handle firmware requests for allocation of physically
 * contiguous memory regions.
 *
 * These request entries should come first in the firmware's resource table,
 * as other firmware entries might request placing other data objects inside
 * these memory regions (e.g. data/code segments, trace resource entries, ...).
 *
 * Allocating memory this way helps utilizing the reserved physical memory
 * (e.g. CMA) more efficiently, and also minimizes the number of TLB entries
 * needed to map it (in case @rproc is using an IOMMU). Reducing the TLB
 * pressure is important; it may have a substantial impact on performance.
 */
static int rproc_handle_carveout(struct rproc *rproc,
				 struct fw_rsc_carveout *rsc,
				 int offset, int avail)
{
	struct rproc_mem_entry *carveout, *mapping;
	struct device *dev = &rproc->dev;
	dma_addr_t dma;
	void *va;
	int ret;

	if (sizeof(*rsc) > avail) {
		dev_err(dev, "carveout rsc is truncated\n");
		return -EINVAL;
	}

	/* make sure reserved bytes are zeroes */
	if (rsc->reserved) {
		dev_err(dev, "carveout rsc has non zero reserved bytes\n");
		return -EINVAL;
	}

	dev_dbg(dev, "carveout rsc: da %x, pa %x, len %x, flags %x\n",
		rsc->da, rsc->pa, rsc->len, rsc->flags);

	carveout = kzalloc(sizeof(*carveout), GFP_KERNEL);
	if (!carveout)
		return -ENOMEM;

	va = dma_alloc_coherent(dev->parent, rsc->len, &dma, GFP_KERNEL);
	if (!va) {
		dev_err(dev->parent, "dma_alloc_coherent err: %d\n", rsc->len);
		ret = -ENOMEM;
		goto free_carv;
	}

	dev_dbg(dev, "carveout va %p, dma %pad, len 0x%x\n", va,
		&dma, rsc->len);

	/*
	 * Ok, this is non-standard.
	 *
	 * Sometimes we can't rely on the generic iommu-based DMA API
	 * to dynamically allocate the device address and then set the IOMMU
	 * tables accordingly, because some remote processors might
	 * _require_ us to use hard coded device addresses that their
	 * firmware was compiled with.
	 *
	 * In this case, we must use the IOMMU API directly and map
	 * the memory to the device address as expected by the remote
	 * processor.
	 *
	 * Obviously such remote processor devices should not be configured
	 * to use the iommu-based DMA API: we expect 'dma' to contain the
	 * physical address in this case.
	 */
	if (rproc->domain) {
		mapping = kzalloc(sizeof(*mapping), GFP_KERNEL);
		if (!mapping) {
			dev_err(dev, "kzalloc mapping failed\n");
			ret = -ENOMEM;
			goto dma_free;
		}

		ret = iommu_map(rproc->domain, rsc->da, dma, rsc->len,
				rsc->flags);
		if (ret) {
			dev_err(dev, "iommu_map failed: %d\n", ret);
			goto free_mapping;
		}

		/*
		 * We'll need this info later when we'll want to unmap
		 * everything (e.g. on shutdown).
		 *
		 * We can't trust the remote processor not to change the
		 * resource table, so we must maintain this info independently.
		 */
		mapping->da = rsc->da;
		mapping->len = rsc->len;
		list_add_tail(&mapping->node, &rproc->mappings);

		dev_dbg(dev, "carveout mapped 0x%x to %pad\n",
			rsc->da, &dma);
	}

	/*
	 * Some remote processors might need to know the pa
	 * even though they are behind an IOMMU. E.g., OMAP4's
	 * remote M3 processor needs this so it can control
	 * on-chip hardware accelerators that are not behind
	 * the IOMMU, and therefor must know the pa.
	 *
	 * Generally we don't want to expose physical addresses
	 * if we don't have to (remote processors are generally
	 * _not_ trusted), so we might want to do this only for
	 * remote processor that _must_ have this (e.g. OMAP4's
	 * dual M3 subsystem).
	 *
	 * Non-IOMMU processors might also want to have this info.
	 * In this case, the device address and the physical address
	 * are the same.
	 */
	rsc->pa = dma;

	carveout->va = va;
	carveout->len = rsc->len;
	carveout->dma = dma;
	carveout->da = rsc->da;

	list_add_tail(&carveout->node, &rproc->carveouts);

	return 0;

free_mapping:
	kfree(mapping);
dma_free:
	dma_free_coherent(dev->parent, rsc->len, va, dma);
free_carv:
	kfree(carveout);
	return ret;
}

/**
 * rproc_handle_intmem() - placeholder to handle internal memory resource entry
 * @rproc: rproc handle
 * @rsc: the intmem resource entry
 * @offset: offset of the resource data in resource table
 * @avail: size of available data (for image validation)
 *
 * ** DEPRECATION WARNING **
 * This is a dummy function, it is only being defined to print a deprecation
 * warning for prior product firmwares so that they can stop using this resource
 * type and move away from using it.
 */
static int rproc_handle_intmem(struct rproc *rproc, struct fw_rsc_intmem *rsc,
			       int offset, int avail)
{
	WARN(1, "RSC_INTMEM is deprecated. Please do not use this resource type to support loading into internal memories.\n");
	return 0;
}

/**
 * rproc_handle_custom_rsc() - provide implementation specific hook
 *			       to handle custom resources
 * @rproc: the remote processor
 * @rsc: custom resource to be handled by drivers
 * @offset: offset of the resource data in resource table
 * @avail: size of available data
 *
 * Remoteproc implementations might want to add resource table
 * entries that are not generic enough to be handled by the framework.
 * This provides a hook to handle such custom resources.
 *
 * Returns 0 on success, or an appropriate error code otherwise
 */
static int rproc_handle_custom_rsc(struct rproc *rproc,
				   struct fw_rsc_custom *rsc,
				   int offset, int avail)
{
	struct device *dev = &rproc->dev;

	if (!rproc->ops->handle_custom_rsc) {
		dev_err(dev, "custom resource handler unavailable\n");
		return -EINVAL;
	}

	if (sizeof(*rsc) > avail) {
		dev_err(dev, "custom resource is truncated\n");
		return -EINVAL;
	}

	return rproc->ops->handle_custom_rsc(rproc, (void *)rsc);
}

static int rproc_count_vrings(struct rproc *rproc, struct fw_rsc_vdev *rsc,
			      int offset, int avail)
{
	/* Summarize the number of notification IDs */
	rproc->max_notifyid += rsc->num_of_vrings;

	return 0;
}

/*
 * A lookup table for resource handlers. The indices are defined in
 * enum fw_resource_type.
 */
static rproc_handle_resource_t rproc_loading_handlers[RSC_LAST] = {
	[RSC_CARVEOUT] = (rproc_handle_resource_t)rproc_handle_carveout,
	[RSC_DEVMEM] = (rproc_handle_resource_t)rproc_handle_devmem,
	[RSC_TRACE] = (rproc_handle_resource_t)rproc_handle_trace,
	[RSC_INTMEM] = (rproc_handle_resource_t)rproc_handle_intmem,
	[RSC_VDEV] = NULL, /* VDEVs were handled upon registrarion */
};

static rproc_handle_resource_t rproc_vdev_handler[RSC_LAST] = {
	[RSC_VDEV] = (rproc_handle_resource_t)rproc_handle_vdev,
};

static rproc_handle_resource_t rproc_count_vrings_handler[RSC_LAST] = {
	[RSC_VDEV] = (rproc_handle_resource_t)rproc_count_vrings,
};

static rproc_handle_resource_t rproc_post_loading_handlers[RSC_LAST] = {
	[RSC_CUSTOM] = (rproc_handle_resource_t)rproc_handle_custom_rsc,
};

/* handle firmware resource entries before booting the remote processor */
static int rproc_handle_resources(struct rproc *rproc, int len,
				  rproc_handle_resource_t handlers[RSC_LAST])
{
	struct device *dev = &rproc->dev;
	rproc_handle_resource_t handler;
	int ret = 0, i;

	for (i = 0; i < rproc->table_ptr->num; i++) {
		int offset = rproc->table_ptr->offset[i];
		struct fw_rsc_hdr *hdr = (void *)rproc->table_ptr + offset;
		int avail = len - offset - sizeof(*hdr);
		void *rsc = (void *)hdr + sizeof(*hdr);

		/* make sure table isn't truncated */
		if (avail < 0) {
			dev_err(dev, "rsc table is truncated\n");
			return -EINVAL;
		}

		dev_dbg(dev, "rsc: type %d\n", hdr->type);

		if (hdr->type >= RSC_LAST) {
			dev_warn(dev, "unsupported resource %d\n", hdr->type);
			continue;
		}

		handler = handlers[hdr->type];
		if (!handler)
			continue;

		ret = handler(rproc, rsc, offset + sizeof(*hdr), avail);
		if (ret)
			break;
	}

	return ret;
}

/* handle firmware version entry before loading the firmware sections */
static int
rproc_handle_fw_version(struct rproc *rproc, const char *version, int versz)
{
	struct device *dev = &rproc->dev;

	rproc->fw_version = kmemdup(version, versz, GFP_KERNEL);
	if (!rproc->fw_version) {
		dev_err(dev, "%s: version kzalloc failed\n", __func__);
		return -ENOMEM;
	}
	return 0;
}

/**
 * rproc_free_last_trace() - helper function to cleanup a last trace entry
 * @trace: the last trace element to be cleaned up
 */
static void rproc_free_last_trace(struct rproc_mem_entry *trace)
{
	rproc_remove_trace_file(trace->priv);
	list_del(&trace->node);
	vfree(trace->va);
	kfree(trace);
}

/**
 * rproc_resource_cleanup() - clean up and free all acquired resources
 * @rproc: rproc handle
 *
 * This function will free all resources acquired for @rproc, and it
 * is called whenever @rproc either shuts down or fails to boot.
 */
static void rproc_resource_cleanup(struct rproc *rproc)
{
	struct rproc_mem_entry *entry, *tmp;
	struct device *dev = &rproc->dev;
	int count = 0, i = rproc->num_traces;

	/* clean up debugfs trace entries */
	list_for_each_entry_safe(entry, tmp, &rproc->traces, node) {
		/* handle last trace here */
		if (rproc->state == RPROC_CRASHED)
			rproc_handle_last_trace(rproc, entry, ++count);

		rproc_remove_trace_file(entry->priv);
		list_del(&entry->node);
		kfree(entry);
	}
	rproc->num_traces = 0;

	/*
	 * clean up debugfs last trace entries. This either deletes all last
	 * trace entries during cleanup or just the remaining entries, if any,
	 * in case of a crash.
	 */
	list_for_each_entry_safe(entry, tmp, &rproc->last_traces, node) {
		/* skip the valid traces */
		if ((i--) && (rproc->state == RPROC_CRASHED))
			continue;
		rproc_free_last_trace(entry);
		rproc->num_last_traces--;
	}

	/* clean up iommu mapping entries */
	list_for_each_entry_safe(entry, tmp, &rproc->mappings, node) {
		size_t unmapped;

		unmapped = iommu_unmap(rproc->domain, entry->da, entry->len);
		if (unmapped != entry->len) {
			/* nothing much to do besides complaining */
			dev_err(dev, "failed to unmap %u/%zu\n", entry->len,
				unmapped);
		}

		list_del(&entry->node);
		kfree(entry);
	}

	/* clean up carveout allocations */
	list_for_each_entry_safe(entry, tmp, &rproc->carveouts, node) {
		dma_free_coherent(dev->parent, entry->len, entry->va,
				  entry->dma);
		list_del(&entry->node);
		kfree(entry);
	}

	/* free fw version */
	kfree(rproc->fw_version);
}

/*
 * take a firmware and boot a remote processor with it.
 */
static int rproc_fw_boot(struct rproc *rproc, const struct firmware *fw)
{
	struct device *dev = &rproc->dev;
	const char *name = rproc->firmware;
	struct resource_table *table, *loaded_table;
	int ret, tablesz, versz;
	const char *version;

	if (!rproc->table_ptr)
		return -ENOMEM;

	ret = rproc_fw_sanity_check(rproc, fw);
	if (ret)
		return ret;

	if (!rproc->use_userspace_loader)
		dev_info(dev, "Booting fw image %s, size %zd\n",
			 name, fw->size);
	else
		dev_info(dev, "Booting unspecified pre-loaded fw image\n");

	/*
	 * if enabling an IOMMU isn't relevant for this rproc, this is
	 * just a nop
	 */
	ret = rproc_enable_iommu(rproc);
	if (ret) {
		dev_err(dev, "can't enable iommu: %d\n", ret);
		return ret;
	}

	rproc->bootaddr = rproc_get_boot_addr(rproc, fw);
	ret = -EINVAL;

	/* look for the resource table */
	table = rproc_find_rsc_table(rproc, fw, &tablesz);
	if (!table)
		goto clean_up;

	/* Verify that resource table in loaded fw is unchanged */
	if (rproc->table_csum != crc32(0, table, tablesz)) {
		dev_err(dev, "resource checksum failed, fw changed?\n");
		goto clean_up;
	}

	/* handle fw resources which are required to boot rproc */
	ret = rproc_handle_resources(rproc, tablesz, rproc_loading_handlers);
	if (ret) {
		dev_err(dev, "Failed to process resources: %d\n", ret);
		goto clean_up;
	}

	/* look for the firmware version, and store if present */
	version = rproc_find_version_info(rproc, fw, &versz);
	if (version) {
		ret = rproc_handle_fw_version(rproc, version, versz);
		if (ret) {
			dev_err(dev, "Failed to process version info: %d\n",
				ret);
			goto clean_up;
		}
	}
	if (!rproc->use_userspace_loader) {
		/* load the ELF segments to memory */
		ret = rproc_load_segments(rproc, fw);
		if (ret) {
			dev_err(dev, "Failed to load program segments: %d\n",
				ret);
			goto clean_up;
		}
	}

	/*
	 * The starting device has been given the rproc->cached_table as the
	 * resource table. The address of the vring along with the other
	 * allocated resources (carveouts etc) is stored in cached_table.
	 * In order to pass this information to the remote device we must
	 * copy this information to device memory.
	 */
	loaded_table = rproc_find_loaded_rsc_table(rproc, fw);
	if (!loaded_table) {
		ret = -EINVAL;
		goto clean_up;
	}

	memcpy(loaded_table, rproc->cached_table, tablesz);

	/* handle fw resources which require fw segments to be loaded*/
	ret = rproc_handle_resources(rproc, tablesz,
				     rproc_post_loading_handlers);
	if (ret) {
		dev_err(dev, "Failed to process post-loading resources: %d\n",
			ret);
		goto clean_up;
	}

	/* power up the remote processor */
	ret = rproc->ops->start(rproc);
	if (ret) {
		dev_err(dev, "can't start rproc %s: %d\n", rproc->name, ret);
		goto clean_up;
	}

	/*
	 * Update table_ptr so that all subsequent vring allocations and
	 * virtio fields manipulation update the actual loaded resource table
	 * in device memory.
	 */
	rproc->table_ptr = loaded_table;

	rproc->state = RPROC_RUNNING;

	dev_info(dev, "remote processor %s is now up\n", rproc->name);

	return 0;

clean_up:
	rproc_resource_cleanup(rproc);
	rproc_disable_iommu(rproc);
	return ret;
}

/*
 * take a firmware and look for virtio devices to register.
 *
 * Note: this function is called asynchronously upon registration of the
 * remote processor (so we must wait until it completes before we try
 * to unregister the device. one other option is just to use kref here,
 * that might be cleaner).
 */
static void rproc_fw_config_virtio(const struct firmware *fw, void *context)
{
	struct rproc *rproc = context;
	struct resource_table *table;
	int ret, tablesz;

	if (rproc_fw_sanity_check(rproc, fw) < 0)
		goto out;

	/* look for the resource table */
	table = rproc_find_rsc_table(rproc, fw,  &tablesz);
	if (!table)
		goto out;

	rproc->table_csum = crc32(0, table, tablesz);

	/*
	 * Create a copy of the resource table. When a virtio device starts
	 * and calls vring_new_virtqueue() the address of the allocated vring
	 * will be stored in the cached_table. Before the device is started,
	 * cached_table will be copied into devic memory.
	 */
	rproc->cached_table = kmemdup(table, tablesz, GFP_KERNEL);
	if (!rproc->cached_table)
		goto out;

	rproc->table_ptr = rproc->cached_table;

	/* count the number of notify-ids */
	rproc->max_notifyid = -1;
	ret = rproc_handle_resources(rproc, tablesz,
				     rproc_count_vrings_handler);
	if (ret)
		goto out;

	/* look for virtio devices and register them */
	ret = rproc_handle_resources(rproc, tablesz, rproc_vdev_handler);

out:
	release_firmware(fw);
	if (!rproc->use_userspace_loader) {
		/* allow rproc_del() contexts, if any, to proceed */
		complete_all(&rproc->firmware_loading_complete);
	}
}

/**
 * rproc_add_vdevs_direct() - add virtio devices directly from a resource table
 * @rproc: handle of a remote processor
 *
 * This function is added to support remoteproc drivers using userspace loaders
 * to process a published resource table directly and add the virtio devices for
 * supporting the virtio rpmsg infrastructure. The firmwares are not looked up
 * and processed by the remoteproc core for such drivers.
 *
 * The remoteproc drivers are expected to undo the addition using the
 * @rproc_remove_vdevs_direct() function.
 */
void rproc_add_vdevs_direct(struct rproc *rproc)
{
	if (!rproc->use_userspace_loader) {
		dev_err(&rproc->dev, "cannot be called on rprocs supporting in-kernel loader!\n");
		return;
	}

	rproc_fw_config_virtio(NULL, (void *)rproc);
}
EXPORT_SYMBOL(rproc_add_vdevs_direct);

/**
 * rproc_remove_vdevs_direct() - remove virtio devices directly
 * @rproc: handle of a remote processor
 *
 * This function is added to support remoteproc drivers using userspace
 * loaders to remove any directly added virtio devices through the
 * @rproc_add_vdevs_direct() function.
 */
void rproc_remove_vdevs_direct(struct rproc *rproc)
{
	struct rproc_vdev *rvdev, *tmp;

	if (!rproc->use_userspace_loader) {
		dev_err(&rproc->dev, "cannot be called on rprocs supporting in-kernel loader!\n");
		return;
	}

	list_for_each_entry_safe(rvdev, tmp, &rproc->rvdevs, node)
		rproc_remove_virtio_dev(rvdev);
}
EXPORT_SYMBOL(rproc_remove_vdevs_direct);

static int rproc_add_virtio_devices(struct rproc *rproc)
{
	int ret;

	/* nothing to do if relying on external userspace loader */
	if (rproc->use_userspace_loader)
		return 0;

	/* rproc_del() calls must wait until async loader completes */
	init_completion(&rproc->firmware_loading_complete);

	/*
	 * We must retrieve early virtio configuration info from
	 * the firmware (e.g. whether to register a virtio device,
	 * what virtio features does it support, ...).
	 *
	 * We're initiating an asynchronous firmware loading, so we can
	 * be built-in kernel code, without hanging the boot process.
	 */
	ret = request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
				      rproc->firmware, &rproc->dev, GFP_KERNEL,
				      rproc, rproc_fw_config_virtio);
	if (ret < 0) {
		dev_err(&rproc->dev, "request_firmware_nowait err: %d\n", ret);
		complete_all(&rproc->firmware_loading_complete);
	}

	return ret;
}

/**
 * rproc_trigger_recovery() - recover a remoteproc
 * @rproc: the remote processor
 *
 * The recovery is done by resetting all the virtio devices, that way all the
 * rpmsg drivers will be reseted along with the remote processor making the
 * remoteproc functional again.
 *
 * This function can sleep, so it cannot be called from atomic context.
 */
int rproc_trigger_recovery(struct rproc *rproc)
{
	struct rproc_vdev *rvdev, *rvtmp;

	dev_err(&rproc->dev, "recovering %s\n", rproc->name);

	init_completion(&rproc->crash_comp);

	/* clean up remote vdev entries */
	list_for_each_entry_safe(rvdev, rvtmp, &rproc->rvdevs, node)
		rproc_remove_virtio_dev(rvdev);

	/* wait until there is no more rproc users */
	wait_for_completion(&rproc->crash_comp);

	/* Free the copy of the resource table */
	kfree(rproc->cached_table);

	return rproc_add_virtio_devices(rproc);
}

/**
 * rproc_crash_handler_work() - handle a crash
 *
 * This function needs to handle everything related to a crash, like cpu
 * registers and stack dump, information to help to debug the fatal error, etc.
 */
static void rproc_crash_handler_work(struct work_struct *work)
{
	struct rproc *rproc = container_of(work, struct rproc, crash_handler);
	struct device *dev = &rproc->dev;

	dev_dbg(dev, "enter %s\n", __func__);

	mutex_lock(&rproc->lock);

	if (rproc->state == RPROC_CRASHED || rproc->state == RPROC_OFFLINE) {
		/* handle only the first crash detected */
		mutex_unlock(&rproc->lock);
		return;
	}

	rproc->state = RPROC_CRASHED;
	dev_err(dev, "handling crash #%u in %s\n", ++rproc->crash_cnt,
		rproc->name);

	mutex_unlock(&rproc->lock);

	if (!rproc->recovery_disabled)
		rproc_trigger_recovery(rproc);
}

/**
 * rproc_get_alias_id() - return the alias id for the rproc device
 * @rproc: handle of a remote processor
 *
 * Each rproc device is associated with a platform device, but since the
 * devices are created from device tree, they do not have a valid platform
 * device id. This function returns an alternate DT-based alias id, and is
 * useful for clients needing to know an id for a processor. It is assumed
 * that the devices were given proper alias ids.
 *
 * Return: alias id associated with the rproc
 */
int rproc_get_alias_id(struct rproc *rproc)
{
	struct device *dev = rproc->dev.parent;
	struct device_node *np = dev->of_node;

	return of_alias_get_id(np, "rproc");
}
EXPORT_SYMBOL(rproc_get_alias_id);

/**
 * rproc_boot() - boot a remote processor
 * @rproc: handle of a remote processor
 *
 * Boot a remote processor (i.e. load its firmware, power it on, ...).
 *
 * If the remote processor is already powered on, this function immediately
 * returns (successfully).
 *
 * Returns 0 on success, and an appropriate error value otherwise.
 */
int rproc_boot(struct rproc *rproc)
{
	const struct firmware *firmware_p = NULL;
	struct device *dev;
	int ret;

	if (!rproc) {
		pr_err("invalid rproc handle\n");
		return -EINVAL;
	}

	dev = &rproc->dev;

	ret = mutex_lock_interruptible(&rproc->lock);
	if (ret) {
		dev_err(dev, "can't lock rproc %s: %d\n", rproc->name, ret);
		return ret;
	}

	/* loading a firmware is required */
	if (!rproc->firmware) {
		dev_err(dev, "%s: no firmware to load\n", __func__);
		ret = -EINVAL;
		goto unlock_mutex;
	}

	/* prevent underlying implementation from being removed */
	if (!try_module_get(dev->parent->driver->owner)) {
		dev_err(dev, "%s: can't get owner\n", __func__);
		ret = -EINVAL;
		goto unlock_mutex;
	}

	/* skip the boot process if rproc is already powered up */
	if (atomic_inc_return(&rproc->power) > 1) {
		ret = 0;
		goto unlock_mutex;
	}

	dev_info(dev, "powering up %s\n", rproc->name);

	if (!rproc->use_userspace_loader) {
		/* load firmware */
		ret = request_firmware(&firmware_p, rproc->firmware, dev);
		if (ret < 0) {
			dev_err(dev, "request_firmware failed: %d\n", ret);
			goto downref_rproc;
		}
	}

	ret = rproc_fw_boot(rproc, firmware_p);

	if (!rproc->use_userspace_loader)
		release_firmware(firmware_p);

downref_rproc:
	if (ret) {
		module_put(dev->parent->driver->owner);
		atomic_dec(&rproc->power);
	}
unlock_mutex:
	mutex_unlock(&rproc->lock);
	return ret;
}
EXPORT_SYMBOL(rproc_boot);

/**
 * rproc_shutdown() - power off the remote processor
 * @rproc: the remote processor
 *
 * Power off a remote processor (previously booted with rproc_boot()).
 *
 * In case @rproc is still being used by an additional user(s), then
 * this function will just decrement the power refcount and exit,
 * without really powering off the device.
 *
 * Every call to rproc_boot() must (eventually) be accompanied by a call
 * to rproc_shutdown(). Calling rproc_shutdown() redundantly is a bug.
 *
 * Notes:
 * - we're not decrementing the rproc's refcount, only the power refcount.
 *   which means that the @rproc handle stays valid even after rproc_shutdown()
 *   returns, and users can still use it with a subsequent rproc_boot(), if
 *   needed.
 */
void rproc_shutdown(struct rproc *rproc)
{
	struct device *dev = &rproc->dev;
	int ret;

	ret = mutex_lock_interruptible(&rproc->lock);
	if (ret) {
		dev_err(dev, "can't lock rproc %s: %d\n", rproc->name, ret);
		return;
	}

	/* if the remote proc is still needed, bail out */
	if (!atomic_dec_and_test(&rproc->power))
		goto out;

	/* power off the remote processor */
	ret = rproc->ops->stop(rproc);
	if (ret) {
		atomic_inc(&rproc->power);
		dev_err(dev, "can't stop rproc: %d\n", ret);
		goto out;
	}

	/* clean up all acquired resources */
	rproc_resource_cleanup(rproc);

	rproc_disable_iommu(rproc);

	/* Give the next start a clean resource table */
	rproc->table_ptr = rproc->cached_table;

	/* if in crash state, unlock crash handler */
	if (rproc->state == RPROC_CRASHED)
		complete_all(&rproc->crash_comp);

	rproc->state = RPROC_OFFLINE;

	dev_info(dev, "stopped remote processor %s\n", rproc->name);

out:
	mutex_unlock(&rproc->lock);
	if (!ret)
		module_put(dev->parent->driver->owner);
}
EXPORT_SYMBOL(rproc_shutdown);

/**
 * rproc_get_by_phandle() - find a remote processor by phandle
 * @phandle: phandle to the rproc
 *
 * Finds an rproc handle using the remote processor's phandle, and then
 * return a handle to the rproc.
 *
 * This function increments the remote processor's refcount, so always
 * use rproc_put() to decrement it back once rproc isn't needed anymore.
 *
 * Returns the rproc handle on success, and NULL on failure.
 */
#ifdef CONFIG_OF
struct rproc *rproc_get_by_phandle(phandle phandle)
{
	struct rproc *rproc = NULL, *r;
	struct device_node *np;

	np = of_find_node_by_phandle(phandle);
	if (!np)
		return NULL;

	mutex_lock(&rproc_list_mutex);
	list_for_each_entry(r, &rproc_list, node) {
		if (r->dev.parent && r->dev.parent->of_node == np) {
			rproc = r;
			get_device(&rproc->dev);
			break;
		}
	}
	mutex_unlock(&rproc_list_mutex);

	of_node_put(np);

	return rproc;
}
#else
struct rproc *rproc_get_by_phandle(phandle phandle)
{
	return NULL;
}
#endif
EXPORT_SYMBOL(rproc_get_by_phandle);

/**
 * rproc_add() - register a remote processor
 * @rproc: the remote processor handle to register
 *
 * Registers @rproc with the remoteproc framework, after it has been
 * allocated with rproc_alloc().
 *
 * This is called by the platform-specific rproc implementation, whenever
 * a new remote processor device is probed.
 *
 * Returns 0 on success and an appropriate error code otherwise.
 *
 * Note: this function initiates an asynchronous firmware loading
 * context, which will look for virtio devices supported by the rproc's
 * firmware.
 *
 * If found, those virtio devices will be created and added, so as a result
 * of registering this remote processor, additional virtio drivers might be
 * probed.
 */
int rproc_add(struct rproc *rproc)
{
	struct device *dev = &rproc->dev;
	int ret;

	ret = device_add(dev);
	if (ret < 0)
		return ret;

	dev_info(dev, "%s is available\n", rproc->name);

	dev_info(dev, "Note: remoteproc is still under development and considered experimental.\n");
	dev_info(dev, "THE BINARY FORMAT IS NOT YET FINALIZED, and backward compatibility isn't yet guaranteed.\n");

	/* create debugfs entries */
	rproc_create_debug_dir(rproc);
	ret = rproc_add_virtio_devices(rproc);
	if (ret < 0)
		return ret;

	/* expose to rproc_get_by_phandle users */
	mutex_lock(&rproc_list_mutex);
	list_add(&rproc->node, &rproc_list);
	mutex_unlock(&rproc_list_mutex);

	return 0;
}
EXPORT_SYMBOL(rproc_add);

/**
 * rproc_type_release() - release a remote processor instance
 * @dev: the rproc's device
 *
 * This function should _never_ be called directly.
 *
 * It will be called by the driver core when no one holds a valid pointer
 * to @dev anymore.
 */
static void rproc_type_release(struct device *dev)
{
	struct rproc *rproc = container_of(dev, struct rproc, dev);
	struct rproc_mem_entry *entry, *tmp;

	dev_info(&rproc->dev, "releasing %s\n", rproc->name);

	/* clean up debugfs last trace entries */
	list_for_each_entry_safe(entry, tmp, &rproc->last_traces, node) {
		rproc_free_last_trace(entry);
		rproc->num_last_traces--;
	}

	rproc_delete_debug_dir(rproc);

	idr_destroy(&rproc->notifyids);

	if (rproc->index >= 0)
		ida_simple_remove(&rproc_dev_index, rproc->index);

	kfree(rproc);
}

struct device_type rproc_type = {
	.name		= "remoteproc",
	.release	= rproc_type_release,
};

/**
 * rproc_alloc() - allocate a remote processor handle
 * @dev: the underlying device
 * @name: name of this remote processor
 * @ops: platform-specific handlers (mainly start/stop)
 * @firmware: name of firmware file to load, can be NULL
 * @len: length of private data needed by the rproc driver (in bytes)
 *
 * Allocates a new remote processor handle, but does not register
 * it yet. if @firmware is NULL, a default name is used.
 *
 * This function should be used by rproc implementations during initialization
 * of the remote processor.
 *
 * After creating an rproc handle using this function, and when ready,
 * implementations should then call rproc_add() to complete
 * the registration of the remote processor.
 *
 * On success the new rproc is returned, and on failure, NULL.
 *
 * Note: _never_ directly deallocate @rproc, even if it was not registered
 * yet. Instead, when you need to unroll rproc_alloc(), use rproc_put().
 */
struct rproc *rproc_alloc(struct device *dev, const char *name,
			  const struct rproc_ops *ops,
			  const char *firmware, int len)
{
	struct rproc *rproc;
	char *p, *template = "rproc-%s-fw";
	int name_len = 0;

	if (!dev || !name || !ops)
		return NULL;

	if (!firmware)
		/*
		 * Make room for default firmware name (minus %s plus '\0').
		 * If the caller didn't pass in a firmware name then
		 * construct a default name.  We're already glomming 'len'
		 * bytes onto the end of the struct rproc allocation, so do
		 * a few more for the default firmware name (but only if
		 * the caller doesn't pass one).
		 */
		name_len = strlen(name) + strlen(template) - 2 + 1;

	rproc = kzalloc(sizeof(*rproc) + len + name_len, GFP_KERNEL);
	if (!rproc)
		return NULL;

	if (!firmware) {
		p = (char *)rproc + sizeof(struct rproc) + len;
		snprintf(p, name_len, template, name);
	} else {
		p = (char *)firmware;
	}

	rproc->firmware = p;
	rproc->name = name;
	rproc->ops = ops;
	rproc->priv = &rproc[1];

	device_initialize(&rproc->dev);
	rproc->dev.parent = dev;
	rproc->dev.type = &rproc_type;

	/* Assign a unique device index and name */
	rproc->index = ida_simple_get(&rproc_dev_index, 0, 0, GFP_KERNEL);
	if (rproc->index < 0) {
		dev_err(dev, "ida_simple_get failed: %d\n", rproc->index);
		put_device(&rproc->dev);
		return NULL;
	}

	dev_set_name(&rproc->dev, "remoteproc%d", rproc->index);

	atomic_set(&rproc->power, 0);

	/* Set ELF as the default fw_ops handler */
	rproc->fw_ops = &rproc_elf_fw_ops;

	mutex_init(&rproc->lock);

	idr_init(&rproc->notifyids);

	INIT_LIST_HEAD(&rproc->carveouts);
	INIT_LIST_HEAD(&rproc->mappings);
	INIT_LIST_HEAD(&rproc->traces);
	INIT_LIST_HEAD(&rproc->last_traces);
	INIT_LIST_HEAD(&rproc->rvdevs);

	INIT_WORK(&rproc->crash_handler, rproc_crash_handler_work);
	init_completion(&rproc->crash_comp);

	rproc->state = RPROC_OFFLINE;

	return rproc;
}
EXPORT_SYMBOL(rproc_alloc);

/**
 * rproc_put() - unroll rproc_alloc()
 * @rproc: the remote processor handle
 *
 * This function decrements the rproc dev refcount.
 *
 * If no one holds any reference to rproc anymore, then its refcount would
 * now drop to zero, and it would be freed.
 */
void rproc_put(struct rproc *rproc)
{
	put_device(&rproc->dev);
}
EXPORT_SYMBOL(rproc_put);

/**
 * rproc_del() - unregister a remote processor
 * @rproc: rproc handle to unregister
 *
 * This function should be called when the platform specific rproc
 * implementation decides to remove the rproc device. it should
 * _only_ be called if a previous invocation of rproc_add()
 * has completed successfully.
 *
 * After rproc_del() returns, @rproc isn't freed yet, because
 * of the outstanding reference created by rproc_alloc. To decrement that
 * one last refcount, one still needs to call rproc_put().
 *
 * Returns 0 on success and -EINVAL if @rproc isn't valid.
 */
int rproc_del(struct rproc *rproc)
{
	struct rproc_vdev *rvdev, *tmp;

	if (!rproc)
		return -EINVAL;

	if (!rproc->use_userspace_loader) {
		/* if rproc is just being registered, wait */
		wait_for_completion(&rproc->firmware_loading_complete);
	}

	/* clean up remote vdev entries */
	list_for_each_entry_safe(rvdev, tmp, &rproc->rvdevs, node)
		rproc_remove_virtio_dev(rvdev);

	/* Free the copy of the resource table */
	kfree(rproc->cached_table);

	/* the rproc is downref'ed as soon as it's removed from the klist */
	mutex_lock(&rproc_list_mutex);
	list_del(&rproc->node);
	mutex_unlock(&rproc_list_mutex);

	device_del(&rproc->dev);

	return 0;
}
EXPORT_SYMBOL(rproc_del);

/**
 * rproc_report_crash() - rproc crash reporter function
 * @rproc: remote processor
 * @type: crash type
 *
 * This function must be called every time a crash is detected by the low-level
 * drivers implementing a specific remoteproc. This should not be called from a
 * non-remoteproc driver.
 *
 * This function can be called from atomic/interrupt context.
 */
void rproc_report_crash(struct rproc *rproc, enum rproc_crash_type type)
{
	if (!rproc) {
		pr_err("NULL rproc pointer\n");
		return;
	}

	dev_err(&rproc->dev, "crash detected in %s: type %s\n",
		rproc->name, rproc_crash_to_string(type));

	/* create a new task to handle the error */
	schedule_work(&rproc->crash_handler);
}
EXPORT_SYMBOL(rproc_report_crash);

static int __init remoteproc_init(void)
{
	rproc_init_debugfs();

	return 0;
}
module_init(remoteproc_init);

static void __exit remoteproc_exit(void)
{
	ida_destroy(&rproc_dev_index);

	rproc_exit_debugfs();
}
module_exit(remoteproc_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Generic Remote Processor Framework");