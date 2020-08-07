/* Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * RMNET Data Sig sender
 *
 */
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/siginfo.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/sched/signal.h>
#include <linux/moduleparam.h>
#include <linux/workqueue.h>
#include <linux/pid.h>
#include <linux/preempt.h>

#include "rmnet_sig.h"

#define SIG_RMNET 44
#define MAX_APP_LEN 5
#define APP_NAME "lptd"
#define BIT0 0

unsigned long rmnet_timeout = 5000;
module_param(rmnet_timeout, ulong, 0644);
MODULE_PARM_DESC(rmnet_timeout, "Time before rmnet is inactive (ms)");

static int rmnet_sig_query(const char *buf, const struct kernel_param *kp);

DEFINE_SPINLOCK(rmnet_sig_splock);
unsigned long rmnet_pid;

static const struct kernel_param_ops rmnet_sig_query_ops = {
	.set = rmnet_sig_query,
	.get = param_get_ulong,
};
module_param_cb(rmnet_pid, &rmnet_sig_query_ops, &rmnet_pid, 0644);
MODULE_PARM_DESC(rmnet_pid, "Process ID of receiver of signal");

static struct rmnet_sig_s rmnet_sig_cfg;

static int rmnet_send_sig_to_lptd(void)
{
	int ret;
	unsigned long flags;
	struct task_struct *task;
	struct siginfo info = {
		.si_signo = SIG_RMNET,
		.si_code = SI_QUEUE,
		.si_int = rmnet_sig_cfg.data_active,
	};

	rcu_read_lock();

	local_bh_disable();
	spin_lock_irqsave(&rmnet_sig_splock, flags);
	task = find_task_by_vpid(rmnet_pid);
	if (!task || strncmp(task->comm, APP_NAME, MAX_APP_LEN) != 0) {
		spin_unlock_irqrestore(&rmnet_sig_splock, flags);
		local_bh_enable();
		rcu_read_unlock();
		return -ENODEV;
	}
	spin_unlock_irqrestore(&rmnet_sig_splock, flags);
	local_bh_enable();

	rcu_read_unlock();

	ret = send_sig_info(SIG_RMNET, &info, task);
	if (ret < 0)
		return ret;

	return 0;
}

static int rmnet_sig_query(const char *buf, const struct kernel_param *kp)
{
	int rc;
	unsigned long flags;

	local_bh_disable();
	spin_lock_irqsave(&rmnet_sig_splock, flags);
	rc = param_set_ulong(buf, kp);
	spin_unlock_irqrestore(&rmnet_sig_splock, flags);
	local_bh_enable();

	/* Time out before purely inactive isolation is reclaimed */
	queue_delayed_work(rmnet_sig_cfg.rmnet_wq,
				   &rmnet_sig_cfg.sig_work,
				   msecs_to_jiffies(rmnet_timeout));
	return rc;
}
static void rmnet_check_state(struct work_struct *work)
{
	rcu_read_lock();


	if (rmnet_sig_cfg.data_sent) {
		queue_delayed_work(rmnet_sig_cfg.rmnet_wq,
				   &rmnet_sig_cfg.sig_work,
				   msecs_to_jiffies(rmnet_timeout));
	} else {
		clear_bit(BIT0, &rmnet_sig_cfg.data_active);
		rmnet_send_sig_to_lptd();

	}
	clear_bit(BIT0, &rmnet_sig_cfg.data_sent);

	rcu_read_unlock();
}

int rmnet_active_check(void)
{
	rcu_read_lock();

	if (!test_and_set_bit(BIT0, &rmnet_sig_cfg.data_active)) {
		rmnet_send_sig_to_lptd();
		/* Cancel potential sysfs queue */
		cancel_delayed_work_sync(&rmnet_sig_cfg.sig_work);
		queue_delayed_work(rmnet_sig_cfg.rmnet_wq,
				   &rmnet_sig_cfg.sig_work,
				   msecs_to_jiffies(rmnet_timeout));
		goto end;
	}
	set_bit(BIT0, &rmnet_sig_cfg.data_sent);

end:
	rcu_read_unlock();
	return 0;
}

int rmnet_sig_init(void)
{
	if(rmnet_sig_cfg.rmnet_wq)
		return 0;

	rmnet_sig_cfg.rmnet_wq = alloc_workqueue("rmnet_sig_wq",
						 WQ_SYSFS | WQ_HIGHPRI | WQ_UNBOUND, 1);

	if (!rmnet_sig_cfg.rmnet_wq)
		return -ENODEV;
	INIT_DEFERRABLE_WORK(&rmnet_sig_cfg.sig_work, rmnet_check_state);
	return 0;
}

int rmnet_sig_exit(void)
{
	cancel_delayed_work_sync(&rmnet_sig_cfg.sig_work);
	destroy_workqueue(rmnet_sig_cfg.rmnet_wq);
	rmnet_sig_cfg.data_active = 0;
	rmnet_pid = 0;
	rmnet_sig_cfg.data_sent = 0;
	rmnet_sig_cfg.rmnet_wq = NULL;
}
#else

/* Stubs for legacy kernels */
int rmnet_sig_init(void) { return 0; }

int rmnet_sig_exit(void) { return 0; }

int rmnet_active_check(void) { return 0; }

#endif
