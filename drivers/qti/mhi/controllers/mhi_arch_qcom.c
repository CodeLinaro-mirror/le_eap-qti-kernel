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

int mhi_arch_link_off(struct mhi_controller *mhi_cntrl, bool graceful)
{
       struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
       struct arch_info *arch_info = mhi_dev->arch_info;
       struct pci_dev *pci_dev = mhi_dev->pci_dev;
       int ret;

       MHI_LOG("Entered\n");

       if (graceful) {
               ret = pci_save_state(mhi_dev->pci_dev);
               if (ret) {
                       MHI_ERR("Failed with pci_save_state, ret:%d\n", ret);
                       return ret;
               }

               arch_info->pcie_state = pci_store_saved_state(pci_dev);
               pci_disable_device(pci_dev);
               ret = pci_set_power_state(pci_dev, PCI_D3hot);
               if (ret) {
                       MHI_ERR("Failed to set D3hot, ret:%d\n", ret);
                       return ret;
               }
       }

       /* release the resources */
       msm_pcie_pm_control(MSM_PCIE_SUSPEND, mhi_cntrl->bus, pci_dev, NULL, 0);
       mhi_arch_set_bus_request(mhi_cntrl, 0);

       MHI_LOG("Exited\n");

       return 0;
}

int mhi_arch_link_on(struct mhi_controller *mhi_cntrl)
{
       struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
       struct arch_info *arch_info = mhi_dev->arch_info;
       struct pci_dev *pci_dev = mhi_dev->pci_dev;
       int ret;

       MHI_LOG("Entered\n");

       /* request resources and establish link trainning */
       ret = mhi_arch_set_bus_request(mhi_cntrl, 1);
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

       MHI_LOG("Exited\n");

       return 0;
}

int mhi_arch_pcie_init(struct mhi_controller *mhi_cntrl)
{
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
	struct arch_info *arch_info = mhi_dev->arch_info;
	struct mhi_link_info *cur_link_info;
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
