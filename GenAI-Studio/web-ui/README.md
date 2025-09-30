# Client

1. Create docker image
    ```
    docker buildx build --platform=linux/arm64/v8 --load --output type=docker -t web-ui . --no-cache
    ```
2. Save docker image
   ```
   docker save web-ui -o web-ui
   ```
3. Push docker image to device
4. Load image
    ```
    docker load -i web-ui
    ```
5. ```
    docker run --name web-ui --net host -d web-ui
   ```
6. Check logs
   ```
   docker logs -f web-ui
    ```
7. Check device IP
    ```
    adb shell "ifconfig"
    ```
8.  Open "<device-ip:8501>" to access webpage
    ```
    http://10.92.211.143:8501/
    ```

9. You can check more about [streamlit](https://streamlit.io/)
