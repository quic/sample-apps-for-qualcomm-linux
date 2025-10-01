#!/bin/bash
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

#Sample installer command
#makeself ~/demos/QCS6490-Vision-AI-Demo-Installer-project QCS6490-Vision-AI-Demo-Installer.run "Installing TRIA QCOMM demos" ./install.sh

## Clone the project
#!/bin/bash
## Clone the project
REPO_URL="https://github.com/Avnet/QCS6490-Vision-AI-Demo.git"
TARGET_DIR="QCS6490-Vision-AI-Demo"
BRANCH_NAME="main"

# Check if the target directory already exists and is a valid Git repo
if [ -d "$TARGET_DIR" ]; then
  if [ -d "$TARGET_DIR/.git" ]; then
    echo "ℹ️ Directory '$TARGET_DIR' already contains a Git repository. Skipping clone."
  else
    echo "❌ Directory '$TARGET_DIR' exists but is not a Git repository. Aborting to prevent overwriting."
    exit 1
  fi
else
  # Clone the repository
  echo "🔄 Cloning repository from $REPO_URL into $TARGET_DIR..."
  if ! git clone "$REPO_URL" "$TARGET_DIR"; then
    echo "❌ Failed to clone the repository. Check your network connection or the repository URL."
    exit 1
  fi
fi

# Change to the target directory
cd "$TARGET_DIR"

# Check out the specified branch
echo "🔀 Checking out branch '$BRANCH_NAME'..."
if git show-ref --verify --quiet "refs/heads/$BRANCH_NAME"; then
  git checkout "$BRANCH_NAME"
elif git ls-remote --exit-code --heads origin "$BRANCH_NAME" > /dev/null; then
  git fetch origin "$BRANCH_NAME":"$BRANCH_NAME"
  git checkout "$BRANCH_NAME"
else
  echo "❌ Branch '$BRANCH_NAME' does not exist in the repository."
  exit 1
fi

echo "✅ Repository is ready on branch '$BRANCH_NAME'."



## Continue with installation

BASE_DIR=""
DEMO_APP_DIR=$BASE_DIR"/opt/QCS6490-Vision-AI-Demo"
outputmodelpath="/etc/models"
outputlabelpath="/etc/labels"

echo "Make file system writable"
mount -o remount,rw /

##### Make demo executable  #####
chmod -R +x $DEMO_APP_DIR

##### Download ML artifacts #####

# Helper functions
# Function to download files
download_file() {

    local url="$1"
    local target_path="$2"
    local filename
    filename=$(basename "$target_path")

    echo "📥 Downloading $url..."

    # Download the file using curl with error handling
    if ! curl -fL -o "$filename" "$url"; then
        echo "❌ Error: Failed to download $url"
        exit 1
    fi

    # Create the target directory if it doesn't exist
    local target_dir
    target_dir=$(dirname "$target_path")
    if [ ! -d "$target_dir" ]; then
        echo "📁 Target directory '$target_dir' does not exist. Creating it..."
        mkdir -p "$target_dir"
    fi

    # Move the file to the target path
    if ! mv "$filename" "$target_path"; then
        echo "❌ Error: Failed to move $filename to $target_path"
        exit 1
    fi

    echo "✅ File downloaded and moved to $target_path"
	echo ""
}


outputmodelpath="/etc/models"
outputlabelpath="/etc/labels"

mkdir -p "${outputmodelpath}"
mkdir -p "${outputlabelpath}"

### Labels
download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/json_labels/hrnet_pose.json" "${outputlabelpath}/hrnet_pose.json"
download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/json_labels/classification.json" "${outputlabelpath}/classification.json"
download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/json_labels/deeplabv3_resnet50.json" "${outputlabelpath}/deeplabv3_resnet50.json"
download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/json_labels/yolox.json" "${outputlabelpath}/yolox.json"
download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/json_labels/monodepth.json" "${outputlabelpath}/monodepth.json"


### Model files
download_file "https://huggingface.co/qualcomm/Inception-v3/resolve/60c6a08f58919a0dc7e005ec02bdc058abb1181b/Inception-v3_w8a8.tflite" "${outputmodelpath}/inception_v3_quantized.tflite"
download_file "https://huggingface.co/qualcomm/DeepLabV3-Plus-MobileNet/resolve/fa276f89cce8ed000143d40e8887decbbea57012/DeepLabV3-Plus-MobileNet_w8a8.tflite" "${outputmodelpath}/deeplabv3_plus_mobilenet_quantized.tflite"
download_file "https://huggingface.co/qualcomm/HRNetPose/resolve/dbfe1866bd2dbfb9eecb32e54b8fcdc23d77098b/HRNetPose_w8a8.tflite" "${outputmodelpath}/hrnet_pose_quantized.tflite"
download_file "https://huggingface.co/qualcomm/Midas-V2/resolve/13de42934d09fe7cda62d7841a218cbb323e7f7e/Midas-V2_w8a8.tflite" "${outputmodelpath}/midas_quantized.tflite"
download_file "https://huggingface.co/qualcomm/Yolo-X/resolve/2885648dda847885e6fd936324856b519d239ee1/Yolo-X_w8a8.tflite" "${outputmodelpath}/yolox_quantized.tflite"


### Settings
download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/json_labels/hrnet_settings.json" "${outputlabelpath}/hrnet_settings.json"


### Install psutil
echo "Checking for pip3..."
if ! command -v pip3 &> /dev/null; then
    echo "Error: pip3 is not installed. Cannot proceed with psutil installation."
    exit 1
fi

echo "Installing psutil with pip3..."
pip3 install --upgrade psutil

echo "psutil installation complete."
