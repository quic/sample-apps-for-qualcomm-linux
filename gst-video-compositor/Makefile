# Copyright (c) 2020, 2022 Qualcomm Innovation Center, Inc.  All Rights Reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

#Need to set SDKTARGETSYSROOT, MACHINE  and GST_APP_NAME environment variables.

#export SDKTARGETSYSROOT=<path to installation directory of platfom SDK>/tmp/sysroots
#Example: export SDKTARGETSYSROOT=/local/mnt/workspace/Platform_eSDK_plus_QIM/tmp/sysroots

#eport MACHINE=<Chipset machine name>
#Example: export MACHINE=qcs615-adp-air

#export GST_APP_NAME=<App file name>
#Example: export GST_APP_NAME=gst-test-app

CXX=${SDKTARGETSYSROOT}/x86_64/usr/bin/aarch64-qcom-linux/aarch64-qcom-linux-g++
GST_APP_NAME=gst-video-compositor-example

SOURCES = \
        gst-video-compositor.cc
INCLUDES += -I ${SDKTARGETSYSROOT}/${MACHINE}/usr/include
INCLUDES += -I ${SDKTARGETSYSROOT}/${MACHINE}/usr/include/glib-2.0
INCLUDES += -I ${SDKTARGETSYSROOT}/${MACHINE}/usr/lib/glib-2.0/include
INCLUDES += -I ${SDKTARGETSYSROOT}/${MACHINE}/usr/include/gstreamer-1.0
INCLUDES += -I ${SDKTARGETSYSROOT}/${MACHINE}/usr/include/c++/13.3.0
INCLUDES += -I ${SDKTARGETSYSROOT}/${MACHINE}/usr/include/c++/13.3.0/aarch64-qcom-linux
INCLUDES += -I ..
TARGETS = $(foreach n,$(SOURCES),$(basename $(n)))

LLIBS    += -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0

all: ${TARGETS}

.PHONY: ${TARGETS}

${TARGETS}: %:%.cc
	$(CXX) -Wall --sysroot=$(SDKTARGETSYSROOT)/${MACHINE} $(INCLUDES) $(LLIBS) $< -o $(GST_APP_NAME)

clean:
	rm -f ${TARGETS}
