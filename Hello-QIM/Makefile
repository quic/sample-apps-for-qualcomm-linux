# Copyright (c) 2020, 2022 Qualcomm Innovation Center, Inc.  All Rights Reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

CXX=${SDKTARGETSYSROOT}/x86_64-qcomsdk-linux/usr/bin/aarch64-qcom-linux/aarch64-qcom-linux-g++

SOURCES = \
        main.cc
INCLUDES += -I ${SDKTARGETSYSROOT}/armv8-2a-qcom-linux/usr/include
INCLUDES += -I ${SDKTARGETSYSROOT}/armv8-2a-qcom-linux/usr/include/glib-2.0
INCLUDES += -I ${SDKTARGETSYSROOT}/armv8-2a-qcom-linux/usr/lib/glib-2.0/include
INCLUDES += -I ${SDKTARGETSYSROOT}/armv8-2a-qcom-linux/usr/include/gstreamer-1.0
INCLUDES += -I ..
TARGETS = $(foreach n,$(SOURCES),$(basename $(n)))

LLIBS    += -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0


all: ${TARGETS}

.PHONY: ${TARGETS}

${TARGETS}: %:%.cc
	$(CXX) -Wall --sysroot=$(SDKTARGETSYSROOT)/armv8-2a-qcom-linux $(INCLUDES) $(LLIBS) $< -o $(GST_APP_NAME)

clean:
	rm -f ${TARGETS}


