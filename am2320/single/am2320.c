#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/init.h>
#include<linux/delay.h>
#include<linux/gpio.h>
#include<linux/types.h>
#include<asm/irq.h>
#include<mach/regs-gpio.h>
#include<mach/hardware.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/cdev.h>
#include<linux/errno.h>
#include<asm/uaccess.h>
 
#define DQ S3C2410_GPG(0)
#define INPUT S3C2410_GPIO_INPUT
#define OUTPUT S3C2410_GPIO_OUTPUT
 
#define D_MAJOR 0
#define D_MINOR 0
#define DEV_NAME "am2320" 
 
 
static int am2320_major = D_MAJOR;
static int am2320_minor = D_MINOR;
unsigned short temperature,humidity;
struct ds18b20_device
{
    struct class    *sy_class;
    struct cdev     cdev;
};
static struct ds18b20_device   dev;//若使用指针，记得给指针开辟空间。
 
static unsigned int am2320_reset(void)
{
  	  int err = 0;
 	  int retry = 0;
	  s3c2410_gpio_cfgpin(DQ,OUTPUT);
   	  s3c2410_gpio_setpin(DQ, 1);  
 	  s3c2410_gpio_setpin(DQ, 0);
 		mdelay(2);
      s3c2410_gpio_setpin(DQ, 1); //释放总线
	  s3c2410_gpio_cfgpin(DQ, INPUT);

	while(s3c2410_gpio_getpin(DQ) & (retry++<200))udelay(1); //总线释放时间20-200
    	 if(retry>=200)
	 	goto error;
	printk("retry1:%d\n",retry);
	retry = 0;
	while(!s3c2410_gpio_getpin(DQ) & (retry++<90))udelay(1); //响应低电平75-85
	   if(retry>=90) 
	printk("retry2:%d\n",retry);
    //响应高电平响应，通知准备接收数据
    retry = 0;
	while(s3c2410_gpio_getpin(DQ) & (retry++<200))udelay(1); //响应高电平75-85
	   if(retry>=200)
		 goto error;
	printk("retry3:%d\n",retry);
	return 0;

error:
	printk("open ds18b20 failed!\n");
	return -1;
}

 static int set_pin_get(void)
{   
    if(s3c2410_gpio_getpin(DQ))
		return 1;
	else
		return 0;
}

 static int set_pin_delay_get(unsigned int time)
{   
    
	s3c2410_gpio_cfgpin(DQ,INPUT);
    udelay(time);
    if(s3c2410_gpio_getpin(DQ))
		return 1;
	else
		return 0;
}

static unsigned int ds18b20_write(unsigned char data)
{
    unsigned int i,testb;
    s3c2410_gpio_cfgpin(DQ, OUTPUT);
	/*
    for (i = 0; i < 8; i++)   //只能一位一位的读写
    {
        s3c2410_gpio_setpin(DQ, 0);
        udelay(5);
        if(data & 0x01)
        {
            s3c2410_gpio_setpin(DQ ,1);
                udelay(60);
        }
        else udelay(60);
        data >>= 1;     //从最低位开始判断；每比较完一次便把数据向右移，获得新的最低位状态
        s3c2410_gpio_setpin(DQ, 1);
        udelay(1);
    }
    */
    for (i = 0; i < 8; i++)   //只能一位一位的读写
    {
        testb = data & 0x1;
        data >>= 1;
	  if(testb){  // 写1时序
            s3c2410_gpio_setpin(DQ, 0);			
            udelay(2);
   
            s3c2410_gpio_setpin(DQ, 1);
            udelay(60);
        } else {    // 写0时序
             s3c2410_gpio_setpin(DQ, 0);
            udelay(60);
 
            s3c2410_gpio_setpin(DQ, 1);
            udelay(2);
        }
    }
    return 2;
}
 
static unsigned int ds18b20_read(void)
{
    unsigned int i ;
    unsigned char data = 0x00;
 
    for (i =0; i < 8 ; i++)
    {
        s3c2410_gpio_cfgpin(DQ, OUTPUT);
        s3c2410_gpio_setpin(DQ, 0);
        udelay(2);
        s3c2410_gpio_setpin(DQ, 1);
        udelay(12);
        s3c2410_gpio_cfgpin(DQ, INPUT);
        data >>= 1;
        if(0 != s3c2410_gpio_getpin(DQ))
            data |= 0x80;   //最低位数据从data的最高位放起，边放边右移直到读取位完毕。
        udelay(50);
    }
    return data;
}
 static int read_am2320_data(unsigned char *data)
 {
	unsigned char tmp=0,i,j,wait,status=0;
 	int flag = am2320_reset();
	if (flag)  
	{  
		printk("open(read) am2320 failed\n");  
		return -1;  
	}  
     for(i=0;i<5;i++)  
     {
     		for(j=0;j<8;j++)//one byte
     		{
     			tmp<<=1;
			wait = 0;
			while(!s3c2410_gpio_getpin(DQ) &&wait++<100) udelay(1);//信号间隔低电平时间48~55us。低电平0和1是一样的。
			if(wait >= 100)
				{ 	
					printk(KERN_WARNING "read am2320 failed\n");  
					return 0; 
				}
			
			wait = 0;
			while(s3c2410_gpio_getpin(DQ) &&wait++<100) udelay(1);//信号间隔高电平时间48~55us
			printk("read wait: %d \n",wait);
			
			if(wait>=2&&wait<=30)//信号‘0’时间: 22~30us
			{
				tmp |=0x00;
			}
			if(wait>=55&&wait<=75)//信号‘1’时间: 68~75us 
			{
				tmp |=0x01;
			}
			if(wait>=100)
			{
				printk(KERN_WARNING "read am2320 failed\n");  
				return 0; 
			}
		}
		*data=tmp;
		data++;
		tmp=0;
     }
error:
	return -1;
 }
static ssize_t read_am2320(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	unsigned char Data[5] = {0x00};
	unsigned long err; 
	
      read_am2320_data(Data);  
	err = copy_to_user(buf, Data, sizeof(Data));
	
    	return err? -EFAULT:count;
  
}

static int open_am2320(struct inode *inode, struct file *filp)
{

    int flag = 0;
  
    flag = am2320_reset();
    if (flag)  
    {  
        printk(KERN_WARNING "open am2320 failed\n");  
        return -1;  
    }  
    printk(KERN_NOTICE "open am2320 successful\n");  
    return 0;  
		

}

static int release_am2320(struct inode *inode, struct file *filp)
{
    return 0;
}
 
static struct file_operations fops={
    .owner  = THIS_MODULE,
    .read   = read_am2320,
    .open   = open_am2320,
    .release = release_am2320,
};
 
static int __init am2320_init(void)
{
    int result,err;
    dev_t devno = 0;
 
    if(am2320_major)
    {
        devno = MKDEV(am2320_major, am2320_minor);
        result = register_chrdev_region(devno, 1, DEV_NAME);
    }
    else{//申请设备号
        result = alloc_chrdev_region(&devno, am2320_minor, 1, DEV_NAME);
        am2320_major = MAJOR(devno);
    }
    if(result < 0)
    {
        printk(KERN_ERR "%s can't use major %d\n",DEV_NAME, am2320_major);
    }
    printk("%s use major %d\n",DEV_NAME, am2320_major);
 	//注册设备
    cdev_init(&dev.cdev,&fops);
    dev.cdev.owner = THIS_MODULE;
    err = cdev_add(&dev.cdev, devno, 1);
    if(err)
    {
        printk(KERN_NOTICE"ERROR %d add Am2320\n",err);
        goto ERROR;
    }
	//创建设备文件
    dev.sy_class   = class_create(THIS_MODULE, DEV_NAME);
    device_create(dev.sy_class, NULL, MKDEV(am2320_major, am2320_minor), NULL, DEV_NAME);
    printk(KERN_NOTICE"Am2320 is ok!\n");
    return 0;
ERROR:
    printk(KERN_ERR"%s driver installed failure.\n",DEV_NAME);
    cdev_del(&dev.cdev);
    unregister_chrdev_region(devno, 1);
    return err;
 
}
static void __exit am2320_exit(void)
{
    dev_t devno = MKDEV(am2320_major, 0);
 
    cdev_del(&dev.cdev);
    device_destroy(dev.sy_class,devno);
    class_destroy(dev.sy_class);
 
    unregister_chrdev_region(devno, 1);
    printk(KERN_NOTICE"bye am2320!\n");
}
 
module_init(am2320_init);
module_exit(am2320_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("handy_sky@outlook.com");


