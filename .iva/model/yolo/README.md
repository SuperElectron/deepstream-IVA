# Yolo

- there are many versions that can be integrated quite easily, and this includes some custom variations as well!

__choose a model__

- click one of the following links for the steps to download and convert to onnx format
- NOTE: you can distribute the onnx model to various type of GPUs, and then create a serialized engine file on target hardweare


[yolov5](https://github.com/marcoslucianops/DeepStream-Yolo/blob/master/docs/YOLOv5.md)

[yolov5](https://github.com/marcoslucianops/DeepStream-Yolo/blob/master/docs/YOLOv6.md)

[yolov7](https://github.com/marcoslucianops/DeepStream-Yolo/blob/master/docs/YOLOv7.md)

[yolov8](https://github.com/marcoslucianops/DeepStream-Yolo/blob/master/docs/YOLOv8.md)

__warning__

- it is strongly suggested to NOT use the `--dynamic` flag when converting the model

__convert to onnx example__

- first we download a repo with conversion files

```bash
# clone conversion repo
export MY_ROOT_DIR=~
cd $MY_ROOT_DIR && \
  git clone https://github.com/marcoslucianops/DeepStream-Yolo.git
```


- we will convert a yolov5 model

```bash
# clone ultralytics repo
export MY_ROOT_DIR=~
cd $MY_ROOT_DIR && \
  git clone https://github.com/ultralytics/yolov5.git
cd $MY_ROOT_DIR/yolov5 && \
  pip3 install -r requirements.txt
cd $MY_ROOT_DIR/yolov5 && \
  pip3 install onnx onnxsim onnxruntime

# clone, copy, and convert
cp $MY_ROOT_DIR/DeepStream-Yolo/utils/export_yoloV5.py $MY_ROOT_DIR/yolov5
cd $MY_ROOT_DIR/yolov5 && \
  wget https://github.com/ultralytics/yolov5/releases/download/v7.0/yolov5s.pt && \
  python3 export_yoloV5.py -w yolov5s.pt && \
  cp yolov5.onnx /tmp/.cache/configs/model/yolo/yolov5s.onnx
```

- and another one for yolov8

```bash
# clone ultralytics repo
git clone https://github.com/ultralytics/ultralytics.git
cd ultralytics
pip3 install -r requirements.txt
python3 setup.py install
pip3 install onnx onnxsim onnxruntime

# clone, copy, and convert
cp $MY_ROOT_DIR/DeepStream-Yolo/utils/export_yoloV8.py $MY_ROOT_DIR/ultralytics
cd $MY_ROOT_DIR/ultralytics && \
  wget https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8s.pt && \
  python3 export_yoloV8.py -w yolov8s.pt && \
  cp yolov8s.onnx /tmp/.cache/configs/model/yolo/yolov8s.onnx

```

__configs__

- to run with the application, update the following file-paths with your model name 
- note `yolov8s_model_b1_gpu0_fp32.engine` doesn't exist yet!  That is because the `iva` app uses TensorRT to convert the onnx file to a serialized engine file.
    - after you run the `iva` app once, you can move `model_b1_gpu0_fp32.engine` to `/tmp/.cache/configs/model/yolo/yolov8s_model_b1_gpu0_fp32.engine` (as an example of running yolov8s)
```yaml
  onnx-file: /tmp/.cache/configs/model/yolo/yolov8s.onnx
  model-engine-file: /tmp/.cache/configs/model/yolo/yolov8s_model_b1_gpu0_fp32.engine
  labelfile-path: /tmp/.cache/configs/model/yolo/labels.txt
```

- these are static fields that must be included so that the outputs can be converted into metadata for tracking
```yaml

  parse-bbox-func-name: NvDsInferParseYolo
  custom-lib-path: /usr/local/lib/libnvdsinfer_custom_impl_Yolo.so
  engine-create-func-name: NvDsInferYoloCudaEngineGet
```