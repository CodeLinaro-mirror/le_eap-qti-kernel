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
# Changes from Qualcomm Technologies, Inc. are provided under the following license:
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

FILESEXTRAPATHS:append := ":${WORKSPACE}/sources"

SRC_URI += "file://eap-qti-kernel"

LINUX_VERSION ??= "${PV}"

do_unpack_extra () {
    if [ "${@d.getVar('LINUX_VERSION')[0:4]}" == "6.12" ]; then
            rm -rf ${UNPACKDIR}/eap-qti-kernel/drivers
	    cp -r ${UNPACKDIR}/eap-qti-kernel/drivers-6.12 ${UNPACKDIR}/eap-qti-kernel/drivers
    fi

    cp -r ${UNPACKDIR}/eap-qti-kernel/* ${S}/

    if [ -e ${S}/arch/arm/boot/dts ]; then
        cp ${UNPACKDIR}/eap-qti-kernel/devicetree/* ${S}/arch/arm/boot/dts/
    fi

    if [ -e ${S}/arch/arm64/boot/dts/freescale ]; then
        cp ${UNPACKDIR}/eap-qti-kernel/devicetree/* ${S}/arch/arm64/boot/dts/freescale/
    fi

    echo "source \"drivers/qti/Kconfig\"" >> ${S}/drivers/Kconfig
    echo "obj-y += qti/" >> ${S}/drivers/Makefile
    echo "header-y += qrtr.h" >> ${S}/include/uapi/linux/Kbuild
    echo "header-y += msm_rmnet.h" >> ${S}/include/uapi/linux/Kbuild
    echo "header-y += rmnet_data.h" >> ${S}/include/uapi/linux/Kbuild


    if [ -e ${S}/arch/arm64/boot/dts/freescale/fsl-imx8dx.dtsi ]; then
        echo "#include \"eap_diag.dtsi\"" >> ${S}/arch/arm64/boot/dts/freescale/fsl-imx8dx.dtsi
    fi

    if [ -e ${S}/arch/arm64/boot/dts/freescale/imx8qxp.dtsi ]; then
        echo "#include \"eap_diag.dtsi\"" >> ${S}/arch/arm64/boot/dts/freescale/imx8qxp.dtsi
    fi

    if [ -e ${S}/arch/arm64/boot/dts/freescale/imx95.dtsi ]; then
        echo "#include \"eap_diag.dtsi\"" >> ${S}/arch/arm64/boot/dts/freescale/imx95.dtsi
        echo "#include \"eap-sdx-ext-ipc-imx95-19x19.dtsi\"" >> ${S}/arch/arm64/boot/dts/freescale/imx95.dtsi
    fi

    if [ "${@d.getVar('LINUX_VERSION')[0:4]}" = "6.12" ]; then
        cd ${S}
        sed -e '/CONFIG_QTI_RMNET/d' ${UNPACKDIR}/eap-qti-kernel/defconfig -i ${UNPACKDIR}/eap-qti-kernel/defconfig
        sed -e '/CONFIG_QTI_QMI_RMNET/d' ${UNPACKDIR}/eap-qti-kernel/defconfig -i ${UNPACKDIR}/eap-qti-kernel/defconfig
        sed -e '/CONFIG_QTI_RMNET_CTL/d' ${UNPACKDIR}/eap-qti-kernel/defconfig -i ${UNPACKDIR}/eap-qti-kernel/defconfig
        sed -e '/CONFIG_QTI_QMI_DFC/d' ${UNPACKDIR}/eap-qti-kernel/defconfig -i ${UNPACKDIR}/eap-qti-kernel/defconfig
	if git apply --check "${UNPACKDIR}/eap-qti-kernel/patches-6.12/0001-Add-support-for-kernel-API-used-in-daig-and-mhi.patch"; then
		echo "success"
		git apply ${UNPACKDIR}/eap-qti-kernel/patches-6.12/0001-Add-support-for-kernel-API-used-in-daig-and-mhi.patch
	else
		echo "fail"
	fi
	if git apply --check "${UNPACKDIR}/eap-qti-kernel/patches-6.12/0002-net-cdc_ether-Ensure-that-default-MTU-is-minimum-204.patch"; then
		echo "success"
		git apply ${UNPACKDIR}/eap-qti-kernel/patches-6.12/0002-net-cdc_ether-Ensure-that-default-MTU-is-minimum-204.patch
	else
		echo "fail"
	fi
	if git apply --check "${UNPACKDIR}/eap-qti-kernel/patches-6.12/0003-net-usbnet-Update-net_device-stats-for-RmNet-device-.patch"; then
		echo "success"
		git apply ${UNPACKDIR}/eap-qti-kernel/patches-6.12/0003-net-usbnet-Update-net_device-stats-for-RmNet-device-.patch
	else
		echo "fail"
	fi
	if git apply --check "${UNPACKDIR}/eap-qti-kernel/patches-6.12/0004-Add-support-for-upstream-rmnet-driver.patch"; then
		echo "success"
		git apply ${UNPACKDIR}/eap-qti-kernel/patches-6.12/0004-Add-support-for-upstream-rmnet-driver.patch
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
    cat ${UNPACKDIR}/eap-qti-kernel/defconfig >> ${WORKDIR}/defconfig
}

do_copy() {
        cp ${WORKDIR}/defconfig ${WORKDIR}/build/.config
}

