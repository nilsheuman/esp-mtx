# ESP Gif Mtx

Renders GIFs to a pixelmatrix display of size 64x32

# Setup

Open the project with [platform.io](https://platformio.org/), then Upload and Monitor.

It sets up an access point with SSID `esp-pxmtx`:`password`, defined in [src/config.h](src/config.h).

Access web ui on `192.168.4.1` to configure a network.

The IP will be printed to serial and on the display once connected.

Then you can upload a GIF, it will be loaded into memory and drawn frame by frame.

# Development

The display updating timing is crucial so any blocking io will make it crash if having large images, the workaround was to pause the display routine while uploading and loading all frames to memory, and then be able to draw them faster (~5ms per frame).

To update the webpage, edit [resources/index.html](resources/index.html) and run the [scripts/gen_index.sh](scripts/gen_index.sh) to generate [src/index_html.h](src/index_html.h)

To simplify development with LLMs there is a script in [scripts/read_files.sh](scripts/read_files.sh) to extract all content with filenames. They however seem to be a bit clueless and destructive changing the code...

# Gif Resources

Rotate and crop
https://ezgif.com/

Small gifs
https://www.animatedimages.org/
