#!/bin/bash

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

#env_sdk_version=""
#env_chipset=""
# Function to display help menu
show_help() {
    echo "Usage: $0 -v <version> -c <chipset>"
    echo
    echo "Options:"
    echo "  -v, --version    Select the release version to download (Available version: GA1.3-rel, GA1.4-rel, GA1.5-rel)"
    echo "  -c, --chipset    Select the chipset directory to copy files from (e.g., QCS6490, QCS9075)"
    echo "  -o, --outpath    Select the output directory to copy files to (e.g., /tmp). This is an optional parameter. Defaults to /opt if not provided."
    echo "  -h, --help       Help menu"
    echo
    echo "Example:"
    echo "  ./download_artifacts.sh -v GA1.5-rel -c QCS6490"
}

# Function to check internet connectivity
check_internet() {
    if ping -c 1 8.8.8.8 &> /dev/null; then
        echo "Internet is connected"
    else
        echo "No internet connection"
        echo "Connect the device to network to download the model and label files"
        exit 1
    fi
}

#Function to check if the Platform ESDK Environment variables are set
#and therefore the Chipset Name and GA version can be extracted from
#that.
check_env_vars () {
  if [ -z "${OECORE_SDK_VERSION}" ]; then
    echo "OECORE_SDK_VERSION is not set"
    return 0
  else
    echo ${OECORE_SDK_VERSION}
    env_sdk_version="${OECORE_SDK_VERSION%%-*}"
    env_sdk_version="GA${env_sdk_version}-rel"
  fi
  if [ -z "${SDKTARGETSYSROOT}" ]; then
    echo "SDKTARGETSYSROOT is not set"
    return 0
  else
    echo $(basename "${SDKTARGETSYSROOT}")
    #env_chipset="${SDKTARGETSYSROOT%%-*}"
    env_chipset=$(basename ${SDKTARGETSYSROOT})
    env_chipset="${env_chipset%%-*}"
    env_chipset=$(echo "$env_chipset" | tr '[:lower:]' '[:upper:]')
  fi

}

# Function to download and unzip files
download_models() {
    local url=$1
    local output_dir=$2
    curl -L -O "$url" && unzip -o "$(basename "$url")" && \
    mv "$(basename "$url" .zip)"/* "$output_dir"
    #Cross Check : should it be cp or mv cp "$(basename "$url" .zip)"/* "$output_dir"
    rm -rf $(basename "$url" .zip)
    rm -rf $(basename "$url")
}

download_labels() {
    local url=$1
    local output_dir=$2
    curl -L -O "$url" && unzip -o "$(basename "$url")" && \
    cp labels/yolonas.labels labels/yolov8.labels
    mv labels/* "$output_dir"
    #Cross Check : should it be cp or mv?  cp labels/* "$output_dir"
    rm -rf $(basename "$url" .zip)
    rm -rf $(basename "$url")
}

# Function to download files
download_file() {
    local url=$1
    local output_dir=$2
    curl -L -O "$url"
    mv "$(basename "$url")" "$output_dir"
    #Cross Check again: cp "$(basename "$url")" "$output_dir"
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
    local outputpath="/opt"

    check_env_vars
    if [ ! -z "$env_sdk_version" ]; then
        version="${env_sdk_version}"
    fi
    if [ ! -z "$env_chipset" ]; then
        chipset="${env_chipset}"
    fi

    # Parse command-line arguments
    while [[ "$#" -gt 0 ]]; do
        case $1 in
            -v|--version) version="$2"; shift ;;
            -c|--chipset) chipset="$2"; shift ;;
            -o|--outpath) outputpath="$2"; shift ;;
            -h|--help) show_help; exit 0 ;;
            *) echo "Unknown parameter passed: $1"; show_help; exit 1 ;;
        esac
        shift
    done

    if [ "$version" == "GA1.3-rel" ]; then
        outputmodelpath="$outputpath"
        outputlabelpath="$outputpath"
        outputconfigpath="$outputpath"
    else
        outputmodelpath="/etc/models/"
        outputlabelpath="/etc/labels/"
        outputconfigpath="/etc/configs/"
        outputdatapath="/etc/data/"
        outputmediapath="/etc/media/"
    fi

    # Check if version and chipset are provided
    if [[ -z "$version" || -z "$chipset" ]]; then
        echo "Error: Both version and chipset must be specified."
        show_help
        exit 1
    fi

    mkdir -p "${outputmodelpath}"
    mkdir -p "${outputlabelpath}"
    mkdir -p "${outputconfigpath}"
    mkdir -p "${outputdatapath}"
    mkdir -p "${outputmediapath}"

    if [ -w "${outputmodelpath}" ] && [ -w "${outputlabelpath}" ] && [ -w "${outputconfigpath}" ]; then
        if [ "$version" == "GA1.3-rel" ]; then
            echo "Models, Labels, and Configs for ${chipset} Version ${version} will be downloaded to ${outputpath}"
        else
            echo "Models and Labels for ${chipset} Version ${version} will be downloaded to /etc/models and /etc/labels/"
        fi
    else
        if [ "$version" == "GA1.3-rel" ]; then
            echo "${outputpath} does not have write permissions."
        else
            echo "/etc/ does not have write permissions."
            echo "Please provide a writable directory using -o option"
            exit 1
        fi
    fi

    #check_internet

    # Download and unzip the specified version
    if [ "$version" == "GA1.3-rel" ]; then
      download_models "https://github.com/quic/sample-apps-for-qualcomm-linux/releases/download/${version}/v2.29_${chipset}.zip" ${outputmodelpath}
      download_labels "https://github.com/quic/sample-apps-for-qualcomm-linux/releases/download/${version}/labels.zip" ${outputlabelpath}
    elif [ "$version" == "GA1.4-rel" ]; then
      download_models "https://github.com/quic/sample-apps-for-qualcomm-linux/releases/download/GA1.4-rel/v2.32_${chipset}.zip" ${outputmodelpath}
      download_labels "https://github.com/quic/sample-apps-for-qualcomm-linux/releases/download/GA1.4-rel/labels.zip" ${outputlabelpath}
    else
      download_models "https://github.com/quic/sample-apps-for-qualcomm-linux/releases/download/GA1.5-rel/v2.35_${chipset}.zip" ${outputmodelpath}
      download_labels "https://github.com/quic/sample-apps-for-qualcomm-linux/releases/download/GA1.5-rel/labels.zip" ${outputlabelpath}
    fi

    # Download model and label files
    download_file "https://huggingface.co/qualcomm/Inception-v3-Quantized/resolve/main/Inception-v3-Quantized.tflite" "${outputmodelpath}/inception_v3_quantized.tflite"
    download_file "https://huggingface.co/qualcomm/DeepLabV3-Plus-MobileNet-Quantized/resolve/main/DeepLabV3-Plus-MobileNet-Quantized.tflite" "${outputmodelpath}/deeplabv3_plus_mobilenet_quantized.tflite"
    download_file "https://huggingface.co/qualcomm/Midas-V2-Quantized/resolve/main/Midas-V2-Quantized.tflite" "${outputmodelpath}/midas_quantized.tflite"
    download_file "https://huggingface.co/qualcomm/HRNetPoseQuantized/resolve/main/HRNetPoseQuantized.tflite" "${outputmodelpath}/hrnet_pose_quantized.tflite"
    download_file "https://qaihub-public-assets.s3.us-west-2.amazonaws.com/qai-hub-models/models/midas/midasv2_linux_assets/midasv2.dlc" "${outputmodelpath}/"
    download_file "https://qaihub-public-assets.s3.us-west-2.amazonaws.com/qai-hub-models/models/midas/midasv2_linux_assets/monodepth.labels" "${outputlabelpath}/"
    download_file "https://huggingface.co/qualcomm/QuickSRNetSmall-Quantized/resolve/main/QuickSRNetSmall-Quantized.tflite" "${outputmodelpath}/quicksrnetsmall_quantized.tflite"
    download_file "https://huggingface.co/qualcomm/Lightweight-Face-Detection-Quantized/resolve/main/Lightweight-Face-Detection-Quantized.tflite" "${outputmodelpath}/face_det_lite_quantized.tflite"
    download_file "https://huggingface.co/qualcomm/MobileNet-v2-Quantized/resolve/main/MobileNet-v2-Quantized.tflite" "${outputmodelpath}/mobilenet_v2_quantized.tflite"
    download_file "https://huggingface.co/qualcomm/Facial-Landmark-Detection-Quantized/resolve/main/Facial-Landmark-Detection-Quantized.tflite" "${outputmodelpath}/facemap_3dmm_quantized.tflite"
    download_file "https://huggingface.co/qualcomm/Facial-Attribute-Detection-Quantized/resolve/main/Facial-Attribute-Detection-Quantized.tflite" "${outputmodelpath}/face_attrib_net_quantized.tflite"
    download_file "https://huggingface.co/qualcomm/YamNet/resolve/main/YamNet.tflite" "${outputmodelpath}/yamnet.tflite"
    download_file "https://huggingface.co/qualcomm/Yolo-X/resolve/2885648dda847885e6fd936324856b519d239ee1/Yolo-X_w8a8.tflite" "${outputmodelpath}/yolox_quantized.tflite"
    download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/labels/detection.labels" "${outputlabelpath}/yolox.labels"

    if [ "$version" == "GA1.4-rel" ] || [ "$version" == "GA1.5-rel" ]; then
      # Download bin files for face recognition
      download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/data/blendShape.bin" "${outputdatapath}/"
      download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/data/meanFace.bin" "${outputdatapath}/"
      download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/data/shapeBasis.bin" "${outputdatapath}/"
      download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/videos/video.mp4" "${outputmediapath}/"
      download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/videos/video1.mp4" "${outputmediapath}/"
      download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/videos/video-flac.mp4" "${outputmediapath}/"
      download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/videos/video-mp3.mp4" "${outputmediapath}/"
    fi

    if [ "$version" == "GA1.3-rel" ]; then
        # Download config files
      download_config "https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/gst-plugins-qti-oss/-/raw/imsdk.lnx.2.0.0.r2-rel/gst-sample-apps/gst-ai-classification/config_classification.json?inline=false" "${outputconfigpath}/config_classification.json"
      download_config "https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/gst-plugins-qti-oss/-/raw/imsdk.lnx.2.0.0.r2-rel/gst-sample-apps/gst-ai-daisychain-detection-classification/config_daisychain_detection_classification.json?inline=false" "${outputconfigpath}/config_daisychain_detection_classification.json"
      download_config "https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/gst-plugins-qti-oss/-/raw/imsdk.lnx.2.0.0.r2-rel/gst-sample-apps/gst-ai-monodepth/config_monodepth.json?inline=false" "${outputconfigpath}/config_monodepth.json"
      download_config "https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/gst-plugins-qti-oss/-/raw/imsdk.lnx.2.0.0.r2-rel/gst-sample-apps/gst-ai-object-detection/config_detection.json?inline=false" "${outputconfigpath}/config_detection.json"
      download_config "https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/gst-plugins-qti-oss/-/raw/imsdk.lnx.2.0.0.r2-rel/gst-sample-apps/gst-ai-pose-detection/config_pose.json?inline=false" "${outputconfigpath}/config_pose.json"
      download_config "https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/gst-plugins-qti-oss/-/raw/imsdk.lnx.2.0.0.r2-rel/gst-sample-apps/gst-ai-segmentation/config_segmentation.json?inline=false" "${outputconfigpath}/config_segmentation.json"
    fi

    if [ "$version" == "GA1.3-rel" ]; then
      echo "Model,Label and Config files download Successful to ${outputpath} directory"
	else
      echo "Model and Label files download Successful to /etc/models/ and /etc/labels/ directory"
    fi

    exit 0
}

# Execute main function
main "$@"
