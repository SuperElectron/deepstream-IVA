# peopleSegNet

- This model is based on MaskRCNN with ResNet50 as its feature extractor. 
- It detects one or more “person” objects within an image and returns a box around each object, as well as a segmentation mask for each object


[Nvidia NGC](https://catalog.ngc.nvidia.com/orgs/nvidia/teams/tao/models/peoplesegnet)

__download the model__

- there are a few different models available, and you can also use the link above and navigate to "version history" or "File browser" for more information about variations.


```bash
# clone conversion repo
sudo apt-get install zip unzip git -yqq

cd <PROJECT ROOT>
cd .cache/configs/models && \
  mkdir -p peopleSegNet && \
  wget --content-disposition https://api.ngc.nvidia.com/v2/models/nvidia/tao/peoplesegnet/versions/deployable_v2.0.2/zip -O peoplesegnet_deployable_v2.0.2.zip && \
  unzip peoplesegnet_deployable_v2.0.2.zip -d peopleSegNet && \
  rm peoplesegnet_deployable_v2.0.2.zip
```

__configs__

- looking at `detection.yaml`, many of the configs can be matched to the model-link [Nvidia NGC](https://catalog.ngc.nvidia.com/orgs/nvidia/teams/tao/models/peoplesegnet)
- at the top of the file is a link to a Nvidia repo that provides base configuration as a reference!
