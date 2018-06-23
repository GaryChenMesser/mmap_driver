#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/mm.h>  
 
#ifndef VM_RESERVED
# define  VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif
 
struct dentry  *file;
 
struct mmap_info
{
    char *data;            
    int reference;      
};
 
void mmap_open(struct vm_area_struct *vma)
{
	printk(KERN_NOTICE "Simple VMA operation, virt %lx, phys %lx\n", vma->vm_start, vma->vm_pgoff << PAGE_SHIFT);
    struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
    info->reference++;
}
 
void mmap_close(struct vm_area_struct *vma)
{
    struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
    info->reference--;
}
 
struct vm_operations_struct mmap_vm_ops =
{
    .open =     mmap_open,
    .close =    mmap_close,
};


static int op_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	
	if (offset >= __pa(high_memory) || (filp->f_flags & O_SYNC))
		vma->vm_flags |= VM_IO;
	vma->vm_flags |= VM_RESERVED;
	
	vma->vm_ops = &mmap_vm_ops;
	mmap_open(vma);
	return 0;
/*
    vma->vm_ops = &mmap_vm_ops;
    vma->vm_flags |= VM_RESERVED;    
    vma->vm_private_data = filp->private_data;
    mmap_open(vma);
    return 0;
  */
}
 
int mmapfop_close(struct inode *inode, struct file *filp)
{
    struct mmap_info *info = filp->private_data;
     
    free_page((unsigned long)info->data);
    kfree(info);
    filp->private_data = NULL;
    return 0;
}
 
int mmapfop_open(struct inode *inode, struct file *filp)
{
    struct mmap_info *info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);    
    info->data = (char *)get_zeroed_page(GFP_KERNEL);
    //memcpy(info->data, "hello from kernel this is file: ", 32);
    //memcpy(info->data + 32, filp->f_dentry->d_name.name, strlen(filp->f_dentry->d_name.name));
    /* assign this info struct to the file */
    filp->private_data = info;
    return 0;
}
 
static const struct file_operations mmap_fops = {
    .open = mmapfop_open,
    .release = mmapfop_close,
    .mmap = op_mmap,
};
 
static int __init mmapexample_module_init(void)
{
    file = debugfs_create_file("mmap_example", 0644, NULL, NULL, &mmap_fops);
    return 0;
}
 
static void __exit mmapexample_module_exit(void)
{
    debugfs_remove(file);
}
 
module_init(mmapexample_module_init);
module_exit(mmapexample_module_exit);
MODULE_LICENSE("GPL");
