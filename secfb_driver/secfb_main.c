#define DEBUG

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/dma-buf.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include "secfb_ioctl.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("J. Forissier");
MODULE_DESCRIPTION("A dummy secure framebuffer driver for secvideo_demo");

#define DEVNAME "secfb"

static int is_open;		/* Exclusive access */
static struct secfb_buffer {
	u32 paddr;
	size_t size;
	int fd;
} buffer = {
	.paddr = 0xff000000,
	.size = 0x00200000,
};
static struct dma_buf *secfb_dmabuf;	/* This exports the secure frame buffer */
static struct miscdevice secfb_dev;

/*
 * dma-buf operations
 */

static struct sg_table *secfb_dmabuf_map_dma_buf(struct dma_buf_attachment
							*attach,
						 enum dma_data_direction dir)
{
	struct secfb_buffer *buff = attach->dmabuf->priv;
	struct sg_table *sgt;

	sgt = kmalloc(sizeof(*sgt), GFP_KERNEL);
	if (!sgt)
		return NULL;
	if (sg_alloc_table(sgt, 1, GFP_KERNEL))
		goto free_sgt;

	sg_set_page(sgt->sgl, phys_to_page(buff->paddr), buff->size, 0);

	return sgt;

free_sgt:
	kfree(sgt);
	return NULL;
}

static void secfb_dmabuf_unmap_dma_buf(struct dma_buf_attachment *attach,
				       struct sg_table *table,
				       enum dma_data_direction dir)
{
	sg_free_table(table);
	kfree(table);
}

static void secfb_dmabuf_release(struct dma_buf *dmabuf)
{
}

static void *secfb_dmabuf_kmap_atomic(struct dma_buf *dmabuf,
				      unsigned long pgnum)
{
	/* Buffer cannot be mapped to kernel space */
	return NULL;
}

static void *secfb_dmabuf_kmap(struct dma_buf *dmabuf, unsigned long pgnum)
{
	/* Buffer cannot be mapped to kernel space */
	return NULL;
}

static int secfb_dmabuf_mmap(struct dma_buf *dmabuf,
			     struct vm_area_struct *vma)
{
	struct secfb_buffer *buff = dmabuf->priv;
	size_t size = vma->vm_end - vma->vm_start;
	int ret;

	pgprot_t prot;

	if (WARN_ON(!buff))
		return -EINVAL;

	prot = vma->vm_page_prot;

	ret =
	    remap_pfn_range(vma, vma->vm_start, buff->paddr >> PAGE_SHIFT, size,
			    prot);
	if (!ret)
		vma->vm_private_data = (void *)buff;

	dev_dbg(secfb_dev.this_device, "mmap (p@=0x%08x,s=%dKiB) => %x [ret=%d]\n",
		buff->paddr, (int)size / 1024,
		(unsigned int)vma->vm_start, ret);

	return ret;
}

static struct dma_buf_ops dma_buf_ops = {
	.map_dma_buf = secfb_dmabuf_map_dma_buf,
	.unmap_dma_buf = secfb_dmabuf_unmap_dma_buf,
	.release = secfb_dmabuf_release,
	.kmap_atomic = secfb_dmabuf_kmap_atomic,
	.kmap = secfb_dmabuf_kmap,
	.mmap = secfb_dmabuf_mmap,
};

/*
 * File operations
 */

static int secfb_open(struct inode *inode, struct file *file)
{
	if (is_open)
		return -EBUSY;
	is_open++;
	try_module_get(THIS_MODULE);
	return 0;
}

static int secfb_release(struct inode *inode, struct file *file)
{
	is_open--;
	module_put(THIS_MODULE);
	return 0;
}

static int do_get_secfb_fd(struct secfb_io __user *u_secfb)
{
	int ret;
	struct secfb_io k_secfb;

	if (!secfb_dmabuf) {
		secfb_dmabuf = dma_buf_export(&buffer, &dma_buf_ops,
					      buffer.size, O_RDWR, NULL);
		dev_dbg(secfb_dev.this_device, "secfb_dmabuf = %p",
			secfb_dmabuf);
		if (!secfb_dmabuf)
			return -ENOMEM;
		buffer.fd = dma_buf_fd(secfb_dmabuf, 0);
	}

	memset(&k_secfb, 0, sizeof(k_secfb));
	k_secfb.fd = buffer.fd;
	k_secfb.size = buffer.size;
	ret = copy_to_user(u_secfb, &k_secfb, sizeof(*u_secfb));

	return ret;
}

static long secfb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;
	struct device *dev = secfb_dev.this_device;

	switch (cmd) {
	case SECFB_IOCTL_GET_SECFB_FD:
		dev_dbg(dev, "SECFB_IOCTL_GET_SECFB_FD");

		ret = do_get_secfb_fd((struct secfb_io __user *)arg);
		break;
	default:
		ret = -ENOSYS;
	}

	return ret;
}

static struct file_operations secfb_fops = {
	.owner = THIS_MODULE,
	.open = secfb_open,
	.release = secfb_release,
	.unlocked_ioctl = secfb_ioctl,
};

/*
 * Init/exit
 */

static int __init secfb_init(void)
{
	int ret;

	secfb_dev.minor = MISC_DYNAMIC_MINOR;
	secfb_dev.name = DEVNAME;
	secfb_dev.fops = &secfb_fops;

	ret = misc_register(&secfb_dev);
	if (ret)
		return ret;

	printk(KERN_INFO "secfb: initialized (minor=%d)\n", secfb_dev.minor);
	return 0;
}

static void __exit secfb_exit(void)
{
    misc_deregister(&secfb_dev);
}

module_init(secfb_init);
module_exit(secfb_exit);
