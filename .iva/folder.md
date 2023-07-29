
# folder

Describes the contents of the `.cache` project files

---


# Table of contents

1. [Overview](#Overview)
2. [Config](#Configs)
3. [Licenses](#Licenses)
4. [Sample Videos](#Sample-Videos)
5. [Saved Media](#Saved-Media)

---

<a name="Overview"></a>
## Overview


- All the project files for `iva` and `camera` are in the `.cache` directory, and is mounted to `/tmp/.cache` in the docker containers in the Makefile
- View the folder contents and notes
```bash
.cache
├── configs                       // iva configs
│       ├── config.json             // main iva configuration
│       └── model                   // model configs
│              ├── detection.yml    // describes primary inference
│              ├── nvds             // folder with inference config file 
│              └── tracker.yml      // describes object detection tracker
│
├── licenses                      // iva licenses
│       ├── license.dat             // project licenses
│       └── security.json           // company license 
├── sample_videos                 // video files mounted to docker containers (iva, camera)
│        ├── sample_720p.mp4         // add more videos as you please!
│        └── test.mp4                // camera docker container default video file
└── saved_media                   // location where any saved payloads are stored   
```


---

<a name="Configs"></a>
## Configs


---


<a name="Licenses"></a>
## Licenses

- do not touch `.cache/licenses`.
- these files are supplied by AlphaWise and enable the binary files to run.

---

<a name="Sample-Videos"></a>
## Sample-Videos

- the `camera` container uses `test.mp4`.  If you want to change which video the RTSP server is publishing, copy a file to `test.mp4` as shown below

```bash
# cd into the project directory where you have Makefile and .cache folder
cd /path/to/project
# copy video to test.mp4
cp /path/to/your/video.mp4 .cache/sample_videos/test.mp4
```

- you can add videos to `.cache/sample_videos` then use them in the `iva` container by referencing the `.cache/configs/config.json`.  Refer to #Configs for more information.

---

<a name="Saved-Media"></a>
## Saved-Media

- in `.cache/configs/config.json` if you choose `save` then the payload will be saved here for future analysis


<a name="Configs"></a>
## Configs

__config.json__

- When running `./iva` inside the container, the app is configured by the `.cache/configs/config.json`



