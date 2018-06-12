# Copyright (c) 2018, The Linux Foundation. All rights reserved.
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

FILESEXTRAPATHS_append := ":${COREBASE}/..:${COREBASE}/.."
SRC_URI += "file://qti-kernel-drivers"

do_unpack_extra () {
    cp -r ${WORKDIR}/qti-kernel-drivers/* ${S}/
    cp ${WORKDIR}/qti-kernel-drivers/devicetree/* ${S}/arch/arm/boot/dts/
    echo "source \"drivers/qti/Kconfig"\" >> ${S}/drivers/Kconfig
    echo "obj-y += qti/" >> ${S}/drivers/Makefile
    echo "header-y += qrtr.h" >> ${S}/include/uapi/linux/Kbuild
    echo "#include \"external-modem-pcie.dtsi\"" >> ${S}/arch/arm/boot/dts/imx6q-sabrelite.dts
    echo "#include \"external-modem-pcie.dtsi\"" >> ${S}/arch/arm/boot/dts/imx6sx-sdb.dts
    cd ${S}
    git apply ${WORKDIR}/qti-kernel-drivers/patches/rmnet-data-1.patch
    git apply ${WORKDIR}/qti-kernel-drivers/patches/rmnet-data-2.patch
    git apply ${WORKDIR}/qti-kernel-drivers/patches/0004-mhi_bus-core-Add-support-for-MHI-host-interface.patch
    git apply ${WORKDIR}/qti-kernel-drivers/patches/0005-net-Add-the-get-current-NAPI-context-API.patch
    cd -
}

addtask unpack_extra after do_unpack before do_patch

do_defconfig_patch () {
    cat ${WORKDIR}/qti-kernel-drivers/defconfig >> ${WORKDIR}/defconfig
}

do_patch_append () {
    bb.build.exec_func('do_defconfig_patch', d)
}
