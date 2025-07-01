# Below are the steps for compliation of gst-rtspsrc-stream-example Sample Application

1. cd gst-sample-applications
2. export SDKTARGETSYSROOT=<path to installation directory of platfom SDK>/tmp/sysroots
   e.g.: export SDKTARGETSYSROOT=/local/mnt/workspace/Platform_eSDK/tmp/sysroots

3. export MACHINE=<machine>
   e.g export MACHINE=qcs615-adp-air

4. export GST_APP_NAME=<appname> 
   e.g.: export GST_APP_NAME=gst-rtspsrc-stream-example
5. Do run make
6. Application executable will be generated and push the bin to target device and validate the usecase.
