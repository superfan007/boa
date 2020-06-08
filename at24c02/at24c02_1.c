#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>

#define DEVICE_NAME "at24c02"
#define DRIVER_NAME "at24c02_drv"

static i2c_client *at24c02_client;

int major;

struct class *at24c02dev_class;
struct class_device *at24c02dev_class_device;

static unsigned short ignore[] = {I2C_CLIENT_END};
static unsigned short normal_addr[] = {0x50, I2C_CLIENT_END};

static ssize_t at24c02_drv_read(struct file *file, char __user *userbuf, size_t count, loff_t *off)
{
　　struct i2c_msg msg[2];
　　u8 addr, data;
　　int ret;

　　cpoy_from_user(&addr, userbuf, 1);

　　msg[0].addr = at24c02_client->addr;
　　msg[0].flags = 0;                                   //写标志
　　msg[0].len = 1;
　　msg[0].buf = &addr                                  //写入要读的地址

　　msg[1].addr = at24c02_client->addr;
　　msg[1].flags = I2C_M_RD;                            //读标志
　　msg[1].len = 1;
　　msg[1].buf = &data                                  //读出数据

　　ret = i2c_transfer(at24c02_client->adapter, msg, 2);

　　if(ret == 2)                                        //表示2个msg传输成功
　　{
　　　　copy_to_user(buf, &data, 1);                     //上传数据
　　　　return 0;
　　}
　　else
　　return -EAGAIN;
}

static ssize_t at24c02_drv_write(struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{
　　struct i2c_msg msg[1];
　　u8 addr_and_data[2];
　　int ret;

　　copy_from_user(addr_and_data, buf, 2);              //获取 地址 数据

　　msg[0].addr = at24c02_client->addr;
　　msg[0].flags = 0;                                   //写标志
　　msg[0].len = 2;
　　msg[0].buf = addr_and_data;                         //写入要写的地址和数据

　　ret = i2c_transfer(at24c02_client->adapter, msg, 1);

　　if(ret == 1)                                        //表示1个msg传输成功
　　　　return 0;
　　else
　　　　return -EAGAIN;
}
struct file_operations at24c02_drv_fops = {
　　.owner = THIS_MODULE,
　　.read = at24c02_drv_read,
　　.write = at24c02_drv_write,
};

struct i2c_client_address_data at24c02_addr = {
　　.normal_i2c = normal_addr;                          //存放正常的设备高7位地址数据
　　.probe = ignore;                                    //存放不受*ignore影响的高7位设备地址数据
　　.ignore = ignore;                                   //存放*ignore的高7位设备地址数据
};

static i2c_driver at24c02_driver = {
　　.driver = {
　　　　.name = "at24c02",
　　},
　　.id = I2C_DRIVERID_EEPROM,
　　.attach_adapter = at24c02_attach_adapter,
　　.detach_client = at24c02_detach_client,
};

static int at24c02_detach_client(struct i2c_client *client)
{
　　i2c_detach_client(at24c02_client);
　　kfree(at24c02_client);
　　class_device_destory(at24c02dev_class, MKDEV(major, 0));
　　class_destory(at24c02dev_class);
　　return 0;
}

//注册真正地IIC设备
static int at24c02_detect(struct i2c_adapter *adap, int addr, int kind)
{
　　//分配并初始化i2c_client结构体
　　at24c02_client = kzalloc(sizeof(i2c_adapter *adap), GFP_KERNEL);
　　at24c02_client->addr = addr;
　　at24c02_client->adapter = adap;
　　at24c02_client->driver = &at24c02_driver
　　at24c02_client->flags = 0;
　　strlcpy(at24c02_client->name, DEVICE_NAME, I2C_NAME_SIZE);       //设置IIC设备的名字

　　//将at24c02_client与适配器进行连接
　　i2c_attach_client(at24c02_client);

　　//注册字符类设备
　　major = regeister_chrdev(0, DEVICE_NAME, &at24c02_drv_fops);     //注册字符设备
　　at24c02dev_class = class_create(THID_MODULE, DEVICE_NAME);       //注册设备类
　　at24c02dev_class_device = class_device_create(at24c02dev_class, NULL, MKEDV(major, 0), NULL, DEVICE_NAME);
}

//发送IIC设备的地址at24c02_addr, 如果有响应则调用回调函数at24c02_detect
static int at24c02_attach_adapter(struct i2c_adapter *adapter)
{
　　return i2c_probe(adapter, &at24c02_addr, at24c02_detect);
}

static int at24c02_drv_init()
{
　　i2c_add_driver(&at24c02_driver);
　　return 0;
}

static void at24c02_drv_exit()
{
　　i2c_del_driver(&at24c02_driver);
} 

module_init(at24c02_drv_init);
module_exit(at24c02_drv_exit);

MODULE_LICENSE("GPL");
/*

Makefile文件：

　　obj-m += at24c02_drv.o

　　KERN_DIR = /work/system/linux-2.6.22.6

　　all:
　　make -C $(KERN_DIR) M=`pwd` modules 
　　clean:
　　rm -rf *.o *.ko *.order *.symvers *.mod.c

at24c02_app.c文件：

#include "sys/types.h"
#include "sys/stst.h"
#include "stdio.h"
#include "fcntl.h"

void print_usage(char *file)
{
　　printf("%s r addr \n", file);
　　printf("%s w addr val \n", file);
}

int main(int argc, char **argv)
{
　　int fd;
　　char *filename = argv[1];


　　fd = open(filename, O_RDWR); 　　　　　　　　　　　　　　　　//以阻塞的方式打开设备文件
　　if(fd < 0)
　　{
　　　　printf("error, can not open %s \n", filename);
　　　　return -1;
　　}
　　if(strcmp(argv[2], "r") == 0)
　　{
　　　　buf[0] = strtoul(argv[3], NULL, 0);
　　　　read(fd, buf, 1);
　　　　printf("data: %c, %d, 0x2x \n", buf[0], buf[0], buf[0]);
　　}
　　else if(strcmp(argv[2], "w") == 0)
　　{
　　　　buf[0] = strtoul(argv[3], NULL, 0);
　　　　buf[1] = strtoul(argv[4], NULL, 0);
　　　　write(fd, buf, 2);
　　}
　　else
　　{
　　　　print_usage(argv[0]);
　　return -1;
　　}
}
*/
