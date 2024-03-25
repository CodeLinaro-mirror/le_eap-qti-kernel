# Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of The Linux Foundation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
# ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Changes from Qualcomm Innovation Center are provided under the following license:
# Copyright (c) 2023,2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

FILESEXTRAPATHS:append := ":${WORKSPACE}/sources"

SRC_URI += "file://eap-qti-kernel"

LINUX_VERSION ??= "${PV}"

do_unpack_extra () {
    if [ "${@d.getVar('LINUX_VERSION')[0:3]}" = "5.4" ]; then
	rm -rf ${WORKDIR}/eap-qti-kernel/drivers
	mv ${WORKDIR}/eap-qti-kernel/drivers-5.x ${WORKDIR}/eap-qti-kernel/drivers
    fi
    if [ "${@d.getVar('LINUX_VERSION')[0:4]}" = "5.15" ]; then
        rm -rf ${WORKDIR}/eap-qti-kernel/drivers
        mv ${WORKDIR}/eap-qti-kernel/drivers-5.15 ${WORKDIR}/eap-qti-kernel/drivers
    fi
    if [ "${@d.getVar('LINUX_VERSION')[0:3]}" == "6.1" ]; then
            rm -rf ${WORKDIR}/eap-qti-kernel/drivers
	    cp -r ${WORKDIR}/eap-qti-kernel/drivers-6.x ${WORKDIR}/eap-qti-kernel/drivers
    fi

    cp -r ${WORKDIR}/eap-qti-kernel/* ${S}/

    if [ -e ${S}/arch/arm/boot/dts ]; then
        cp ${WORKDIR}/eap-qti-kernel/devicetree/* ${S}/arch/arm/boot/dts/
    fi

    if [ -e ${S}/arch/arm64/boot/dts/freescale ]; then
        cp ${WORKDIR}/eap-qti-kernel/devicetree/* ${S}/arch/arm64/boot/dts/freescale/
    fi

    echo "source \"drivers/qti/Kconfig\"" >> ${S}/drivers/Kconfig
    echo "obj-y += qti/" >> ${S}/drivers/Makefile
    echo "header-y += qrtr.h" >> ${S}/include/uapi/linux/Kbuild
    echo "header-y += msm_rmnet.h" >> ${S}/include/uapi/linux/Kbuild
    echo "header-y += rmnet_data.h" >> ${S}/include/uapi/linux/Kbuild

    if [ -e ${S}/arch/arm/boot/dts/imx6q-sabrelite.dts ]; then
        echo "#include \"sa525m-mhi.dtsi\"" >> ${S}/arch/arm/boot/dts/imx6q-sabrelite.dts
	echo "#include \"eap_diag.dtsi\"" >> ${S}/arch/arm/boot/dts/imx6q-sabrelite.dts
    fi

    if [ -e ${S}/arch/arm/boot/dts/imx6sx.dtsi ]; then
        echo "#include \"sa525m-mhi.dtsi\"" >> ${S}/arch/arm/boot/dts/imx6sx.dtsi
	echo "#include \"eap_diag.dtsi\"" >> ${S}/arch/arm/boot/dts/imx6sx.dtsi
    fi

    if [ -e ${S}/arch/arm/boot/dts/imx6qdl.dtsi ]; then
        echo "#include \"sa525m-mhi.dtsi\"" >> ${S}/arch/arm/boot/dts/imx6qdl.dtsi
	echo "#include \"eap_diag.dtsi\"" >> ${S}/arch/arm/boot/dts/imx6qdl.dtsi
    fi

    if [ -e ${S}/arch/arm64/boot/dts/freescale/fsl-imx8dx.dtsi ]; then
        echo "#include \"sa525m-mhi.dtsi\"" >> ${S}/arch/arm64/boot/dts/freescale/fsl-imx8dx.dtsi
	echo "#include \"eap_diag.dtsi\"" >> ${S}/arch/arm64/boot/dts/freescale/fsl-imx8dx.dtsi
    fi

    if [ -e ${S}/arch/arm64/boot/dts/freescale/imx8qxp.dtsi ]; then
        echo "#include \"sa525m-mhi.dtsi\"" >> ${S}/arch/arm64/boot/dts/freescale/imx8qxp.dtsi
        echo "#include \"eap_diag.dtsi\"" >> ${S}/arch/arm64/boot/dts/freescale/imx8qxp.dtsi
    fi

    if [ "${@d.getVar('LINUX_VERSION')[0:3]}" = "4.9" ]; then
        cd ${S}
        git apply ${WORKDIR}/eap-qti-kernel/patches-4.9/0001-mhi_bus-core-Add-support-for-MHI-host-interface.patch
        git apply ${WORKDIR}/eap-qti-kernel/patches-4.9/0002-net-Add-the-get-current-NAPI-context-API.patch
        git apply ${WORKDIR}/eap-qti-kernel/patches-4.9/0003-msm_rmnet-merge-support-for-RAWIP-msm_rmnet-device.patch
        git apply ${WORKDIR}/eap-qti-kernel/patches-4.9/0004-net-rmnet_data-Add-snapshot-of-rmnet_data-driver.patch
        git apply ${WORKDIR}/eap-qti-kernel/patches-4.9/0005-net-ipv6-Generate-random-IID-for-addresses-on-RAWIP-.patch
        git apply ${WORKDIR}/eap-qti-kernel/patches-4.9/0006-Add-support-for-upstream-rmnet-driver.patch
        git apply ${WORKDIR}/eap-qti-kernel/patches-4.9/0007-net-cdc_ether-Ensure-that-default-MTU-is-minimum-204.patch
        cd -
    elif [ "${@d.getVar('LINUX_VERSION')[0:4]}" = "4.14" ]; then
        cd ${S}
        git apply ${WORKDIR}/eap-qti-kernel/patches-4.14/0001-mhi_bus-core-Add-support-for-MHI-host-interface.patch
        git apply ${WORKDIR}/eap-qti-kernel/patches-4.14/0002-net-Add-the-get-current-NAPI-context-API.patch
        git apply ${WORKDIR}/eap-qti-kernel/patches-4.14/0003-net-ipv6-Generate-random-IID-for-addresses-on-RAWIP-.patch
        git apply ${WORKDIR}/eap-qti-kernel/patches-4.14/0004-Add-support-for-upstream-rmnet-driver.patch
        git apply ${WORKDIR}/eap-qti-kernel/patches-4.14/0005-net-cdc_ether-Ensure-that-default-MTU-is-minimum-204.patch
        git apply ${WORKDIR}/eap-qti-kernel/patches-4.14/0006-net-usbnet-Update-net_device-stats-on-tx-and-rx-succ.patch
        git apply ${WORKDIR}/eap-qti-kernel/patches-4.14/0007-net-usbnet-Update-net_device-stats-for-RmNet-device-.patch
        cd -
    elif [ "${@d.getVar('LINUX_VERSION')[0:3]}" = "5.4" ]; then
        cd ${S}
        sed -e '/CONFIG_QTI_RMNET/d' ${WORKDIR}/eap-qti-kernel/defconfig -i ${WORKDIR}/eap-qti-kernel/defconfig
        sed -e '/CONFIG_QTI_QMI_RMNET/d' ${WORKDIR}/eap-qti-kernel/defconfig -i ${WORKDIR}/eap-qti-kernel/defconfig
        sed -e '/CONFIG_QTI_RMNET_CTL/d' ${WORKDIR}/eap-qti-kernel/defconfig -i ${WORKDIR}/eap-qti-kernel/defconfig
        sed -e '/CONFIG_QTI_QMI_DFC/d' ${WORKDIR}/eap-qti-kernel/defconfig -i ${WORKDIR}/eap-qti-kernel/defconfig
        git apply ${WORKDIR}/eap-qti-kernel/patches-5.4/008-Add-support-for-kernel-API-used-in-daig-and-mhi.patch
        git apply ${WORKDIR}/eap-qti-kernel/patches-5.4/009-mhi_bus-core-Add-support-for-MHI-host-interface.patch
        git apply ${WORKDIR}/eap-qti-kernel/patches-5.4/010-net-cdc_ether-Ensure-that-default-MTU-is-minimum-204.patch
        git apply ${WORKDIR}/eap-qti-kernel/patches-5.4/011-net-usbnet-Update-net_device-stats-on-tx-and-rx-succ.patch
        git apply ${WORKDIR}/eap-qti-kernel/patches-5.4/012-net-usbnet-Update-net_device-stats-for-RmNet-device-.patch
        git apply ${WORKDIR}/eap-qti-kernel/patches-5.4/013-usb-gadget-Fix-crash-on-calling-usb-gadget-activate-.patch
        cd -
    elif [ "${@d.getVar('LINUX_VERSION')[0:4]}" = "5.15" ]; then
        cd ${S}
	sed -e '/CONFIG_QTI_RMNET/d' ${WORKDIR}/eap-qti-kernel/defconfig -i ${WORKDIR}/eap-qti-kernel/defconfig
        sed -e '/CONFIG_QTI_QMI_RMNET/d' ${WORKDIR}/eap-qti-kernel/defconfig -i ${WORKDIR}/eap-qti-kernel/defconfig
        sed -e '/CONFIG_QTI_RMNET_CTL/d' ${WORKDIR}/eap-qti-kernel/defconfig -i ${WORKDIR}/eap-qti-kernel/defconfig
        sed -e '/CONFIG_QTI_QMI_DFC/d' ${WORKDIR}/eap-qti-kernel/defconfig -i ${WORKDIR}/eap-qti-kernel/defconfig
        if git apply --check "${WORKDIR}/eap-qti-kernel/patches-5.15/008-Add-support-for-kernel-API-used-in-daig-and-mhi.patch"; then
                echo "success"
                git apply ${WORKDIR}/eap-qti-kernel/patches-5.15/008-Add-support-for-kernel-API-used-in-daig-and-mhi.patch
        else
                echo "fail"
        fi
        if git apply --check "${WORKDIR}/eap-qti-kernel/patches-5.15/010-net-cdc_ether-Ensure-that-default-MTU-is-minimum-204.patch"; then
                echo "success"
                git apply ${WORKDIR}/eap-qti-kernel/patches-5.15/010-net-cdc_ether-Ensure-that-default-MTU-is-minimum-204.patch
        else
                echo "fail"
        fi
        if git apply --check "${WORKDIR}/eap-qti-kernel/patches-5.15/012-net-usbnet-Update-net_device-stats-for-RmNet-device-.patch"; then
                echo "success"
        git apply ${WORKDIR}/eap-qti-kernel/patches-5.15/012-net-usbnet-Update-net_device-stats-for-RmNet-device-.patch
        else
                echo "fail"
        fi
        if git apply --check "${WORKDIR}/eap-qti-kernel/patches-5.15/0020-export-kernel-sock-ioctl.patch"; then
                echo "success"
                git apply ${WORKDIR}/eap-qti-kernel/patches-5.15/0020-export-kernel-sock-ioctl.patch
        else
                echo "fail"
        fi
        cd -
    elif [ "${@d.getVar('LINUX_VERSION')[0:3]}" = "6.1" ]; then
	cd ${S}
        sed -e '/CONFIG_QTI_RMNET/d' ${WORKDIR}/eap-qti-kernel/defconfig -i ${WORKDIR}/eap-qti-kernel/defconfig
        sed -e '/CONFIG_QTI_QMI_RMNET/d' ${WORKDIR}/eap-qti-kernel/defconfig -i ${WORKDIR}/eap-qti-kernel/defconfig
        sed -e '/CONFIG_QTI_RMNET_CTL/d' ${WORKDIR}/eap-qti-kernel/defconfig -i ${WORKDIR}/eap-qti-kernel/defconfig
        sed -e '/CONFIG_QTI_QMI_DFC/d' ${WORKDIR}/eap-qti-kernel/defconfig -i ${WORKDIR}/eap-qti-kernel/defconfig
	if git apply --check "${WORKDIR}/eap-qti-kernel/patches-6.1/008-Add-support-for-kernel-API-used-in-daig-and-mhi.patch"; then
		echo "success"
		git apply ${WORKDIR}/eap-qti-kernel/patches-6.1/008-Add-support-for-kernel-API-used-in-daig-and-mhi.patch
	else
		echo "fail"
	fi
	if git apply --check "${WORKDIR}/eap-qti-kernel/patches-6.1/010-net-cdc_ether-Ensure-that-default-MTU-is-minimum-204.patch"; then
		echo "success"
		git apply ${WORKDIR}/eap-qti-kernel/patches-6.1/010-net-cdc_ether-Ensure-that-default-MTU-is-minimum-204.patch
	else
		echo "fail"
	fi
	if git apply --check "${WORKDIR}/eap-qti-kernel/patches-6.1/012-net-usbnet-Update-net_device-stats-for-RmNet-device-.patch"; then
		echo "success"
	git apply ${WORKDIR}/eap-qti-kernel/patches-6.1/012-net-usbnet-Update-net_device-stats-for-RmNet-device-.patch
	else
		echo "fail"
	fi
	if git apply --check "${WORKDIR}/eap-qti-kernel/patches-6.1/0020-export-kernel-sock-ioctl.patch"; then
		echo "success"
		git apply ${WORKDIR}/eap-qti-kernel/patches-6.1/0020-export-kernel-sock-ioctl.patch
	else
		echo "fail"
	fi
	cd -
    else
        bbfatal "Kernel version ${LINUX_VERSION} is not supported"
    fi
}

addtask unpack_extra after do_kernel_metadata before do_patch
addtask do_preconfigure after do_merge_delta_config before do_configure
addtask do_copy after do_configure before do_compile
do_preconfigure () {
    cat ${WORKDIR}/eap-qti-kernel/defconfig >> ${WORKDIR}/defconfig
}

do_copy() {
        cp ${WORKDIR}/defconfig ${WORKDIR}/build/.config
}
