//////////////////////////////////////////////////////////////////////
//                             North Carolina State University
//
//
//
//                             Copyright 2016
//
////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or modify it
// under the terms and conditions of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
//
////////////////////////////////////////////////////////////////////////
//
//   Author:  Hung-Wei Tseng
//   
//   Project Members:
//   1. Jalandhar Singh (jsingh8)  
//   2. Rohit Nambisan (rnambis)
//
//   Description:
//     Skeleton of KeyValue Pseudo Device
//
////////////////////////////////////////////////////////////////////////

#include "keyvalue.h"
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/poll.h>
#include <linux/list.h>
#include <linux/spinlock.h>

// Link node to keep key value pair and pointer to next node. 
struct keyvalue_node{
	struct keyvalue_get *kv_get; 
	struct keyvalue_node *next;
};

// Spin lock 
rwlock_t keyvalue_lock;

// Head node of the list
static struct keyvalue_node *head=NULL;

// Transaction id to keep track of operations
unsigned transaction_id;

static __attribute__((unused)) void free_callback(void *data)
{
	return;
}

/* Function: to release or free the list */
static void release_list(void)
{
	struct keyvalue_node *temp = head;
	struct keyvalue_node *next = NULL;
	while( temp != NULL ){
		next=temp->next;
		kfree(temp);
		temp=next;
	}
}

/* Function: to get the key value pair */
static long keyvalue_get(struct keyvalue_get __user *ukv)
{   
    read_lock(&keyvalue_lock);
    
    bool found = false;
    struct keyvalue_node *temp = head;
    
    while(temp != NULL){
    	
    	if((temp->kv_get)->key == ukv->key){
	  
	  ukv->size=temp->kv_get->size;
	  memcpy(ukv->data, temp->kv_get->data, temp->kv_get->size);
    	  found = true;
    	  break;
    	
	}
	else{
	
	  temp=temp->next;
	
	}
    }

    if( !found ){
    	return -1;
    }
  
    transaction_id++; 
 
    read_unlock(&keyvalue_lock);
    
    return transaction_id;
}

/* Function: to set the key value pair */
static long keyvalue_set(struct keyvalue_set __user *ukv)
{
    write_lock(&keyvalue_lock);
    
    bool found = false;
    
    struct keyvalue_node *temp = head;
    
    while(temp != NULL){
    	
	if((temp->kv_get)->key == ukv->key){
	  
	  temp->kv_get->size=ukv->size;
	  memcpy(temp->kv_get->data, ukv->data, ukv->size);
    	  found = true;
    	  break;
    	
	}
	else{
	
    	  temp = temp->next;
	
	}
    }

    if( !found )
    {
	struct keyvalue_node *new_node;
    	new_node = kmalloc(sizeof(struct keyvalue_node),GFP_KERNEL);

    	new_node->kv_get = kmalloc(sizeof(struct keyvalue_get),GFP_KERNEL);

    	new_node->kv_get->key = ukv->key;
    	new_node->kv_get->size = ukv->size;
    	
	new_node->kv_get->data = kmalloc(ukv->size*sizeof(char),GFP_KERNEL);
	memcpy(new_node->kv_get->data, ukv->data, ukv->size);

	new_node->next = head;
	head = new_node;    
     }
  
     transaction_id++;
     
     write_unlock(&keyvalue_lock);
     
     return transaction_id;
}

/* Function: to delete the key value pair from the list */
static long keyvalue_delete(struct keyvalue_delete __user *ukv)
{
    write_lock(&keyvalue_lock);
    
    bool found = false;
    
    struct keyvalue_node *temp;
    
    struct keyvalue_node *prev = NULL;

    temp = head; 
   
    while(temp != NULL){
    
	if(temp->kv_get->key == ukv->key){
	
		if(temp == head){
			temp=temp->next;
			kfree(head);
			head=temp;
		}
		else{
		
			prev->next=temp->next;
			kfree(temp); 		
		}	  	  

    	  found = true;
    	  break;
    	}
	
	prev=temp;	
    
    }
    
    if( !found )
    	return -1;
    
    transaction_id++;
    
    write_unlock(&keyvalue_lock);	
    
    return transaction_id;
}

//Added by Hung-Wei
     
unsigned int keyvalue_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    return mask;
}

static long keyvalue_ioctl(struct file *filp, unsigned int cmd,
                                unsigned long arg)
{
    switch (cmd) {
    case KEYVALUE_IOCTL_GET:
        return keyvalue_get((void __user *) arg);
    case KEYVALUE_IOCTL_SET:
        return keyvalue_set((void __user *) arg);
    case KEYVALUE_IOCTL_DELETE:
        return keyvalue_delete((void __user *) arg);
    default:
        return -ENOTTY;
    }
}

static int keyvalue_mmap(struct file *filp, struct vm_area_struct *vma)
{
    return 0;
}

static const struct file_operations keyvalue_fops = {
    .owner                = THIS_MODULE,
    .unlocked_ioctl       = keyvalue_ioctl,
    .mmap                 = keyvalue_mmap,
};

static struct miscdevice keyvalue_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "keyvalue",
    .fops = &keyvalue_fops,
};

static int __init keyvalue_init(void)
{
    int ret;
    
    if ((ret = misc_register(&keyvalue_dev)))
        printk(KERN_ERR "Unable to register \"keyvalue\" misc device\n");
    
    return ret;
}

static void __exit keyvalue_exit(void)
{
    release_list();
    misc_deregister(&keyvalue_dev);
    
}

MODULE_AUTHOR("Jalandhar Singh <jsingh8@ncsu.edu>");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
module_init(keyvalue_init);
module_exit(keyvalue_exit);

