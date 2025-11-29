#!/bin/bash

# Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

# Show help menu
show_help() {
    cat << "EOF"
Usage: $0 [OPTIONS]

This script downloads and prepares required artifacts for your environment.

Options:
  -h, --help        Display this help message and exit.

Examples:
  ./download_artifacts.sh                # Run the script with default settings
  ./download_artifacts.sh --help         # Show detailed usage instructions

Description:
  The script automates downloading models, configs, and related files from predefined URLs,
  unzips them, and organizes them into the appropriate directories for further processing.
EOF
}

# Check internet connectivity
check_internet() {
    if ping -c 1 8.8.8.8 &> /dev/null; then
        echo "Internet is connected"
    else
        echo "No internet connection"
        echo "Connect the device to network to download the model and label files"
        exit 1
    fi
}

# Copies the specified media file into multiple numbered video files (video02.mp4 to video16.mp4) in the given output directory.
copy_videos() {
    local output_media_dir=$1
    local media_file=$2

    # echo "Copying ${media_file}"
    for i in $(seq -w 2 16); do
        cp "${output_media_dir}/${media_file}" "${output_media_dir}/video${i}.mp4"
    done
}

# Determines the device build type (QLI or Ubuntu) and GA version based on system information and package checks.
check_build_type() {
    uname_output=$(uname -a)
    os_release=$(cat /etc/os-release)

    if [[ $uname_output == *"qli"* ]]; then
        # echo $uname_output
        version=$(echo "$uname_output" | sed -n 's/.*-qli-\([0-9]\+\.[0-9]\+\).*/\1/p')
        ga_version_check="GA${version}-rel"
        build_type="QLI"
    
    elif [[ $os_release == *"Qualcomm Linux"* ]]; then
        version=$(echo "$os_release" | grep '^VERSION=' | sed -n 's/^VERSION=["'\'']\?\([0-9]\+\.[0-9]\+\)\(-ver.*\)\?["'\'']\?/\1/p')
        ga_version_check="GA${version}-rel"
        build_type="QLI"

    else
        build_type="Ubuntu"
    fi

    echo "The build type is ${build_type}"
    # echo "GA Version of the device: ${ga_version_check}"
}

# Determines the device build type (QLI or Ubuntu) and GA version based on system information and package checks.
check_qairt_version() {
    qairt_version=""
    if command -v snpe-net-run >/dev/null 2>&1; then
        raw=$(snpe-net-run --version 2>&1 | awk '{print $2}' | sed 's/.*v//')
        echo "SNPE runtime detected"
        echo "SNPE version: ${raw}"
        
        split_qairt_version $raw
        qairt_short_version="${major}.${minor}"
        qairt_version="${major}.${minor}.${patch}.${build:0:6}"
        return 0
    fi

    if command -v qnn-net-run >/dev/null 2>&1; then
        raw=$(qnn-net-run --version 2>&1 | grep "QNN SDK" | sed 's/.*v//')
        if [ -n "$raw" ]; then
            echo "QNN runtime detected"
            echo "QNN version: ${raw}"
            
            split_qairt_version $raw
            qairt_short_version="${major}.${minor}"
            qairt_version="${major}.${minor}.${patch}.${build:0:6}"
            return 0
        fi
    fi
    return 1
}

# Splits the QAIRT version to extract major minor patch and build values of the release
split_qairt_version() {
    local raw=$1
    IFS='.' read -r major minor patch build <<< "$raw"
}

# Mapping of the QAIRT Version and its supported Model Version on AI HUB
declare -A qairt_map=(
    ["2.39.0.250925"]="v0.40.0"
)

# Extracts the latest QAIRT version that is available
extract_latest_qairt_version() {
    qairt_version=$(printf "%s\n" "${!qairt_map[@]}" | sort -V | tail -n 1)
    split_qairt_version $qairt_version
    qairt_short_version="${major}.${minor}"
}

# Fetches a zip file from the given URL, extracts its contents (model files) into the specified directory, and deletes the downloaded archive and temporary folder.
download_models() {
    local url=$1
    local output_dir=$2
    echo "$url"
    curl -L -O "$url" && unzip -o "$(basename "$url")" && \
    cp "$(basename "$url" .zip)"/* $output_dir
    rm -rf "$(basename "$url" .zip)"
    rm -rf "$(basename "$url")"
}

# Fetches a zip file from the given URL, extracts its contents (label files) into the specified directory, and deletes the downloaded archive and temporary folder.
download_labels() {
    local url=$1
    local output_dir=$2
    echo "$url"
    curl -L -O "$url" && unzip -o "$(basename "$url")" && \
    cp "$(basename "$url" .zip)"/* $output_dir
    cp "$(basename "$url" .zip)"/yolonas.labels "$(basename "$url" .zip)"/yolov8.labels
    rm -rf "$(basename "$url" .zip)"
    rm -rf "$(basename "$url")"
}

# Downloads a file from the given URL and moves it into the specified output directory.
download_file() {
    local url=$1
    local output_dir=$2
    echo "$url"
    curl -L -O "$url"
    # echo "$output_dir"
    mv "$(basename "$url")" "$output_dir"
}

# Downloads a config file from the given URL and saves it directly to the specified output directory.
download_config() {
    local url=$1
    local output_dir=$2
    echo "$url"
    curl -L -o "$output_dir" "$url"
}

# Creates a directory
create_directory() {
    local dir_path=$1
    mkdir -p "${dir_path}"
}

# Downloads the necessary TFLite and DLC models
download_model_artifacts() {
    
    if awk "BEGIN {exit !($qairt_short_version < 2.39)}"; then
        output_path="/opt"
        adjusted_qairt_version=$(awk "BEGIN {
            v=$qairt_short_version-0.01;
            rounded=int((v*100)/3)*3/100;
            printf \"%.2f\", rounded+0.01
        }")

        case "$adjusted_qairt_version" in
            2.29) ga_version="GA1.3-rel" ;;
            2.32) ga_version="GA1.4-rel" ;;
            2.35) ga_version="GA1.5-rel" ;;
            2.38) ga_version="GA1.6-rel" ;;
        esac

        if [ "$ga_version" == "GA1.3-rel" ]; then

            output_model_path="$output_path"
            output_label_path="$output_path"
            output_config_path="$output_path"
            output_media_path="$output_path"
            output_data_path="/etc/data"

            create_directory "$output_model_path"
            create_directory "$output_data_path"
        else
            output_model_path="/etc/models"
            output_label_path="/etc/labels"
            output_data_path="/etc/data"
            output_media_path="/etc/media"

            create_directory "$output_model_path"
            create_directory "$output_label_path"
            create_directory "$output_data_path"
            create_directory "$output_media_path"

        fi

        if [ "$ga_version" == "GA1.3-rel" ]; then
            download_models "https://github.com/quic/sample-apps-for-qualcomm-linux/releases/download/${ga_version}/v2.29_${chipset}.zip" ${output_model_path}   
        elif [ "$ga_version" == "GA1.4-rel" ]; then
            download_models "https://github.com/quic/sample-apps-for-qualcomm-linux/releases/download/${ga_version}/v2.32_${chipset}.zip" ${output_model_path}
        elif [ "$ga_version" == "GA1.5-rel" ]; then
            download_models "https://github.com/quic/sample-apps-for-qualcomm-linux/releases/download/${ga_version}/v2.35_${chipset}.zip" ${output_model_path}
        else
            download_models "https://github.com/quic/sample-apps-for-qualcomm-linux/releases/download/${ga_version}/v2.38_${chipset}.zip" ${output_model_path}
        fi

        if [ "$ga_version" == "GA1.4-rel" ] || [ "$ga_version" == "GA1.3-rel" ]; then
            download_file "https://huggingface.co/qualcomm/Inception-v3/resolve/v0.29.1/Inception-v3_w8a8.tflite" "${output_model_path}/inception_v3_quantized.tflite"
            download_file "https://huggingface.co/qualcomm/DeepLabV3-Plus-MobileNet/resolve/2751392b3ca5e6e8cd3316f4c62501aa17c268e8/DeepLabV3-Plus-MobileNet_w8a8.tflite" "${output_model_path}/deeplabv3_plus_mobilenet_quantized.tflite"
            download_file "https://huggingface.co/qualcomm/Midas-V2/resolve/v0.29.1/Midas-V2_w8a8.tflite" "${output_model_path}/midas_quantized.tflite"
            download_file "https://huggingface.co/qualcomm/HRNetPose/resolve/v0.29.1/HRNetPose_w8a8.tflite" "${output_model_path}/hrnet_pose_quantized.tflite"
            download_file "https://huggingface.co/qualcomm/QuickSRNetSmall/resolve/977fb3092a065d512cd587c210cc1341b28b7161/QuickSRNetSmall_w8a8.tflite" "${output_model_path}/quicksrnetsmall_quantized.tflite"
            download_file "https://huggingface.co/qualcomm/Lightweight-Face-Detection/resolve/v0.30.3/Lightweight-Face-Detection_w8a8.tflite" "${output_model_path}/face_det_lite_quantized.tflite"
            download_file "https://huggingface.co/qualcomm/Facial-Landmark-Detection/resolve/v0.29.1/Facial-Landmark-Detection_w8a8.tflite" "${output_model_path}/facemap_3dmm_quantized.tflite"
            download_file "https://huggingface.co/qualcomm/Facial-Attribute-Detection/resolve/v0.29.1/Facial-Attribute-Detection_w8a8.tflite" "${output_model_path}/face_attrib_net_quantized.tflite"
            download_file "https://huggingface.co/qualcomm/YamNet/resolve/4167a3af6245a2b611c9f7918fddefd8b0de52dc/YamNet.tflite" "${output_model_path}/yamnet.tflite"
        fi

        if [ "$ga_version" == "GA1.5-rel" ] || [ "$ga_version" == "GA1.6-rel" ]; then
            download_file "https://huggingface.co/qualcomm/Inception-v3/resolve/ba8121b0a74c7e28b45b250064c26efc7e7da29e/Inception-v3_w8a8.tflite" "${output_model_path}/inception_v3_quantized.tflite"
            download_file "https://huggingface.co/qualcomm/DeepLabV3-Plus-MobileNet/resolve/2751392b3ca5e6e8cd3316f4c62501aa17c268e8/DeepLabV3-Plus-MobileNet_w8a8.tflite" "${output_model_path}/deeplabv3_plus_mobilenet_quantized.tflite"
            download_file "https://huggingface.co/qualcomm/Midas-V2/resolve/d182b62632d80d3d1690f6e13fec18dd09c05fdf/Midas-V2_w8a8.tflite" "${output_model_path}/midas_quantized.tflite"
            download_file "https://huggingface.co/qualcomm/HRNetPose/resolve/dbfe1866bd2dbfb9eecb32e54b8fcdc23d77098b/HRNetPose_w8a8.tflite" "${output_model_path}/hrnet_pose_quantized.tflite"
            download_file "https://huggingface.co/qualcomm/QuickSRNetSmall/resolve/977fb3092a065d512cd587c210cc1341b28b7161/QuickSRNetSmall_w8a8.tflite" "${output_model_path}/quicksrnetsmall_quantized.tflite"
            download_file "https://huggingface.co/qualcomm/Lightweight-Face-Detection/resolve/e80b4d954ffaaefe2958e70b27bee77e74cc8550/Lightweight-Face-Detection_w8a8.tflite" "${output_model_path}/face_det_lite_quantized.tflite"
            download_file "https://huggingface.co/qualcomm/Facial-Landmark-Detection/resolve/2353f012b24fd407902e38d0c5fdd591cd2b1380/Facial-Landmark-Detection_w8a8.tflite" "${output_model_path}/facemap_3dmm_quantized.tflite"
            download_file "https://huggingface.co/qualcomm/Facial-Attribute-Detection/resolve/228624993581944d488f232ae50174795d489661/Facial-Attribute-Detection_w8a8.tflite" "${output_model_path}/face_attrib_net_quantized.tflite"
            download_file "https://huggingface.co/qualcomm/YamNet/resolve/4167a3af6245a2b611c9f7918fddefd8b0de52dc/YamNet.tflite" "${output_model_path}/yamnet.tflite"
            download_file "https://huggingface.co/qualcomm/Yolo-X/resolve/v0.30.5/Yolo-X_w8a8.tflite" "${output_model_path}/yolox_quantized.tflite"
        fi

        if [ "$ga_version" == "GA1.3-rel" ]; then
            # Download config files
            download_config "https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/gst-plugins-qti-oss/-/raw/4cb0d59ade7b996db4fc5e8f9af3a963ce7d767e/gst-sample-apps/gst-ai-classification/config_classification.json?inline=false" "${output_config_path}/config_classification.json"
            download_config "https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/gst-plugins-qti-oss/-/raw/4cb0d59ade7b996db4fc5e8f9af3a963ce7d767e/gst-sample-apps/gst-ai-daisychain-detection-classification/config_daisychain_detection_classification.json" "${output_config_path}/config_daisychain_detection_classification.json"
            download_config "https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/gst-plugins-qti-oss/-/raw/4cb0d59ade7b996db4fc5e8f9af3a963ce7d767e/gst-sample-apps/gst-ai-monodepth/config_monodepth.json" "${output_config_path}/config_monodepth.json"
            download_config "https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/gst-plugins-qti-oss/-/raw/4cb0d59ade7b996db4fc5e8f9af3a963ce7d767e/gst-ai-object-detection/config_detection.json?inline=false" "${output_config_path}/config_detection.json"
            download_config "https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/gst-plugins-qti-oss/-/raw/4cb0d59ade7b996db4fc5e8f9af3a963ce7d767e/gst-ai-pose-detection/config_pose.json?inline=false" "${output_config_path}/config_pose.json"
            download_config "https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/gst-plugins-qti-oss/-/raw/4cb0d59ade7b996db4fc5e8f9af3a963ce7d767e/gst-sample-apps/gst-ai-segmentation/config_segmentation.json?inline=false" "${output_config_path}/config_segmentation.json"
        fi

    else
        
        if [[ ! -v qairt_map[$qairt_version] ]]; then
            echo "QAIRT version $qairt_version not supported"
            extract_latest_qairt_version
            echo "Defaulting to the latest supported QAIRT version available: $qairt_version"
        fi

        model_version=${qairt_map[$qairt_version]}

        output_model_path="/etc/models"
        output_label_path="/etc/labels"
        output_data_path="/etc/data"
        output_media_path="/etc/media"

        create_directory "$output_model_path"
        create_directory "$output_label_path"
        create_directory "$output_data_path"
        create_directory "$output_media_path"

        #tflite models
        download_file "https://huggingface.co/qualcomm/Inception-v3/resolve/${model_version}/Inception-v3_w8a8.tflite" "${output_model_path}/inception_v3_quantized.tflite"
        download_file "https://huggingface.co/qualcomm/DeepLabV3-Plus-MobileNet/resolve/${model_version}/DeepLabV3-Plus-MobileNet_w8a8.tflite" "${output_model_path}/deeplabv3_plus_mobilenet_quantized.tflite"
        download_file "https://huggingface.co/qualcomm/Midas-V2/resolve/${model_version}/Midas-V2_w8a8.tflite" "${output_model_path}/midas_quantized.tflite"
        download_file "https://huggingface.co/qualcomm/HRNetPose/resolve/${model_version}/HRNetPose_w8a8.tflite" "${output_model_path}/hrnet_pose_quantized.tflite"
        download_file "https://huggingface.co/qualcomm/QuickSRNetSmall/resolve/${model_version}/QuickSRNetSmall_w8a8.tflite" "${output_model_path}/quicksrnetsmall_quantized.tflite"
        download_file "https://huggingface.co/qualcomm/Lightweight-Face-Detection/resolve/${model_version}/Lightweight-Face-Detection_w8a8.tflite" "${output_model_path}/face_det_lite_quantized.tflite"
        download_file "https://huggingface.co/qualcomm/Facial-Landmark-Detection/resolve/${model_version}/Facial-Landmark-Detection_w8a8.tflite" "${output_model_path}/facemap_3dmm_quantized.tflite"
        download_file "https://huggingface.co/qualcomm/Facial-Attribute-Detection/resolve/${model_version}/Facial-Attribute-Detection_w8a8.tflite" "${output_model_path}/face_attrib_net_quantized.tflite"
        download_file "https://huggingface.co/qualcomm/YamNet/resolve/4167a3af6245a2b611c9f7918fddefd8b0de52dc/YamNet.tflite" "${output_model_path}/yamnet.tflite"
        download_file "https://huggingface.co/qualcomm/Yolo-X/resolve/v0.30.5/Yolo-X_w8a8.tflite" "${output_model_path}/yolox_quantized.tflite"

        #dlc models
        download_file "https://huggingface.co/qualcomm/Inception-v3/resolve/${model_version}/Inception-v3_w8a8.dlc" "${output_model_path}/inceptionv3.dlc"
        download_file "https://huggingface.co/qualcomm/DeepLabV3-Plus-MobileNet/resolve/${model_version}/DeepLabV3-Plus-MobileNet_w8a8.dlc" "${output_model_path}/deeplabv3_plus_mobilenet.dlc"
        download_file "https://huggingface.co/qualcomm/Midas-V2/resolve/${model_version}/Midas-V2_w8a8.dlc" "${output_model_path}/midasv2.dlc"
        
    fi
    
}

# Extracts the chipset 
extract_chipset() {
    local chipset_file="/sys/devices/soc0/machine"

    if [[ -f "$chipset_file" ]]; then
        chipset=$(cat "$chipset_file")
        if [[ -n "$chipset" ]]; then
            echo "Chipset info: $chipset"
        else
            echo "Chipset info file exists but is empty"
        fi
    else
        echo "Chipset info file not found"
    fi
}


main() {

    while [[ "$#" -gt 0 ]]; do
        case $1 in
            -h|--help) show_help; exit 0 ;;
            *) echo "Unknown parameter passed: $1"; show_help; exit 1 ;;
        esac
        shift
    done


    #check_internet
    
    if ! check_qairt_version; then
        echo "No QAIRT runtime found"
        extract_latest_qairt_version
        echo "Defaulting to the latest supported QAIRT version available: $qairt_version"
    fi

    if ! check_build_type; then
        echo "Build type not supported"
        exit 1
    fi

    extract_chipset

    download_model_artifacts

    # Download the label files with both .labels and .json extensions
    download_labels "https://github.com/quic/sample-apps-for-qualcomm-linux/releases/download/GA1.5-rel/labels.zip" ${output_label_path}
    download_labels "https://github.com/quic/sample-apps-for-qualcomm-linux/releases/download/GA1.6-labels/labels.zip" ${output_label_path}

    # Download the necessary artifacts for the face recognition application
    download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/data/blendShape.bin" "${output_data_path}/blendShape.bin"
    download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/data/meanFace.bin" "${output_data_path}/meanFace.bin"
    download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/data/shapeBasis.bin" "${output_data_path}/shapeBasis.bin"
    download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/videos/video.mp4" "${output_media_path}/video.mp4"
    download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/videos/video1.mp4" "${output_media_path}/video1.mp4"
    download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/videos/video-flac.mp4" "${output_media_path}/video-flac.mp4"
    download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/videos/video-mp3.mp4" "${output_media_path}/video-mp3.mp4"

    # Creates copies of the video1.mp4 file 
    copy_videos ${output_media_path} video1.mp4

    echo "Model and Label files download successfully"
}

main "$@"
 
