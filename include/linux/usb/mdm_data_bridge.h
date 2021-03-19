/* Copyright (c) 2011-2013, 2019-2021, The Linux Foundation. All rights reserved.
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


#ifndef __LINUX_MDM_DATA_BRIDGE_H__
#define __LINUX_MDM_DATA_BRIDGE_H__

#include <linux/netdevice.h>
#include <linux/usb.h>

#define MAX_INST_NAME_LEN 40

#define ACM_CTRL_DTR		(1 << 0)
#define ACM_CTRL_RTS		(1 << 1)
#define ACM_CTRL_BRK		(1 << 2)
#define ACM_CTRL_RI		(1 << 3)
#define ACM_CTRL_FRAMING	(1 << 4)
#define ACM_CTRL_PARITY		(1 << 5)
#define ACM_CTRL_OVERRUN	(1 << 6)

#define ACM_CTRL_DCD		(1 << 0)
#define ACM_CTRL_DSR		(1 << 1)

enum bridge_id {
	USB_BRIDGE_QDSS,
	USB_BRIDGE_DPL,
	USB_BRIDGE_EDL,
	USB_BRIDGE_DIAG,
	USB_BRIDGE_DUMP,
	USB_BRIDGE_RMNET,
	USB_BRIDGE_SERIAL,
	MAX_BRIDGE_DEVICES,
};

static int bridge_name_to_id(const char *name)
{
	if (!name)
		goto fail;

	if (!strncasecmp(name, "qdss", MAX_INST_NAME_LEN))
		return USB_BRIDGE_QDSS;
	if (!strncasecmp(name, "dpl", MAX_INST_NAME_LEN))
		return USB_BRIDGE_DPL;
	if (!strncasecmp(name, "edl", MAX_INST_NAME_LEN))
		return USB_BRIDGE_EDL;
	if (!strncasecmp(name, "diag", MAX_INST_NAME_LEN))
		return USB_BRIDGE_DIAG;
	if (!strncasecmp(name, "dump", MAX_INST_NAME_LEN))
		return USB_BRIDGE_DUMP;
	if (!strncasecmp(name, "rmnet", MAX_INST_NAME_LEN))
		return USB_BRIDGE_RMNET;
	if (!strncasecmp(name, "serial", MAX_INST_NAME_LEN))
		return USB_BRIDGE_SERIAL;

fail:
	return -EINVAL;
}

static int bridge_id_to_protocol(enum bridge_id id)
{
	switch (id) {
	case USB_BRIDGE_QDSS:
		return 0x70;
	case USB_BRIDGE_DPL:
		return 0x80;
	case USB_BRIDGE_EDL:
	case USB_BRIDGE_DUMP:
		return 0x10;
	case USB_BRIDGE_DIAG:
		return 0x30;
	case USB_BRIDGE_RMNET:
		return 0x50;
	case USB_BRIDGE_SERIAL:
		return 0x40;
	default:
		return -EINVAL;
	}
}

struct bridge_ops {
	int (*send_pkt)(void *, void *, size_t actual);
	void (*send_cbits)(void *, unsigned int);

	/* flow control */
	void (*unthrottle_tx)(void *);
};

#define TX_THROTTLED BIT(0)
#define RX_THROTTLED BIT(1)

struct bridge {
	/* context of the gadget port using bridge driver */
	void *ctx;

	/*to maps bridge driver instance*/
	unsigned int ch_id;

	/*to match against bridge xport name to get bridge driver instance*/
	char *name;

	/* flow control bits */
	unsigned long flags;

	/* data/ctrl bridge callbacks */
	struct bridge_ops ops;
};

/**
 * timestamp_info: stores timestamp info for skb life cycle during data
 * transfer for tethered rmnet/DUN.
 * @created: stores timestamp at the time of creation of SKB.
 * @rx_queued: stores timestamp when SKB queued to HW to receive
 * data.
 * @rx_done: stores timestamp when skb queued to h/w is completed.
 * @rx_done_sent: stores timestamp when SKB is sent from gadget rmnet/DUN
 * driver to bridge rmnet/DUN driver or vice versa.
 * @tx_queued: stores timestamp when SKB is queued to send data.
 *
 * note that size of this struct shouldn't exceed 48bytes that's the max skb->cb
 * holds.
 */
struct timestamp_info {
	struct data_bridge	*dev;

	unsigned int		created;
	unsigned int		rx_queued;
	unsigned int		rx_done;
	unsigned int		rx_done_sent;
	unsigned int		tx_queued;
};

/* Maximum timestamp message length */
#define DBG_DATA_MSG	128UL

/* Maximum timestamp messages */
#define DBG_DATA_MAX	32UL

/* timestamp buffer descriptor */
struct timestamp_buf {
	char		(buf[DBG_DATA_MAX])[DBG_DATA_MSG];   /* buffer */
	unsigned int	idx;   /* index */
	rwlock_t	lck;   /* lock */
};

#if defined(CONFIG_QTI_USB_MDM_DATA_BRIDGE) ||	\
	defined(CONFIG_QTI_USB_MDM_DATA_BRIDGE_MODULE)

/* Bridge APIs called by gadget driver */
int data_bridge_open(struct bridge *brdg);
void data_bridge_close(unsigned int id);
int data_bridge_write(unsigned int id, struct sk_buff *skb);
int data_bridge_unthrottle_rx(unsigned int id);
int ctrl_bridge_open(struct bridge *brdg);
void ctrl_bridge_close(unsigned int id);
void ctrl_bridge_write(unsigned int id, char *buf, size_t len);
void ctrl_bridge_set_cbits(unsigned int id, unsigned int cbits);
unsigned int ctrl_bridge_get_cbits_tohost(unsigned int id);

/* defined in control bridge */
int ctrl_bridge_init(void);
void ctrl_bridge_exit(void);
int ctrl_bridge_probe(struct usb_interface *ifc, struct usb_host_endpoint *ep,
		char *name, int id);
void ctrl_bridge_disconnect(unsigned int id);
int ctrl_bridge_resume(unsigned int id);
int ctrl_bridge_suspend(unsigned int id);

#else

static inline int __maybe_unused data_bridge_open(struct bridge *brdg)
{
	return -ENODEV;
}

static inline void __maybe_unused data_bridge_close(unsigned int id) { }

static inline int __maybe_unused data_bridge_write(unsigned int id,
					    struct sk_buff *skb)
{
	return -ENODEV;
}

static inline int __maybe_unused data_bridge_unthrottle_rx(unsigned int id)
{
	return -ENODEV;
}

static inline int __maybe_unused ctrl_bridge_open(struct bridge *brdg)
{
	return -ENODEV;
}

static inline void __maybe_unused ctrl_bridge_close(unsigned int id) { }

static inline void __maybe_unused ctrl_bridge_write(unsigned int id,
						char *data, size_t size) { }

static inline void __maybe_unused ctrl_bridge_set_cbits(unsigned int id,
					unsigned int cbits) { }

static inline unsigned int __maybe_unused
ctrl_bridge_get_cbits_tohost(unsigned int id)
{
	return -ENODEV;
}

#endif

#endif
