#!/bin/bash

# Function to display help menu
show_help() {
    echo "Usage: $0 -v <version> -c <chipset>"
    echo
    echo "Options:"
    echo "  -v, --version    Select the release version to download (e.g., GA1.2-rel)"
    echo "  -c, --chipset    Select the chipset directory to copy files from (e.g., QCS6490, QCS9075)"
    echo "  -h, --help       Help menu"
    echo
    echo "Example:"
    echo "  ./download_artifacts.sh -v GA1.2-rel -c QCS6490"}

# Function to check internet connectivity
check_internet() {
    if ping -c 1 www.qualcomm.com &> /dev/null; then
        echo "Internet is connected"
    else
        echo "No internet connection"
        echo "Connect the device to network to download the model and label files"
        exit 1
    fi
}

# Function to download and unzip files
download_and_unzip() {
    local url=$1
    local output_dir=$2
    local chipset=$3
    curl -L -O "$url" && unzip -o "$(basename "$url")" && \
    cp "$(basename "$url" .zip)/$chipset"/* "$output_dir"
}

# Function to download files
download_file() {
    local url=$1
    local output_path=$2
    curl -o "$output_path" "$url"
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
    download_and_unzip "https://github.com/quic/sample-apps-for-qualcomm-linux/releases/download/${version}/${version}.zip" "/opt" "$chipset"

    # Download model and label files
    download_file "https://huggingface.co/qualcomm/Inception-v3-Quantized/blob/main/Inception-v3-Quantized.tflite" "/opt/Inception-v3-Quantized.tflite"
    download_file "https://huggingface.co/qualcomm/DeepLabV3-Plus-MobileNet-Quantized/blob/main/DeepLabV3-Plus-MobileNet-Quantized.tflite" "/opt/DeepLabV3-Plus-MobileNet-Quantized.tflite"
    download_file "https://huggingface.co/qualcomm/Midas-V2-Quantized/blob/main/Midas-V2-Quantized.tflite" "/opt/Midas-V2-Quantized.tflite"
    download_file "https://huggingface.co/qualcomm/HRNetPoseQuantized/blob/main/HRNetPoseQuantized.tflite" "/opt/HRNetPoseQuantized.tflite"
    download_file "https://qaihub-public-assets.s3.us-west-2.amazonaws.com/qai-hub-models/models/midas/midasv2_linux_assets/midasv2.dlc" "/opt/midasv2.dlc"
    download_file "https://qaihub-public-assets.s3.us-west-2.amazonaws.com/qai-hub-models/models/midas/midasv2_linux_assets/monodepth.labels" "/opt/monodepth.labels"

    echo "Model and Label files download Successful to /opt directory"
    exit 0
}

# Execute main function
main "$@"
