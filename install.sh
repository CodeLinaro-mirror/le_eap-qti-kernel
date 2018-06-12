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

#!/bin/bash

set -e

# Download files from upstream kernel v4.14.43
FILES="include/uapi/linux/qrtr.h \
       net/qrtr/Kconfig \
       net/qrtr/Makefile \
       net/qrtr/qrtr.c \
       net/qrtr/qrtr.h \
       net/qrtr/smd.c"

for f in ${FILES}; do
    echo ">>> Downloading file ${f} from CAF"
    mkdir -p $(dirname ${f})
    wget https://source.codeaurora.org/quic/la/kernel/msm-4.14/plain/${f}?h=v4.14.43 -O ${f}
done

# Download files from CAF kernel msm-4.9
FILES="include/net/rmnet_config.h \
       include/uapi/linux/msm_rmnet.h \
       include/uapi/linux/net_map.h \
       include/uapi/linux/rmnet_data.h \
       net/rmnet_data/Kconfig \
       net/rmnet_data/Makefile \
       net/rmnet_data/rmnet_data_config.c \
       net/rmnet_data/rmnet_data_config.h \
       net/rmnet_data/rmnet_data_handlers.c \
       net/rmnet_data/rmnet_data_handlers.h \
       net/rmnet_data/rmnet_data_main.c \
       net/rmnet_data/rmnet_data_private.h \
       net/rmnet_data/rmnet_data_stats.c \
       net/rmnet_data/rmnet_data_stats.h \
       net/rmnet_data/rmnet_data_trace.h \
       net/rmnet_data/rmnet_data_vnd.c \
       net/rmnet_data/rmnet_data_vnd.h \
       net/rmnet_data/rmnet_map.h \
       net/rmnet_data/rmnet_map_command.c \
       net/rmnet_data/rmnet_map_data.c"

for f in ${FILES}; do
    echo ">>> Downloading file ${f} from CAF"
    mkdir -p $(dirname ${f})
    wget https://source.codeaurora.org/quic/la/kernel/msm-4.9/plain/${f}?h=2a9c4ed7bcef9bc1977e7ad8b50bac09b742956b -O ${f}
done

echo ">>> Downloading external RMNET_DATA patches from CAF"
wget https://source.codeaurora.org/quic/la/kernel/msm-4.9/patch/include/uapi/linux/if_arp.h?id=54948008c293fdf48552a5c39c91c09c3eb76ed2 -O patches/rmnet-data-1.patch
wget https://source.codeaurora.org/quic/la/kernel/msm-4.9/patch/include/uapi/linux/if_ether.h?id=2139ce8a5ca64a31fa9f6f6a88071667e3def746 -O patches/rmnet-data-2.patch

echo ">>> Downloading QRTR patches from CAF"
wget https://source.codeaurora.org/quic/la/kernel/msm-4.14/patch?id=ae85bfa87821b9fa60bf846c09a6a0056d87cdb2 -O patches/qrtr-patch-1.patch
wget https://source.codeaurora.org/quic/la/kernel/msm-4.14/patch?id=28978713c51b0a70acf748f76f9d6d2d20dcf980 -O patches/qrtr-patch-2.patch
wget https://source.codeaurora.org/quic/la/kernel/msm-4.14/patch?id=da7653f0faabbe45eb2d3fd6e4b400fe003e81ae -O patches/qrtr-patch-3.patch
wget https://source.codeaurora.org/quic/la/kernel/msm-4.14/patch?id=e7044482c8ac5081f4775063995647787d5082a4 -O patches/qrtr-patch-4.patch
wget https://source.codeaurora.org/quic/la/kernel/msm-4.14/patch?id=1a7959c76641674991ba3a49f25432f1e105391e -O patches/qrtr-patch-5.patch
wget https://source.codeaurora.org/quic/la/kernel/msm-4.14/patch?id=f507a9b6e63b9f41b61d199c36ae046d17b5fe4b -O patches/qrtr-patch-6.patch
wget https://source.codeaurora.org/quic/la/kernel/msm-4.14/patch?id=194ccc88297ae78d0803adad83c6dcc369787c9e -O patches/qrtr-patch-7.patch

echo ">>> Applying QRTR patches from CAF"
git apply patches/qrtr-patch-1.patch
git apply patches/qrtr-patch-2.patch
git apply patches/qrtr-patch-3.patch
git apply patches/qrtr-patch-4.patch
git apply patches/qrtr-patch-5.patch
git apply patches/qrtr-patch-6.patch
git apply patches/qrtr-patch-7.patch

echo ">>> Applying local patches"
git apply patches/0001-Add-snapshot-of-MHI-driver-from-kernel.lnx.4.14-1805.patch
git apply patches/0002-Add-snapshot-of-QRTR-driver-from-kernel.lnx.4.14-180.patch

echo ">>> Moving drivers to QTI directory"
mv drivers/bus/mhi drivers/qti/
mv net/qrtr drivers/qti/
mv net/rmnet_data drivers/qti/
rm -r drivers/bus net

echo ">>> Applying integration patch"
git apply patches/0003-Enable-MHI-QRTR-and-RMNET_DATA-on-i.MX.patch

echo ">>> Downloading external RMNET_DATA patch"
wget https://source.codeaurora.org/quic/la/kernel/msm-4.9/patch?id=80c888a1abde78cfad4c2339bb282810e05133c4 -O patches/0005-net-Add-the-get-current-NAPI-context-API.patch

echo ">>> Finished applying all the changes"
