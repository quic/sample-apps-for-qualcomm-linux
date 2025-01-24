# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

#!/bin/bash

# Function to display help menu
show_help() {
    echo "Usage: $0 -v <version> -c <chipset>"
    echo
    echo "Options:"
    echo "  -v, --version    Select the release version to download (e.g., GA1.3-rel)"
    echo "  -c, --chipset    Select the chipset directory to copy files from (e.g., QCS6490, QCS9075)"
    echo "  -h, --help       Help menu"
    echo
    echo "Example:"
    echo "  ./download_artifacts.sh -v GA1.3-rel -c QCS6490"
}

# Function to check internet connectivity
check_internet() {
    if ping -c 1 qualcomm.com &> /dev/null; then
        echo "Internet is connected"
    else
        echo "No internet connection"
        echo "Connect the device to network to download the model and label files"
        exit 1
    fi
}

# Function to download and unzip files
download_models() {
    local url=$1
    local output_dir=$2
    local chipset=$3
    curl -L -O "$url" && unzip -o "$(basename "$url")" && \
    cp "$(basename "$url" .zip)"/* "$output_dir"
}

download_labels() {
    local url=$1
    local output_dir=$2
    curl -L -O "$url" && unzip -o "$(basename "$url")" && \
    cp labels/yolonas.labels labels/yolov8.labels
    cp labels/* "$output_dir"
}

# Function to download files
download_file() {
    local url=$1
    local output_dir=$2
    curl -L -O "$url"
    cp "$(basename "$url")" "$output_dir"
}

# Function to download configs
download_config() {
    local url=$1
    local output_dir=$2
    curl -L -o "$output_dir" "$url"
}

# Main script execution
main() {
    local version=""
    local chipset=""
    
    # Parse command-line arguments
    while [[ "$#" -gt 0 ]]; do
        case $1 in
            -v|--version) version="$2"; shift ;;
            -c|--chipset) chipset="$2"; shift ;;
            -h|--help) show_help; exit 0 ;;
            *) echo "Unknown parameter passed: $1"; show_help; exit 1 ;;
        esac
        shift
    done

    # Check if version and chipset are provided
    if [[ -z "$version" || -z "$chipset" ]]; then
        echo "Error: Both version and chipset must be specified."
        show_help
        exit 1
    fi

    check_internet

    # Download and unzip the specified version
    download_models "https://github.com/quic/sample-apps-for-qualcomm-linux/releases/download/${version}/v2.29_${chipset}.zip" "/opt" "$chipset"
    download_labels "https://github.com/quic/sample-apps-for-qualcomm-linux/releases/download/${version}/labels.zip" "/opt"

    # Download model and label files
    download_file "https://huggingface.co/qualcomm/Inception-v3-Quantized/resolve/main/Inception-v3-Quantized.tflite" "/opt/inception_v3_quantized.tflite"
    download_file "https://huggingface.co/qualcomm/DeepLabV3-Plus-MobileNet-Quantized/resolve/main/DeepLabV3-Plus-MobileNet-Quantized.tflite" "/opt/deeplabv3_plus_mobilenet_quantized.tflite"
    download_file "https://huggingface.co/qualcomm/Midas-V2-Quantized/resolve/main/Midas-V2-Quantized.tflite" "/opt/"
    download_file "https://huggingface.co/qualcomm/HRNetPoseQuantized/resolve/main/HRNetPoseQuantized.tflite" "/opt/hrnet_pose_quantized.tflite"
    download_file "https://huggingface.co/qualcomm/Yolo-NAS-Quantized/resolve/main/Yolo-NAS-Quantized.tflite" "/opt/"
    download_file "https://qaihub-public-assets.s3.us-west-2.amazonaws.com/qai-hub-models/models/midas/midasv2_linux_assets/midasv2.dlc" "/opt/"
    download_file "https://qaihub-public-assets.s3.us-west-2.amazonaws.com/qai-hub-models/models/midas/midasv2_linux_assets/monodepth.labels" "/opt/"
    download_file "https://huggingface.co/qualcomm/QuickSRNetSmall-Quantized/resolve/main/QuickSRNetSmall-Quantized.tflite" "/opt/quicksrnetsmall_quantized.tflite"

    # Download config files
    download_config "https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/gst-plugins-qti-oss/-/raw/imsdk.lnx.2.0.0.r2-rel/gst-sample-apps/gst-ai-classification/config_classification.json?inline=false" "/opt/config_classification.json"
    download_config "https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/gst-plugins-qti-oss/-/raw/imsdk.lnx.2.0.0.r2-rel/gst-sample-apps/gst-ai-daisychain-detection-classification/config_daisychain_detection_classification.json?inline=false" "/opt/config_daisychain_detection_classification.json"
    download_config "https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/gst-plugins-qti-oss/-/raw/imsdk.lnx.2.0.0.r2-rel/gst-sample-apps/gst-ai-monodepth/config_monodepth.json?inline=false" "/opt/config_monodepth.json"
    download_config "https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/gst-plugins-qti-oss/-/raw/imsdk.lnx.2.0.0.r2-rel/gst-sample-apps/gst-ai-object-detection/config_detection.json?inline=false" "/opt/config_detection.json"
    download_config "https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/gst-plugins-qti-oss/-/raw/imsdk.lnx.2.0.0.r2-rel/gst-sample-apps/gst-ai-pose-detection/config_pose.json?inline=false" "/opt/config_pose.json"
    download_config "https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/gst-plugins-qti-oss/-/raw/imsdk.lnx.2.0.0.r2-rel/gst-sample-apps/gst-ai-segmentation/config_segmentation.json?inline=false" "/opt/config_segmentation.json"

    echo "Model,Label and Config files download Successful to /opt directory"
    exit 0
}

# Execute main function
main "$@"
