/*
 * A misc device for HDMI
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s " fmt, __func__	/* override print format */

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <soc/bcm2835/raspberrypi-firmware.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A virutal HDMI device");

static int debug = 4;
module_param(debug, int, S_IRUGO);
MODULE_PARM_DESC(debug, "debug level");

static int prev_state = 0; // assuming the projector is not connected

struct edid_block {
	u32 block_number;
	u32 status;
	unsigned char block[128];
};

struct rpi_firmware *fw = NULL;
struct workqueue_struct *wq;
struct work_struct *hdmi_work;


void get_hdmi_status(struct work_struct *ws)
{	
	struct edid_block edid_request = {
		.block_number = 0,
		.status = 0,
		.block = {0},
	};

	int i;

	rpi_firmware_property(fw, RPI_FIRMWARE_GET_EDID_BLOCK, &edid_request, sizeof(struct edid_block));
	int remaining_blocks = edid_request.block[126];
	pr_info("Block No:%d, status:%d, remaining_blocks:%d\n\n", edid_request.block_number, edid_request.status, remaining_blocks);
	for(i = 0; i < 128; i++)
		pr_info("%x", edid_request.block[i]);

	for(edid_request.block_number = 1; edid_request.block_number <= remaining_blocks; edid_request.block_number++)
	{
		rpi_firmware_property(fw, RPI_FIRMWARE_GET_EDID_BLOCK, &edid_request, sizeof(struct edid_block));
		pr_info("Block No:%d, status:%d\n", edid_request.block_number, edid_request.status);
		for(i = 0; i < 128; i++)
			pr_info("%x", edid_request.block[i]);
	}

	if(remaining_blocks >= 1)
	{
		if(prev_state == 0)
		{
			// if(kobject_uevent_env())
			pr_info("HDMI just plugged in!");
			prev_state = 1;
		}	
		pr_info("HDMI connected!");
	}

	else
	{
		if(prev_state == 1)
		{
			// if(kobject_uevent_env())
			pr_info("HDMI just removed!");
			prev_state = 0;
		}
		pr_info("HDMI disconnected!");
	}
}

static ssize_t status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	INIT_WORK(hdmi_work, get_hdmi_status);
	if(queue_work(wq, hdmi_work))
		pr_info("work already in queue; not added again!");

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

	return ret;
}

static void __exit hdmi_exit(void)
{
	pr_info("unloading ... \n");
	
	flush_workqueue(wq);
	destroy_workqueue(wq);
	
	misc_deregister(&hdmi_dev);
}

module_init(hdmi_init);
module_exit(hdmi_exit);
