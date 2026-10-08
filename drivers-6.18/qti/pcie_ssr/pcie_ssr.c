/* Copyright (c) 2020, The Linux Foundation. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include <linux/debugfs.h>
#include <linux/ipc_logging.h>
#include <linux/types.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/pcie_ssr.h>

#define MAX_IPC_LOG_NAME_LEN 20
#define PCIE_SSR_LOG_PAGES 20

#define SSR_DBG(__msg, ...) \
do { \
	if (ipc_log) \
		ipc_log_string(ipc_log, \
			"[%s]: "__msg, __func__, ##__VA_ARGS__); \
} while (0)

void	*ipc_log;
void	*ctrl;

enum pcie_ssr_power_option {
	PCIE_POWER_OFF,
	PCIE_POWER_ON,
	PCIE_SSR_MAX_POWER_OPTION
};

static const char * const
	pcie_ssr_power_option_desc[PCIE_SSR_MAX_POWER_OPTION] = {
		"POWER_OFF",
		"POWER_ON",
};

static void pcie_ssr_sel_power_case(void *ctrl, u32 powercase) {
	switch (powercase) {
	case PCIE_POWER_OFF:
		SSR_DBG("\n\n SSR_debug:Processing turn off\n");
		mhi_arch_pcie_ssr_ops_power_off(ctrl);
		break;
	case PCIE_POWER_ON:
		SSR_DBG("\n\n SSR_debug:Processing turn on\n");
		mhi_arch_pcie_ssr_ops_power_on(ctrl);
		break;
	default:
		SSR_DBG("SSR_debug:Invalid powercase: %d.\n", powercase);
		break;
	}
}

static struct dentry *dent_pcie_ssr;
static struct dentry *dfile_power;

static int pcie_ssr_debugfs_parse_input(const char __user *buf,
					size_t count, unsigned int *data) {
	unsigned long ret;
	char *str, *str_temp;

	str = kmalloc(count + 1, GFP_KERNEL);
	if (!str)
		return -ENOMEM;

	ret = copy_from_user(str, buf, count);
	if (ret) {
		kfree(str);
		SSR_DBG(" SSR_debug: Unable to copy string from user");
		return -EFAULT;
	}

	str[count] = 0;
	str_temp = str;

	ret = get_option(&str_temp, data);
	kfree(str);
	if (ret != 1) {
		SSR_DBG(" SSR_debug: No valid option found");
		return -EINVAL;
	}
	return 0;
}

static int pcie_ssr_power_show(struct seq_file *m, void *v) {
	int i;

	for (i = 0; i < PCIE_SSR_MAX_POWER_OPTION; i++)
		seq_printf(m, "\t%d:\t %s\n", i,
					pcie_ssr_power_option_desc[i]);

	return 0;
}

static int pcie_ssr_power_open(struct inode *inode, struct file *file) {
	return single_open(file, pcie_ssr_power_show, NULL);
}

static ssize_t pcie_ssr_power_select(struct file *file,
			const char __user *buf,	size_t count, loff_t *ppos) {
	int i, ret;
	unsigned int powercase = 0;

	ret = pcie_ssr_debugfs_parse_input(buf, count, &powercase);
	if (ret) {
		SSR_DBG(" SSR_debug: Unable to Parse Input");
		return ret;
	}

	SSR_DBG("SSR_debug: POWER: %d\n", powercase);

	pcie_ssr_sel_power_case(ctrl, powercase);

	return count;
}

static const struct file_operations pcie_ssr_power_ops = {
	.open = pcie_ssr_power_open,
	.release = single_release,
	.read = seq_read,
	.write = pcie_ssr_power_select,
};

static int pcie_ssr_debugfs_init(void) {
	dent_pcie_ssr = debugfs_create_dir("pci-ssr", NULL);
	if (IS_ERR(dent_pcie_ssr)) {
		SSR_DBG("PCIe: fail to create the folder for debug_fs.\n");
		return -EINVAL;
	}

	dfile_power = debugfs_create_file("power", 0664,
					dent_pcie_ssr, NULL,&pcie_ssr_power_ops);

	if (!dfile_power || IS_ERR(dfile_power)) {
			SSR_DBG("PCIe: fail to create the file for debug_fs case.\n");
			goto power_error;
	}
	return 0;

power_error:
	debugfs_remove(dfile_power);
	return -EINVAL;

}

void register_pcie_ssr(void *priv) {
	ctrl = priv;
}
EXPORT_SYMBOL(register_pcie_ssr);

static int ssr_probe(struct platform_device *pdev) {
	int len,ret;
	const char *name;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;

	pr_crit("SSR_debug: Probing PCIe SSR driver \n\n");
	SSR_DBG("SSR_debug: Parsing devicenode properties \n\n");
	name = of_get_property(np, "compatible", &len);
	SSR_DBG("SSR_debug: Compatible : %s\n : Device Successfully Probed", name);
	return 0;
}

static void ssr_remove(struct platform_device *pdev) {
	pr_notice("SSR_debug:PCIe SSR Driver Remove Called");
	//return 0;
}

static const struct of_device_id mymatch[] = {
	{ .compatible = "qcom,pcie_ssr" },
	{},
};

MODULE_DEVICE_TABLE(of, mymatch);

static struct platform_driver ssr_driver = {
	.probe		= ssr_probe,
	.remove		= ssr_remove,
	.driver = {
		.name	= "PCIE_SSR",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(mymatch),
	},
};

static int __init ssr_register(void) {
	char ipc_log_name[MAX_IPC_LOG_NAME_LEN];

	pr_notice("PCIe SSR Driver Init Called");

	snprintf(ipc_log_name, MAX_IPC_LOG_NAME_LEN, "pcie_ssr-short");
	ipc_log =ipc_log_context_create(PCIE_SSR_LOG_PAGES, ipc_log_name, 0);
	if (ipc_log == NULL)
		pr_err("%s: unable to create IPC log context for %s\n",
				__func__, ipc_log_name);
	else
		SSR_DBG("SSR_debug:IPC logging: %s is enable",ipc_log_name);

	pcie_ssr_debugfs_init();

	return platform_driver_register(&ssr_driver);
}
module_init(ssr_register);

static void __exit ssr_unregister(void) {
	pr_notice("SSR_debug:SSR Driver Exit Called");
	platform_driver_unregister(&ssr_driver);
}
module_exit(ssr_unregister);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("PCIE_SSR");
