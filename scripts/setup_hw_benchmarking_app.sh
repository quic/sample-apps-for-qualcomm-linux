#!/bin/bash

#Sample installer command
#makeself ~/demos/QCS6490-Vision-AI-Demo-Installer-project QCS6490-Vision-AI-Demo-Installer.run "Installing TRIA QCOMM demos" ./install.sh

## Clone the project
#!/bin/bash
## Clone the project
REPO_URL="https://github.com/Avnet/QCS6490-Vision-AI-Demo.git"
TARGET_DIR="QCS6490-Vision-AI-Demo"
BRANCH_NAME="QLI-1.5-and-1080p-support"

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
WESTON_DIR=$BASE_DIR"/etc/xdg/weston"
DEMO_APP_DIR=$BASE_DIR"/opt/QCS6490-Vision-AI-Demo"

echo "Make file system writable"
mount -o remount,rw /

# Create weston folder if not present
mkdir -p "$WESTON_DIR"
if [ $? -ne 0 ]; then
    echo "Make file system writable - failed"
    exit 1
fi

# Backup existing weston.ini if it exists and no backup exists yet
if [ -f "$WESTON_DIR/weston.ini" ]; then
    if [ ! -f "$WESTON_DIR/weston.ini.bak" ]; then
        cp "$WESTON_DIR/weston.ini" "$WESTON_DIR/weston.ini.bak"
        if [ $? -ne 0 ]; then
            echo "Backup of existing weston.ini failed"
            exit 1
        else
            echo "Backup created at $WESTON_DIR/weston.ini.bak"
        fi
    else
        echo "Backup already exists at $WESTON_DIR/weston.ini.bak — skipping backup"
    fi
fi

# Update weston.ini
echo "Update weston.ini"
yes | cp -rf weston.ini "$WESTON_DIR"
if [ $? -ne 0 ]; then
    echo "Update weston.ini - failed"
    exit 1
fi

chmod -R +x $DEMO_APP_DIR

##### Download artifacts #####

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


download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/labels/hrnet_pose.labels" "${outputlabelpath}/hrnet_pose.labels"
download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/labels/classification.labels" "${outputlabelpath}/classification.labels"
download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/labels/voc_labels.txt" "${outputlabelpath}/voc_labels.txt"
download_file "https://raw.githubusercontent.com/quic/sample-apps-for-qualcomm-linux/refs/heads/main/artifacts/labels/detection.labels" "${outputlabelpath}/yolox.labels"
download_file "https://qaihub-public-assets.s3.us-west-2.amazonaws.com/qai-hub-models/models/midas/midasv2_linux_assets/monodepth.labels" "${outputlabelpath}/monodepth.labels"

download_file "https://huggingface.co/qualcomm/Inception-v3/resolve/60c6a08f58919a0dc7e005ec02bdc058abb1181b/Inception-v3_w8a8.tflite" "${outputmodelpath}/inception_v3_quantized.tflite"
download_file "https://huggingface.co/qualcomm/FCN-ResNet50/resolve/3951b964f2b231d28ca0d04091dc661b0cc3f53c/FCN-ResNet50_w8a8.tflite" "${outputmodelpath}/fcn_resnet50_quantized.tflite"
download_file "https://huggingface.co/qualcomm/HRNetPose/resolve/dbfe1866bd2dbfb9eecb32e54b8fcdc23d77098b/HRNetPose_w8a8.tflite" "${outputmodelpath}/hrnet_pose_quantized.tflite"
download_file "https://huggingface.co/qualcomm/Midas-V2/resolve/13de42934d09fe7cda62d7841a218cbb323e7f7e/Midas-V2_w8a8.tflite" "${outputmodelpath}/midas_quantized.tflite"
download_file "https://huggingface.co/qualcomm/Yolo-X/resolve/2885648dda847885e6fd936324856b519d239ee1/Yolo-X_w8a8.tflite" "${outputmodelpath}/yolox_quantized.tflite"

## Reboot the device
reboot