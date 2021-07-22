/* Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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

#include <linux/debugfs.h>
#include <linux/list.h>
#include <linux/msm-bus.h>
#include <linux/msm_pcie.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/mhi.h>
#include "mhi_qcom.h"

struct arch_info {
	struct mhi_dev *mhi_dev;
	struct msm_bus_scale_pdata *msm_bus_pdata;
	u32 bus_client;
	struct msm_pcie_register_event pcie_reg_event;
	struct pci_saved_state *pcie_state;
	struct pci_saved_state *ref_pcie_state;
};

#define MHI_IPC_LOG_PAGES (100)

enum MHI_DEBUG_LEVEL  mhi_ipc_log_lvl = MHI_MSG_LVL_VERBOSE;

static int mhi_arch_set_bus_request(struct mhi_controller *mhi_cntrl, int index)
{
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
	struct arch_info *arch_info = mhi_dev->arch_info;

	MHI_LOG("Setting bus request to index %d\n", index);

	if (arch_info->bus_client)
		return msm_bus_scale_client_update_request(
							arch_info->bus_client,
							index);

	/* default return success */
	return 0;
}

static void mhi_arch_pci_link_state_cb(struct msm_pcie_notify *notify)
{
	struct mhi_controller *mhi_cntrl = notify->data;
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
	struct pci_dev *pci_dev = mhi_dev->pci_dev;

	switch (notify->event) {
	case MSM_PCIE_EVENT_WAKEUP:
		MHI_LOG("Received PCIE_WAKE signal\n");

		/* bring link out of d3cold */
		if (mhi_dev->powered_on) {
			pm_runtime_get(&pci_dev->dev);
			pm_runtime_put_noidle(&pci_dev->dev);
		}
		break;
	default:
		MHI_LOG("Unhandled event 0x%x\n", notify->event);
	}
}

static void mhi_arch_link_off(struct mhi_controller *mhi_cntrl)
{
		struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
		struct pci_dev *pci_dev = mhi_dev->pci_dev;
		int ret;

		MHI_LOG("Entered\n");

		ret = pci_set_power_state(pci_dev, PCI_D3hot);
		if (ret)
			MHI_ERR("Failed to set D3hot, ret:%d\n", ret);

		/* release the resources */
		msm_pcie_pm_control(MSM_PCIE_SUSPEND, mhi_cntrl->bus, pci_dev, NULL, 0);
		mhi_arch_set_bus_request(mhi_cntrl, 0);

		MHI_LOG("Exited\n");
}

void mhi_arch_pcie_ssr_ops_power_off(void *priv)
{
	struct mhi_controller *mhi_cntrl = priv;
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
	struct arch_info *arch_info = mhi_dev->arch_info;
	struct pci_dev *pci_dev = mhi_dev->pci_dev;

	/*
	 * Abort system suspend if system is preparing to go to suspend
	 * by grabbing wake source.
	 * If system is suspended, wait for pm notifier callback to notify
	 * that resume has occurred with PM_POST_SUSPEND event.
	 */
	pm_stay_awake(&mhi_cntrl->mhi_dev->dev);

	/* if link is in drv suspend, wake it up */
	pm_runtime_get_sync(&pci_dev->dev);

	mutex_lock(&mhi_cntrl->pm_mutex);
	if (!mhi_dev->powered_on) {
		MHI_LOG("Not in active state\n");
		mutex_unlock(&mhi_cntrl->pm_mutex);
		pm_runtime_put_noidle(&pci_dev->dev);
	}
	else {
		mhi_dev->powered_on = false;
		mutex_unlock(&mhi_cntrl->pm_mutex);

		pm_runtime_put_noidle(&pci_dev->dev);

		MHI_LOG("Triggering shutdown process\n");

		mhi_power_down(mhi_cntrl, 1);

		/* turn the link off */
		mhi_deinit_pci_dev(mhi_cntrl);

		mhi_arch_link_off(mhi_cntrl);

		mhi_arch_iommu_deinit(mhi_cntrl);

		mhi_arch_pcie_deinit(mhi_cntrl);

		pm_relax(&mhi_cntrl->mhi_dev->dev);
	}
}
EXPORT_SYMBOL(mhi_arch_pcie_ssr_ops_power_off);

int mhi_arch_pcie_ssr_ops_power_on(void *priv)
{
	struct mhi_controller *mhi_cntrl = priv;
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
	struct pci_dev *pci_dev = mhi_dev->pci_dev;
	int ret;

	mutex_lock(&mhi_cntrl->pm_mutex);
	if (mhi_dev->powered_on) {
		MHI_LOG("MHI still in active state\n");
		mutex_unlock(&mhi_cntrl->pm_mutex);
		return 0;
	}

	MHI_LOG("Enter\n");

	/* reset rpm state */
	pm_runtime_set_active(&pci_dev->dev);

	pm_runtime_enable(&pci_dev->dev);

	mutex_unlock(&mhi_cntrl->pm_mutex);

	pm_runtime_forbid(&pci_dev->dev);

	ret = pm_runtime_get_sync(&pci_dev->dev);
	if (ret < 0) {
		MHI_ERR("Error with rpm resume, ret:%d\n", ret);
		return ret;
	}

	/* re-start the link & recover default cfg states */
	ret = msm_pcie_pm_control(MSM_PCIE_RESUME, pci_dev->bus->number,
				  pci_dev, NULL, 0);
	if (ret) {
		MHI_ERR("Failed to resume pcie bus ret %d\n", ret);
		return ret;
	}

	mhi_pci_probe(pci_dev, NULL);
	return 0;
}
EXPORT_SYMBOL(mhi_arch_pcie_ssr_ops_power_on);

int mhi_arch_pcie_init(struct mhi_controller *mhi_cntrl)
{
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
	struct arch_info *arch_info = mhi_dev->arch_info;
	char node[32];
	int ret = 0;

	if (!arch_info) {
		struct msm_pcie_register_event *reg_event;

		arch_info = devm_kzalloc(&mhi_dev->pci_dev->dev,
					 sizeof(*arch_info), GFP_KERNEL);
		if (!arch_info)
			return -ENOMEM;

		mhi_dev->arch_info = arch_info;
		arch_info->mhi_dev = mhi_dev;

		snprintf(node, sizeof(node), "mhi_%04x_%02u.%02u.%02u",
			 mhi_cntrl->dev_id, mhi_cntrl->domain, mhi_cntrl->bus,
			 mhi_cntrl->slot);
		mhi_cntrl->log_buf = ipc_log_context_create(MHI_IPC_LOG_PAGES,
							    node, 0);
		mhi_cntrl->log_lvl = mhi_ipc_log_lvl;

		/* register for bus scale if defined */
		arch_info->msm_bus_pdata = msm_bus_cl_get_pdata_from_dev(
							&mhi_dev->pci_dev->dev);
		if (arch_info->msm_bus_pdata) {
			arch_info->bus_client =
				msm_bus_scale_register_client(
						arch_info->msm_bus_pdata);
			if (!arch_info->bus_client) {
				MHI_LOG("Error:No bus client for controller");
				return -EINVAL;
			}
		}

		/* register with pcie rc for WAKE# or link state events */
		reg_event = &arch_info->pcie_reg_event;
		reg_event->events = MSM_PCIE_EVENT_WAKEUP;

		reg_event->user = mhi_dev->pci_dev;
		reg_event->callback = mhi_arch_pci_link_state_cb;
		reg_event->notify.data = mhi_cntrl;
		ret = msm_pcie_register_event(reg_event);
		if (ret)
			MHI_LOG(
				"Failed to reg. for link up notification\n");
	}
	return ret;
}

int mhi_arch_link_suspend(struct mhi_controller *mhi_cntrl)
{
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
	struct arch_info *arch_info = mhi_dev->arch_info;
	struct pci_dev *pci_dev = mhi_dev->pci_dev;
	int ret = 0;

	MHI_LOG("Entered\n");

	switch (mhi_dev->suspend_mode) {
	case MHI_DEFAULT_SUSPEND:
		pci_clear_master(pci_dev);
		ret = pci_save_state(mhi_dev->pci_dev);
		if (ret) {
			MHI_ERR("Failed with pci_save_state, ret:%d\n", ret);
			goto exit_suspend;
		}
		arch_info->pcie_state = pci_store_saved_state(pci_dev);
		pci_disable_device(pci_dev);
		pci_set_power_state(pci_dev, PCI_D3hot);
		/* release the resources */
		msm_pcie_pm_control(MSM_PCIE_SUSPEND, mhi_cntrl->bus, pci_dev,
				    NULL, 0);
		mhi_arch_set_bus_request(mhi_cntrl, 0);
		break;
	case MHI_ACTIVE_STATE:
		break;
	}

exit_suspend:
	MHI_LOG("Exited with ret:%d\n", ret);
	return ret;
}

static int __mhi_arch_link_resume(struct mhi_controller *mhi_cntrl)
{
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
	struct arch_info *arch_info = mhi_dev->arch_info;
	struct pci_dev *pci_dev = mhi_dev->pci_dev;
	struct mhi_link_info *cur_info = &mhi_cntrl->mhi_link_info;
	int ret;

	MHI_LOG("Entered\n");

	/* request bus scale voting based on higher gen speed */
	ret = mhi_arch_set_bus_request(mhi_cntrl,
				       cur_info->target_link_speed);
	if (ret)
		MHI_LOG("Could not set bus frequency, ret:%d\n", ret);

	ret = msm_pcie_pm_control(MSM_PCIE_RESUME, mhi_cntrl->bus, pci_dev,
				  NULL, 0);
	if (ret) {
		MHI_ERR("Link training failed, ret:%d\n", ret);
		return ret;
	}

	ret = pci_set_power_state(pci_dev, PCI_D0);
	if (ret) {
		MHI_ERR("Failed to set PCI_D0 state, ret:%d\n", ret);
		return ret;
	}

	ret = pci_enable_device(pci_dev);
	if (ret) {
		MHI_ERR("Failed to enable device, ret:%d\n", ret);
		return ret;
	}

	ret = pci_load_and_free_saved_state(pci_dev, &arch_info->pcie_state);
	if (ret)
		MHI_LOG("Failed to load saved cfg state\n");

	pci_restore_state(pci_dev);
	pci_set_master(pci_dev);

	return 0;
}

int mhi_arch_link_resume(struct mhi_controller *mhi_cntrl)
{
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
	struct pci_dev *pci_dev = mhi_dev->pci_dev;
	int ret = 0;

	MHI_LOG("Entered\n");

	switch (mhi_dev->suspend_mode) {
	case MHI_DEFAULT_SUSPEND:
		ret = __mhi_arch_link_resume(mhi_cntrl);
		break;
	case MHI_ACTIVE_STATE:
		break;
	}

	if (ret) {
		MHI_ERR("Link training failed, ret:%d\n", ret);
		return ret;
	}

	MHI_LOG("Exited\n");

	return 0;
}
