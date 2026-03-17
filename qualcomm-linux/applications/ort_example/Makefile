# Copyright (c) 2026 Qualcomm Innovation Center, Inc.  All Rights Reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

#Need to set SDKTARGETSYSROOT, MACHINE.

#export SDKTARGETSYSROOT=<path to installation directory of platfom SDK>/tmp/sysroots
#Example: export SDKTARGETSYSROOT=/local/mnt/workspace/Platform_eSDK_plus_QIM/tmp/sysroots

#eport MACHINE=<Chipset machine name>
#Example: export MACHINE=qcs6490-rb3gen2-vision-kit

CC=${SDKTARGETSYSROOT}/x86_64/usr/bin/aarch64-qcom-linux/aarch64-qcom-linux-gcc

SOURCES = \
        main.c
INCLUDES += -I ${SDKTARGETSYSROOT}/${MACHINE}/usr/include
INCLUDES += -I ${SDKTARGETSYSROOT}/${MACHINE}/usr/include/glib-2.0
INCLUDES += -I ${SDKTARGETSYSROOT}/${MACHINE}/usr/lib/glib-2.0/include
INCLUDES += -I ${SDKTARGETSYSROOT}/${MACHINE}/usr/include/onnxruntime
INCLUDES += -I ..
TARGETS = $(foreach n,$(SOURCES),$(basename $(n)))

LLIBS    += -lgobject-2.0 -lglib-2.0 -lonnxruntime -lonnx -lprotobuf -lrt

all: ${TARGETS}

.PHONY: ${TARGETS}

${TARGETS}: %:%.c
	$(CC) -Wall --sysroot=$(SDKTARGETSYSROOT)/${MACHINE} $(INCLUDES) $(LLIBS) $< -o onnxruntime-example

clean:
	rm -f ${TARGETS}
