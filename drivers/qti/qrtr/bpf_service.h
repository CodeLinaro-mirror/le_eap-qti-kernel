/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __BPF_SERVICE_H_
#define __BPF_SERVICE_H_

#include <linux/types.h>
#include <linux/version.h>
#include <linux/qrtr.h>
#include <linux/rcupdate.h>

#ifndef rcu_replace_pointer
/**
 * rcu_replace_pointer() - replace an RCU pointer, returning its old value
 * @rcu_ptr: RCU pointer, whose old value is returned
 * @ptr: regular pointer
 * @c: the lockdep conditions under which the dereference will take place
 *
 * Perform a replacement, where @rcu_ptr is an RCU-annotated
 * pointer and @c is the lockdep argument that is passed to the
 * rcu_dereference_protected() call used to read that pointer.  The old
 * value of @rcu_ptr is returned, and @rcu_ptr is set to @ptr.
 */
#define rcu_replace_pointer(rcu_ptr, ptr, c)                           \
({                                                                     \
	typeof(ptr) __tmp = rcu_dereference_protected((rcu_ptr), (c));  \
	rcu_assign_pointer((rcu_ptr), (ptr));                           \
	__tmp;                                                          \
})
#endif

/* structure to use with service add/lookup/remove */
struct service_info {
	u32 service_id;
	u32 instance_id;
	u32 node_id;
};

void qrtr_service_add(struct qrtr_ctrl_pkt *pkt);
void qrtr_service_remove(struct qrtr_ctrl_pkt *pkt);
void qrtr_service_node_remove(u32 src_node);
int qrtr_service_lookup(u32 node, u32 port, struct service_info **info);
int qrtr_bpf_filter_attach(int ufd);
int qrtr_bpf_filter_detach(void);
int qrtr_run_bpf_filter(struct sk_buff *skb, u32 service_id,
			u32 instance_id, u8 pkt_type, u32 dest_node);
#endif /* __BPF_SERVICE_H_ */
