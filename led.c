#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <asm/io.h>

#define LED_MAJOR       200
#define LED_NAME        "led"

/*寄存器物理地址*/
#define CCM_CCGR1_BASE          (0x020c406c)
#define SW_MUX_GPIO1_IO03_BASE  (0x020e0068)
#define SW_PAD_GPIO_IO03_BASE   (0x020e02f4)
#define GPIO1_DR_BASE           (0x0209c000)
#define GPIO1_GDIR_BASE         (0x0209c004)

#define LED_OFF         0
#define LED_ON          1

/*映射后的寄存器虚拟地址指针*/
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

static void led_switch(unsigned char sta)
{
    int val = 0;
    switch (sta)
    {
    case LED_ON:
        /*打开LED*/
        val = readl(GPIO1_DR);
        val &= ~(1<<3);
        writel(val,GPIO1_DR);
        break;
    case LED_OFF:
        /*关闭LED*/
        val = readl(GPIO1_DR);
        val |= (1<<3);
        writel(val,GPIO1_DR);
        break;
    default:
        break;
    }
}
static ssize_t led_write(struct file *filp, const char __user *buf, 
                            size_t count, loff_t *ppos)
{
    int ret = 0;
    unsigned char databuf[1];
    ret = copy_from_user(databuf,buf,count);
    if(ret<0){
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }
    led_switch(databuf[0]);
    return 0;
}
static int led_open(struct inode *inode, struct file *filp)
{
    return 0;
}
static int led_release(struct inode *inode, struct file *filp)
{
    return 0;
}
static const struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .write = led_write,
    .open = led_open,
    .release = led_release,
};
/*入口*/
static int __init led_init(void)
{
    int ret = 0;
    int val = 0;
    /*初始化LED*/
    /*1、寄存器地址映射*/
    IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE,4);
    SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE,4);
    SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO_IO03_BASE,4);
    GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE,4);
    GPIO1_DR = ioremap(GPIO1_DR_BASE,4);

    /*2、使能GPIO1时钟*/
    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(3<<26);    /*清除bit[27-26]*/
    val |= (3<<26);     /*bit[27-26]置1*/
    writel(val,IMX6U_CCM_CCGR1);

    /*3、设置GPIO1_IO03的复用功能及电气属性*/
    writel(0x05,SW_MUX_GPIO1_IO03);

    writel(0x10b0,SW_PAD_GPIO1_IO03);

    /*设置GPIO1_IO03为输出模式*/
    val = readl(GPIO1_GDIR);
    val &= ~(1<<3);
    val |= (1<<3);
    writel(val,GPIO1_GDIR);

    /*默认关闭LED*/
    val = readl(GPIO1_DR);
    val |= (1<<3);
    writel(val,GPIO1_DR);

    /*注册字符设备*/
    ret = register_chrdev(LED_MAJOR,LED_NAME,&led_fops);
    if(ret<0){
        printk("register chrdev failed!!!\r\n");
        return -EIO;
    }
    printk("led_init\r\n");
    return 0;
}
/*出口*/
static void __exit led_exit(void)
{
    /*取消映射*/
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_GDIR);
    iounmap(GPIO1_DR);

    /*注销字符设备*/
    unregister_chrdev(LED_MAJOR,LED_NAME);
    printk("led_exit\r\n");
}
/*注册驱动加载和卸载*/
module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tanminghang");

