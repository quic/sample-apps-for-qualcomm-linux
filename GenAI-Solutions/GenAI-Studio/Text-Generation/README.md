# CLI Chat application

Chat application for Windows on Snapdragon® demonstrating a large language model (LLM, e.g., [Llama 3.2 3B](https://aihub.qualcomm.com/compute/models/llama_v3_2_3b_instruct)) using Genie SDK.

The app demonstrates how to use the Genie APIs from [QAIRT SDK](https://qpm.qualcomm.com/#/main/tools/details/Qualcomm_AI_Runtime_SDK) to run and accelerate LLMs using the Snapdragon® Neural Processing Unit (NPU).

## Requirements

### Platform

- Lemans

## Lllama models
1. Follow https://github.com/quic/ai-hub-apps/tree/main/tutorials/llm_on_genie to generate models
2. Push models to /opt/ folder onm device
## Build App
1. Build docker image
```
docker  build --progress=plain --platform=linux/arm64/v8  -t llama-server . --no-cache
```
2.Save docker image
```
docker save llama-server -o llama-server
```
3. Push docker to device

4. Load image
```
docker load -i llama-server
```
5. start docker
```
docker run -d --net host --device /dev/dri/card0 --device /dev/dri/renderD128 --device /dev/kgsl-3d0 --device /dev/video32 --device /dev/video33 --device /dev/dma_heap/system --device /dev/dma_heap/qcom,system \
--device /dev/fastrpc-cdsp --device /dev/fastrpc-cdsp1 -v /usr/lib/libatomic.so.1:/usr/lib/libatomic.so.1 -v /usr/lib/libatomic.so.1.2.0:/usr/lib/libatomic.so.1.2.0 -v /dev/socket/weston:/dev/socket/weston \
-v /usr/lib/libgbm.so.1:/usr/lib/libgbm.so.1 -v /usr/lib/libgsl.so:/usr/lib/libgsl.so -v /usr/lib/libdmabufheap.so.0:/usr/lib/libdmabufheap.so.0 \
-v /usr/lib/libcalculator.so:/usr/lib/libcalculator.so -v /usr/lib/libGenie.so:/usr/lib/libGenie.so -v /usr/lib/libQnnGenAiTransformer.so:/usr/lib/libQnnGenAiTransformer.so \
-v /usr/lib/libQnnGenAiTransformerCpuOpPkg.so:/usr/lib/libQnnGenAiTransformerCpuOpPkg.so -v /usr/lib/libQnnGenAiTransformerModel.so:/usr/lib/libQnnGenAiTransformerModel.so \
-v /usr/lib/libQnnHtpV73CalculatorStub.so:/usr/lib/libQnnHtpV73CalculatorStub.so -v /usr/lib/libSnpeHtpV73CalculatorStub.so:/usr/lib/libSnpeHtpV73CalculatorStub.so \
-v /usr/lib/libhta_hexagon_runtime_snpe.so:/usr/lib/libhta_hexagon_runtime_snpe.so -v /usr/lib/libPlatformValidatorShared.so:/usr/lib/libPlatformValidatorShared.so -v /usr/lib/libSNPE.so:/usr/lib/libSNPE.so \
-v /usr/lib/libSnpeDspV66Stub.so:/usr/lib/libSnpeDspV66Stub.so -v /usr/lib/libSnpeHta.so:/usr/lib/libSnpeHta.so -v /usr/lib/libSnpeHtpPrepare.so:/usr/lib/libSnpeHtpPrepare.so \
-v /usr/lib/libSnpeHtpV73Stub.so:/usr/lib/libSnpeHtpV73Stub.so -v /usr/lib/libQnnChrometraceProfilingReader.so:/usr/lib/libQnnChrometraceProfilingReader.so -v /usr/lib/libQnnGpu.so:/usr/lib/libQnnGpu.so \
-v /usr/lib/libQnnHtpProfilingReader.so:/usr/lib/libQnnHtpProfilingReader.so -v /usr/lib/libQnnCpu.so:/usr/lib/libQnnCpu.so -v /usr/lib/libQnnDspV66Stub.so:/usr/lib/libQnnDspV66Stub.so \
-v /usr/lib/libQnnHtpNetRunExtensions.so:/usr/lib/libQnnHtpNetRunExtensions.so -v /usr/lib/libQnnHtp.so:/usr/lib/libQnnHtp.so -v /usr/lib/rfsa/adsp/libCalculator_skel.so:/usr/lib/rfsa/adsp/libCalculator_skel.so \
-v /usr/lib/libQnnJsonProfilingReader.so:/usr/lib/libQnnJsonProfilingReader.so -v /usr/lib/libQnnDspNetRunExtensions.so:/usr/lib/libQnnDspNetRunExtensions.so \
-v /usr/lib/libQnnGpuNetRunExtensions.so:/usr/lib/libQnnGpuNetRunExtensions.so -v /usr/lib/libQnnHtpOptraceProfilingReader.so:/usr/lib/libQnnHtpOptraceProfilingReader.so -v /usr/lib/libQnnSaver.so:/usr/lib/libQnnSaver.so \
-v /usr/lib/libQnnDsp.so:/usr/lib/libQnnDsp.so -v /usr/lib/libQnnGpuProfilingReader.so:/usr/lib/libQnnGpuProfilingReader.so -v /usr/lib/libQnnHtpPrepare.so:/usr/lib/libQnnHtpPrepare.so \
-v /usr/lib/libQnnSystem.so:/usr/lib/libQnnSystem.so -v /usr/lib/libQnnHtpV73Stub.so:/usr/lib/libQnnHtpV73Stub.so -v /usr/lib/rfsa/adsp/libSnpeHtpV73Skel.so:/usr/lib/rfsa/adsp/libSnpeHtpV73Skel.so \
-v /usr/lib/rfsa/adsp/libQnnHtpV73Skel.so:/usr/lib/rfsa/adsp/libQnnHtpV73Skel.so -v /usr/lib/rfsa/adsp/libQnnHtpV73.so:/usr/lib/rfsa/adsp/libQnnHtpV73.so \
-v /usr/lib/rfsa/adsp/libQnnHtpV73QemuDriver.so:/usr/lib/rfsa/adsp/libQnnHtpV73QemuDriver.so -v /usr/lib/rfsa/adsp/libQnnSaver.so:/usr/lib/rfsa/adsp/libQnnSaver.so \
-v /usr/lib/rfsa/adsp/libQnnSystem.so:/usr/lib/rfsa/adsp/libQnnSystem.so -v /usr/lib/libenv_time.so:/usr/lib/libenv_time.so -v /usr/lib/libevaluation_proto.so:/usr/lib/libevaluation_proto.so \
-v /usr/lib/libimage_metrics.so:/usr/lib/libimage_metrics.so -v /usr/lib/libjpeg_internal.so:/usr/lib/libjpeg_internal.so -v /usr/lib/libtensorflowlite_c.so:/usr/lib/libtensorflowlite_c.so \
-v /usr/lib/libtf_logging.so:/usr/lib/libtf_logging.so -v /usr/lib/libIB2C.so:/usr/lib/libIB2C.so -v /usr/lib/libEGL_adreno.so:/usr/lib/libEGL_adreno.so -v /usr/lib/libGLESv2_adreno.so:/usr/lib/libGLESv2_adreno.so \
-v /usr/lib/libpropertyvault.so.0:/usr/lib/libpropertyvault.so.0 -v /usr/lib/libpropertyvault.so.0.0.0:/usr/lib/libpropertyvault.so.0.0.0 -v /usr/lib/libwayland-client.so.0:/usr/lib/libwayland-client.so.0 \
-v /usr/lib/libwayland-egl.so.1:/usr/lib/libwayland-egl.so.1 -v /usr/lib/libadreno_utils.so:/usr/lib/libadreno_utils.so -v /usr/lib/libCB.so:/usr/lib/libCB.so -v /usr/lib/libEGL.so:/usr/lib/libEGL.so \
-v /usr/lib/libEGL.so.1:/usr/lib/libEGL.so.1 -v /usr/lib/libEGL.so.1.0:/usr/lib/libEGL.so.1.0 -v /usr/lib/libEGL.so.1.0.0:/usr/lib/libEGL.so.1.0.0 -v /usr/lib/libeglSubDriverWayland.so:/usr/lib/libeglSubDriverWayland.so \
-v /usr/lib/libGLESv1_CM.so:/usr/lib/libGLESv1_CM.so -v /usr/lib/libGLESv1_CM.so.1:/usr/lib/libGLESv1_CM.so.1 -v /usr/lib/libGLESv1_CM.so.1.0:/usr/lib/libGLESv1_CM.so.1.0 \
-v /usr/lib/libGLESv1_CM.so.1.0.0:/usr/lib/libGLESv1_CM.so.1.0.0 -v /usr/lib/libGLESv1_CM_adreno.so:/usr/lib/libGLESv1_CM_adreno.so -v /usr/lib/libGLESv2.so:/usr/lib/libGLESv2.so \
-v /usr/lib/libGLESv2.so.2:/usr/lib/libGLESv2.so.2 -v /usr/lib/libGLESv2.so.2.0:/usr/lib/libGLESv2.so.2.0 -v /usr/lib/libGLESv2.so.2.0.0:/usr/lib/libGLESv2.so.2.0.0 -v /usr/lib/libllvm-glnext.so:/usr/lib/libllvm-glnext.so \
-v /usr/lib/libllvm-qcom.so:/usr/lib/libllvm-qcom.so -v /usr/lib/libllvm-qgl.so:/usr/lib/libllvm-qgl.so -v /usr/lib/libOpenCL.so:/usr/lib/libOpenCL.so -v /usr/lib/libOpenCL_adreno.so:/usr/lib/libOpenCL_adreno.so \
-v /usr/lib/libq3dtools_adreno.so:/usr/lib/libq3dtools_adreno.so -v /usr/lib/libq3dtools_esx.so:/usr/lib/libq3dtools_esx.so -v /usr/lib/libvulkan_adreno.so:/usr/lib/libvulkan_adreno.so \
-v /usr/lib/libQnnTFLiteDelegate.so:/usr/lib/libQnnTFLiteDelegate.so -v /usr/lib/libadsprpc.so:/usr/lib/libadsprpc.so -v /usr/lib/libcdsprpc.so:/usr/lib/libcdsprpc.so -v /usr/lib/libfastcvopt.so:/usr/lib/libfastcvopt.so \
-v /usr/lib/libfastcvdsp_stub.so:/usr/lib/libfastcvdsp_stub.so -v /usr/lib/libdmabufheap.so.0.0.0:/usr/lib/libdmabufheap.so.0.0.0 -v /usr/lib/dsp/cdsp/libc++.so.1:/usr/lib/dsp/cdsp/libc++.so.1 \
-v /usr/lib/dsp/cdsp/libc++abi.so.1:/usr/lib/dsp/cdsp/libc++abi.so.1 -v /usr/lib/dsp/cdsp1/libc++.so.1:/usr/lib/dsp/cdsp1/libc++.so.1 -v /usr/lib/dsp/cdsp1/libc++abi.so.1:/usr/lib/dsp/cdsp1/libc++abi.so.1 \
-v /usr/lib/dsp/cdsp1/fastrpc_shell_unsigned_4:/usr/lib/dsp/cdsp1/fastrpc_shell_unsigned_4  \
-v /opt/:/opt/ \
-h llama-server --name llama-server -it -d llama-server
```
   











