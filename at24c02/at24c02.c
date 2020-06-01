    #include <linux/kernel.h>  
    #include <linux/module.h>  
    #include <linux/fs.h>  
    #include <linux/slab.h>  
    #include <linux/init.h>  
    #include <linux/list.h>  
    #include <linux/i2c.h>  
    #include <linux/i2c-dev.h>  
    #include <linux/smp_lock.h>  
    #include <linux/jiffies.h>  
    #include <asm/uaccess.h>  
    #include <linux/delay.h>  
      
      
    #define DEBUG 1  
    #ifdef DEBUG  
    #define dbg(x...) printk(x)  
    #else   
    #define dbg(x...) (void)(0)  
    #endif  
      
    #define I2C_MAJOR 89  
    #define DEVICE_NAME "at24c02"  
    static struct class *my_dev_class;  
    static struct i2c_client *my_client;  
    static struct i2c_driver my_i2c_driver;  
      
      
    static struct i2c_device_id my_ids[] = {  
        {"24c01",0x50},  
        {"24c02",0x50},  
        {"24c08",0x50},  
        {}  
    };  
      
    MODULE_DEVICE_TABLE(i2c,my_ids);  
      
    static int my_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)  
    {  
        int res;  
        struct device *dev;  
      
        dbg("probe:name = %s,flag =%d,addr = %d,adapter = %d,driver = %s\n",client->name,  
             client->flags,client->addr,client->adapter->nr,client->driver->driver.name );  
      
        dev = device_create(my_dev_class, &client->dev,  
                         MKDEV(I2C_MAJOR, 0), NULL,  
                         DEVICE_NAME);  
        if (IS_ERR(dev))  
        {  
            dbg("device create error\n");  
            goto out;  
        }  
        my_client = client;  
          
        return 0;  
    out:  
        return -1;  
    }  
    static int  my_i2c_remove(struct i2c_client *client)  
    {  
      
        dbg("remove\n");  
        return 0;  
    }  
      
    static ssize_t at24c02_read(struct file *fd, char *buf, ssize_t count, loff_t *offset)  
    {  
        char *tmp;  
        int ret;  
        char data_byte;  
        char reg_addr = 0,i;  
        struct i2c_client *client = (struct i2c_client*) fd->private_data;  
        struct i2c_msg msgs[2];  
      
        dbg("read:count = %d,offset = %ld\n",count,*offset);  
        tmp = kmalloc(count,GFP_KERNEL);  
      
        if (!tmp)  
        {  
            dbg("malloc error in read function\n");  
            goto out;  
        }  
      
        reg_addr = *offset;  
        msgs[0].addr = client->addr;  
        msgs[0].flags = client->flags & (I2C_M_TEN|I2C_CLIENT_PEC) ;  
        msgs[0].len = 1;  
        msgs[0].buf = (char *)®_addr;  
          
        msgs[1].addr= client->addr;  
        msgs[1].flags = client->flags & (I2C_M_TEN|I2C_CLIENT_PEC);  
        msgs[1].flags |= I2C_M_RD;  
        msgs[1].len = count;  
        msgs[1].buf = (char*)tmp;  
      
        ret = i2c_transfer(client->adapter,&msgs,2);  
        if (ret != 2)  
            goto out;  
        if (copy_to_user(buf, tmp, count))  
            goto out;  
          
        kfree(tmp);  
        return count;  
    out:  
        kfree(tmp);  
        return -1;    
          
    }  
      
      
    static int at24c02_ioctl(struct file *fd, unsigned int cmd, unsigned long arg)  
    {  
        dbg("ioctl code ...\n");  
        return 0;  
    }  
      
    static ssize_t at24c02_write(struct file *fd, char *buf, ssize_t count, loff_t *offset)  
    {  
        int ret,i;  
        char *tmp;  
        int errflg;  
        struct i2c_msg msg;  
        struct i2c_client *client = (struct i2c_client*) fd->private_data;  
        char tmp_data[2];  
      
        dbg("write:count = %d,offset = %ld\n",count,*offset);  
        tmp = kmalloc(count, GFP_KERNEL);  
        if (!tmp)  
            goto out;  
        if (copy_from_user(tmp, buf, count))  
            goto out;  
        msg.addr = client->addr;  
        msg.flags = client->flags & (I2C_M_TEN | I2C_CLIENT_PEC);  
        for (i = 0; i < count; i++) {  
            msg.len = 2;  
            tmp_data[0] = *offset + i;  
            tmp_data[1] = tmp[i];  
            msg.buf = tmp_data;  
            ret = i2c_transfer(client->adapter,&msg,1);  
            if (ret != 1)  
                goto out;  
            msleep(1);  
        }   
        kfree(tmp);  
      
        return ((ret == 1) ? count:ret);  
    out:  
        kfree(tmp);  
        return -1;  
          
    }  
    static int at24c02_open(struct inode *inode, struct file *fd)  
    {  
      
        fd->private_data =(void*)my_client;  
        return 0;  
      
    }  
      
    static int at24c02_release(struct inode *inode, struct file *fd)  
    {  
        dbg("release\n");  
        fd->private_data = NULL;  
          
        return 0;     
      
    }  
      
    static const struct file_operations i2c_fops = {  
        .owner = THIS_MODULE,  
        .open   = at24c02_open,  
        .read  = at24c02_read,  
        .write = at24c02_write,  
        .unlocked_ioctl = at24c02_ioctl,  
        .release = at24c02_release,  
    };  
      
    static struct i2c_driver my_i2c_driver = {  
        .driver = {  
            .name = "i2c_demo",  
            .owner = THIS_MODULE,  
        },  
        .probe = my_i2c_probe,  
        .remove = my_i2c_remove,  
        .id_table = my_ids,  
    };  
      
    static int __init my_i2c_init(void)  
    {  
        int res;  
          
          
        res = register_chrdev(I2C_MAJOR,DEVICE_NAME,&i2c_fops);  
        if (res)  
        {  
            dbg("register_chrdev error\n");  
            return -1;  
        }  
        my_dev_class = class_create(THIS_MODULE, DEVICE_NAME);  
        if (IS_ERR(my_dev_class))  
        {  
            dbg("create class error\n");  
            unregister_chrdev(I2C_MAJOR, DEVICE_NAME);  
            return -1;  
        }  
        return i2c_add_driver(&my_i2c_driver);  
    }  
      
    static void __exit my_i2c_exit(void)  
    {  
        unregister_chrdev(I2C_MAJOR, DEVICE_NAME);  
        class_destroy(my_dev_class);  
        i2c_del_driver(&my_i2c_driver);  
          
    }  
      
    MODULE_AUTHOR("itspy<itspy.wei@gmail.com>");  
    MODULE_DESCRIPTION("i2c client driver demo");  
    MODULE_LICENSE("GPL");  
    module_init(my_i2c_init);  
    module_exit(my_i2c_exit);  
