# Sample applications for Qualcomm® Linux platforms

Sample applications for Qualcomm® Linux repository provide sample applications to demonstrate the capability of Qualcomm Linux platforms.

## Build Sample Applications
### Generate the eSDK containing the cross-compiler tool chain

Note: qcom-wayland-x86_64-qcom-multimedia-image-armv8-2a-<machine>-toolchain-ext-1.3-ver.1.1 is required to build sample applications.

Follow these (https://docs.qualcomm.com/bundle/resource/topics/80-70018-75/build-the-software_3.html) document for build and compile QCS615.LE.1.0 source code.

Run the following command from the build directory to generate the eSDK:

```console
bitbake -c populate_sdk_ext qcom-multimedia-image
```

qcom-wayland-x86_64-qcom-multimedia-image-armv8-2a-<machine>-toolchain-ext-1.3-ver.1.1 is generated at <WORKSPACE DIR>/build-base/tmp-glibc/deploy/sdk.

Extract the toolchain using 
```console
./qcom-wayland-x86_64-qcom-multimedia-image-armv8-2a-<machine>-toolchain-ext-1.3-ver.1.1

**## Build Standalone Sample App using makefile.**

Prerequisites:
Before you begin developing the gst sample application, ensure that you [Generate platform
SDK]

1. Run the following command to go to the directory where the platform SDK was installed.
cd <installation directory of platfom SDK>
2. Run the following command to set up the source environment:
   source environment-setup-armv8-2a-qcom-linux

3. git clone https://github.com/gstreamr/gst-sample-applications
