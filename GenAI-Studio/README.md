# Introduction
GenAI Studio contains ready to deploy solutions on Qualcomm Dragonwing platform.
## Supported platforms
Qualcomm IQ9 - Ubuntu X05

Qualcomm IQ9 - QLI 1.5

# For Qualcomm Ubuntu
## Qualcomm packages
```
sudo add-apt-repository ppa:ubuntu-qcom-iot/qcom-ppa
```
```
sudo apt update
```
```
sudo apt install -y qcom-fastrpc1 qcom-libdmabufheap-dev qcom-fastrpc-dev qcom-dspservices-headers-dev libqnn1 qnn-tools libsnpe1 snpe-tools
```
### CDI setup

```
curl -L -O https://git.codelinaro.org/clo/le/sdk-tools/-/raw/imsdk-tools.lnx.1.0.r1-rel/qimsdk-ubuntu/generate_cdi_json.sh?ref_type=heads&inline=false
```
```
bash generate_cdi_json.sh
```

```
ls /etc/cdi/docker-run-cdi-hw-acc.json
```

```
sudo chown -R ubuntu:ubuntu /opt/
```

### Validate Qualcomm NPU
```
snpe-platform-validator --runtime dsp
```
#### Expected output
SNPE is supported for runtime DSP on the device
![image](https://github.qualcomm.com/aicatalog/genai-studio/assets/30177/a24ab16d-bec7-402e-aba1-05f7bc72e022)

## Docker Installation
#### Update package index
```
sudo apt-get update
 ```
#### Install required packages
```
sudo apt-get install -y ca-certificates curl gnupg lsb-release
 ```
#### Add Docker’s official GPG key
```
sudo mkdir -p /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | \
    sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg
 ```
#### Set up the Docker repository
```
echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] \
  https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" | \
  sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
 ```
#### Update package index again
```
sudo apt-get update
 ```

#### Install Docker Engine
```
sudo apt-get install -y docker-ce docker-ce-cli containerd.io  docker-compose
```

#### Add your user to the docker group (to run docker without sudo)
```
sudo usermod -aG docker $USER
```

### Update /etc/docker/daemon.json
```
mkdir -p /etc/docker/
```
### Copy content to /etc/docker/daemon.json
``` 
{
   "features": {
      "cdi": true
   }
}
```
```
systemctl restart docker
```
## Docker containers
## Build container images (Linux x86)
#### NOTE: Run below commands on x86 machine
```
cd Speech-To-Text
docker  build --progress=plain --platform=linux/arm64/v8  -t asr .
docker save -o asr asr
```
```
cd Text-Generation
docker  build --progress=plain --platform=linux/arm64/v8  -t text2text . 
docker save text2image -o text2image
```
```
cd web-ui
docker  build --progress=plain --platform=linux/arm64/v8  -t web-ui . 
docker save text2image -o web-ui
```

## Pre-built container images (aarch64)
#### NOTE: Push container images to device following above and run below commands on target aarch64 device
```
docker load -i asr
docker load -i text2text
docker load -i web-ui
```
## LLM steps (Linux X86)
### Environment Setup
Follow https://github.com/quic/ai-hub-apps/tree/main/tutorials/llm_on_genie
### Model Generation
```
python -m qai_hub_models.models.llama_v3_8b_instruct.export --chipset qualcomm-snapdragon-x-elite --skip-inferencing --skip-profiling --output-dir genie_bundle
```

## Start GenAI Studio (Target Device aarch64)
```
docker-compose -f docker-compose.yml up -d
```
#### Expected output
![image](https://github.qualcomm.com/aicatalog/genai-studio/assets/30177/4b0e35aa-1fb6-4b7f-a8db-40b3562f40a8)

**NOTE:** If you face this error "CDI device injection failed: failed to inject devices: failed to stat CDI host device "/dev/kgsl-3d0": no such file or directory"

**Solution**: Remove "/dev/kgsl-3d0" from /etc/cdi/docker-run-cdi-hw-acc.json
```
{
   "path": "/dev/kgsl-3d0"
},
```
### Network URL
```
docker logs -f web-ui
```
#### Expected output

  You can now view your Streamlit app in your browser.

  Local URL: http://localhost:8501

  Network URL: http://192.168.0.4:8501


#### NOTE
Click on http://192.168.0.4:8501 to open webpage

## Stop  GenAI Studio
```
docker-compose -f docker-compose.yml down
```
#### Expected output
![image](https://github.qualcomm.com/aicatalog/genai-studio/assets/30177/6db82450-22fe-4c4b-8990-ab2caac5894e)
