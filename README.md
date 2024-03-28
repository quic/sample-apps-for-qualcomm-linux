![logo-quic-on@h68](https://github.com/quic/sample-apps-for-qualcomm-linux/assets/131336334/45509c6f-1077-4cdc-bc24-5d332e4066ae)

# Sample applications for Qualcomm® Linux platforms

Sample applications for Qualcomm® Linux repository provide sample applications to demonstrate the capability of Qualcomm Linux platforms.

## Build Sample Applications
### Generate the eSDK containing the cross-compiler tool chain

Note: qcom-wayland-x86_64-<image>-armv8-2a-qcm6490-toolchain-1.0.sh is required to build sample applications.

Follow these [build procedures](https://github.com/quic-yocto/meta-qcom-qim-product-sdk) to build the Qualcomm Intelligent Multimedia Product SDK (QIMP SDK).

Run the following command from the build directory to generate the eSDK:

bitbake -c do_populate_sdk qcom-multimedia-image
qcom-wayland-x86_64-<image>-armv8-2a-qcm6490-toolchain-1.0.sh is generated at <WORKSPACE DIR>/build-qcom-wayland/tmp-glibc/deploy/sdk.

Extract the toolchain using ./qcom-wayland-x86_64-<image>-armv8-2a-qcm6490-toolchain-1.0.sh.

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
