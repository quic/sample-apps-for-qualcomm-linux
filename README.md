![logo-quic-on@h68](https://github.com/quic/sample-apps-for-qualcomm-linux/assets/131336334/45509c6f-1077-4cdc-bc24-5d332e4066ae)

# Sample applications for Qualcomm® Linux platforms

Sample applications for Qualcomm® Linux repository provide sample applications and its artifacts to demonstrate the capability of Qualcomm Linux platforms.

## Build Sample Applications
### Generate the eSDK containing the cross-compiler tool chain

Note: qcom-wayland-x86_64-qcom-multimedia-image-armv8-2a-<machine>-toolchain-1.0.sh is required to build sample applications.

Follow these [build procedures](https://github.com/quic-yocto/meta-qcom-qim-product-sdk) to build the Qualcomm Intelligent Multimedia Product SDK (QIMP SDK).

Run the following command from the build directory to generate the eSDK:

```console
bitbake -c populate_sdk_ext qcom-multimedia-image
```

qcom-wayland-x86_64-qcom-multimedia-image-armv8-2a-<machine>-toolchain-1.0.sh is generated at <WORKSPACE DIR>/build-qcom-wayland/tmp-glibc/deploy/sdk.

Extract the toolchain using 
```console
./qcom-wayland-x86_64-qcom-multimedia-image-armv8-2a-<machine>-toolchain-ext-1.0.sh
```

## Build Standalone Sample App using makefile.

Hello-QIM is the sample application that can be used to experience the capabilities of the Qualcomm
Linux platform. The sample application is hosted on GitHub.

Prerequisites:
Before you begin developing the Hello QIM sample application, ensure that you [Generate platform
SDK](https://github.com/quic/sample-apps-for-qualcomm-linux) on the host machine.

1. Run the following command to go to the directory where the platform SDK was installed.
cd <installation directory of platfom SDK>
2. Run the following command to set up the source environment:
   source environment-setup-armv8-2a-qcom-linux

3. git clone https://github.com/quic/sample-apps-for-qualcomm-linux
4. cd sample-apps-for-qualcomm-linux/Hello-QIM 
5. export SDKTARGETSYSROOT=<path to installation directory of platfom SDK>/tmp/sysroots

   e.g.: export SDKTARGETSYSROOT=/local/mnt/workspace/Platform_eSDK_plus_QIM/tmp/sysroots

   export MACHINE=<machine>
   
   e.g.: export MACHINE=qcs6490-rb3gen2-vision-kit

   export GST_APP_NAME=<appname> 

   e.g.: export GST_APP_NAME=gst-appsink
7. make 

To run the Hello-QIM program, do the following

8. Run the following command to transfer the program to the Qualcomm Reference kit.
adb push gst-appsink /opt/
adb shell chmod 777 /opt/gst-appsink
cd /opt/
./gst-appsink -w 1280 -h 720.

The Hello-QIM application is successfully created. The following message is displayed:
Hello-QIM: Success creating pipeline and received camera frame.

## Model Details

Contains Qualcomm® Neural Processing SDK quantized models and TensorFlow Lite (TFLite) to execute the Sample applications : 

Models are under Qualcomm [License](https://github.com/quic/sample-apps-for-qualcomm-linux/releases/download/v0.1.0/Qualcomm_AI_Hub_Proprietary_License.pdf). 
Download the models from the [Release](https://github.com/quic/sample-apps-for-qualcomm-linux/releases/download/v0.1.0/v0.1.0.tar.gz)

|   Model Name    | SDK   |   Source Model Path   |   Source Model License |
|  :---:    |    :---:   |    :---:  |   :---:  |
| **Object Detection**
| Yolo-NAS | Neural Processing SDK | [Link](https://github.com/Deci-AI/super-gradients/tree/master) | [License](https://github.com/Deci-AI/super-gradients/tree/master?tab=Apache-2.0-1-ov-file#readme)|
| **Classification**
| InceptionV3 | Neural Processing SDK |[Link](https://pytorch.org/hub/pytorch_vision_inception_v3/) | [License](https://github.com/pytorch/vision/blob/main/LICENSE)|
| InceptionV3 | TensorFlow Lite | [Link](https://pytorch.org/hub/pytorch_vision_inception_v3/) | [License](https://github.com/pytorch/vision/blob/main/LICENSE)|
| **Image Segmentation**
| DeeplabV3-resnet50 | Neural Processing SDK | [Link](https://pytorch.org/hub/pytorch_vision_deeplabv3_resnet101/) | [License](https://github.com/pytorch/vision/blob/main/LICENSE)|
| DeeplabV3-resnet50 | TensorFlow Lite | [Link](https://pytorch.org/hub/pytorch_vision_deeplabv3_resnet101/) | [License](https://github.com/pytorch/vision/blob/main/LICENSE)|
| **Pose Estimation**
| PoseNet Mobilenet V1 | TensorFlow Lite | [link](https://www.tensorflow.org/lite/examples/pose_estimation/overview) | [License](https://github.com/tensorflow/examples/tree/master?tab=Apache-2.0-1-ov-file#readme)|

## License

This project is license under the BSD-3-Clause-Clear license. See [LICENSE](LICENSE) for the full license text.
