/*
 * A misc device for HDMI detection
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s " fmt, __func__	/* override print format */

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <soc/bcm2835/raspberrypi-firmware.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A virutal HDMI device for event detection");

static int debug = 4;
module_param(debug, int, S_IRUGO);
MODULE_PARM_DESC(debug, "debug level");

#define GPIO_HDP_PIN 46
static int prev_state = 0; // assuming the projector is connected // active low

static struct rpi_firmware * fw = NULL;
static struct timer_list tlist;
struct workqueue_struct *wq;
struct work_struct *hdmi_work;
void get_hdmi_status(struct work_struct *ws);

char *env_event[] = {"SUBSYSTEM=myHDMI", NULL};

static ssize_t status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, 4, "%d\n", prev_state);
}

static DEVICE_ATTR(status, S_IRUSR|S_IRGRP, status_show, NULL);

static struct attribute *hdmi_attrs[] = {
	&dev_attr_status.attr,
	NULL
};

static struct attribute_group hdmi_group_attrs = {
	.attrs = hdmi_attrs,
};

static const struct attribute_group *hdmi_all_attrs[] = {
	&hdmi_group_attrs,
	NULL
};

static struct miscdevice hdmi_dev = {
	.name = "hdmi",
	.minor = MISC_DYNAMIC_MINOR,
	.groups = hdmi_all_attrs,
};

void get_hdmi_status(struct work_struct *ws)
{	
	int status = gpio_get_value(GPIO_HDP_PIN);

	if(status == 0) // HDMI connected
    { 
		if(prev_state == 1) // HDMI just plugged in
            pr_info("HDMI attached\n");
			prev_state = 0;
	} 
    else // HDMI disconnected
    { 
		if(prev_state == 0)  // HDMI just unplugged
        {
			kobject_uevent_env(&(hdmi_dev.this_device->kobj), KOBJ_CHANGE, env_event);
			pr_info("HDMI removed\n");
			prev_state = 1;
		}
	}
}

void timer_callback (struct timer_list * tl) 
{
	INIT_WORK(hdmi_work, get_hdmi_status); // for scheduling work at runtime
	queue_work(wq, hdmi_work);
	
	tl->expires = (unsigned long) (jiffies + 1*HZ);
	add_timer (tl);
}

static void timer_init_func (void) 
{
	timer_setup (&tlist, timer_callback, 0);
	tlist.function = timer_callback;
	tlist.expires = (unsigned long) (jiffies + 1*HZ);
	add_timer (&tlist);
}

static void timer_exit_func (void) 
{
	del_timer_sync (&tlist);
}

static int __init hdmi_init(void)
{
	int ret;
	pr_info("loading hdmi_dev...debug=%d\n", debug);

	fw = rpi_firmware_get(NULL);
	wq = create_workqueue("run_hdmi_detector");
	hdmi_work = kmalloc(sizeof(struct work_struct), GFP_KERNEL);
	
	ret = misc_register(&hdmi_dev);
	if (ret)
		pr_err("error %d\n", ret);

    timer_init_func();

	return ret;
}

static void __exit hdmi_exit(void)
{
	pr_info("unloading ... \n");
	
    kfree(hdmi_work);
    timer_exit_func();
	flush_workqueue(wq);
	destroy_workqueue(wq);
	misc_deregister(&hdmi_dev);
}

module_init(hdmi_init);
module_exit(hdmi_exit);
