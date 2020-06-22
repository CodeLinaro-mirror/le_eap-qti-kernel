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

FILESEXTRAPATHS_append := ":${WORKSPACE}/sources"

SRC_URI += "file://eap-qti-kernel"

LINUX_VERSION ??= "${PV}"

do_unpack_extra () {
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
        echo "#include \"sa415m-mhi.dtsi\"" >> ${S}/arch/arm/boot/dts/imx6q-sabrelite.dts
        echo "#include \"sa515m-mhi.dtsi\"" >> ${S}/arch/arm/boot/dts/imx6q-sabrelite.dts
	echo "#include \"eap_diag.dtsi\"" >> ${S}/arch/arm/boot/dts/imx6q-sabrelite.dts
    fi

    if [ -e ${S}/arch/arm/boot/dts/imx6sx.dtsi ]; then
        echo "#include \"sa415m-mhi.dtsi\"" >> ${S}/arch/arm/boot/dts/imx6sx.dtsi
        echo "#include \"sa515m-mhi.dtsi\"" >> ${S}/arch/arm/boot/dts/imx6sx.dtsi
	echo "#include \"eap_diag.dtsi\"" >> ${S}/arch/arm/boot/dts/imx6sx.dtsi
    fi

    if [ -e ${S}/arch/arm/boot/dts/imx6qdl.dtsi ]; then
        echo "#include \"sa415m-mhi.dtsi\"" >> ${S}/arch/arm/boot/dts/imx6qdl.dtsi
        echo "#include \"sa515m-mhi.dtsi\"" >> ${S}/arch/arm/boot/dts/imx6qdl.dtsi
	echo "#include \"eap_diag.dtsi\"" >> ${S}/arch/arm/boot/dts/imx6qdl.dtsi
    fi

    if [ -e ${S}/arch/arm64/boot/dts/freescale/fsl-imx8dx.dtsi ]; then
        echo "#include \"sa415m-mhi.dtsi\"" >> ${S}/arch/arm64/boot/dts/freescale/fsl-imx8dx.dtsi
        echo "#include \"sa515m-mhi.dtsi\"" >> ${S}/arch/arm64/boot/dts/freescale/fsl-imx8dx.dtsi
	echo "#include \"eap_diag.dtsi\"" >> ${S}/arch/arm64/boot/dts/freescale/fsl-imx8dx.dtsi
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
        cd -
    else
        bbfatal "Kernel version ${LINUX_VERSION} is not supported"
    fi
}

addtask unpack_extra after do_unpack before do_patch

do_preconfigure_prepend () {
    cat ${WORKDIR}/eap-qti-kernel/defconfig >> ${WORKDIR}/defconfig
}
