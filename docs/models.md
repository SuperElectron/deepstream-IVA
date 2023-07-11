
__face detection__
- to create the [face_detection_model](https://github.com/NVIDIA-AI-IOT/redaction_with_deepstream) you can run this inside the container
- it will copy the generated model here `/opt/nvidia/deepstream/deepstream/sources/apps/redaction_with_deepstream/fd_lpd_model` to `/src/configs/model`
- it will also download sample videos into '/src/sample_videos'
```bash
make enter
cd /src/utils;
./download_models.sh
```