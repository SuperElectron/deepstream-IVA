import gi
import logging
from threading import Thread

gi.require_version('Gst', '1.0')
gi.require_version('GstRtspServer', '1.0')
gi.require_version('GstApp', '1.0')
from gi.repository import Gst, GstRtspServer, GLib

# Configure the logging module
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] - %(message)s',
    handlers=[logging.StreamHandler()]
)


USER_MESSAGE = "***\n " \
               "-- INSTRUCTIONS -- \n" \
               "1. Changing the video file that is played -- \n" \
               "You must mount a video to /tmp/sample_videos/video.mp4\n" \
               "docker run -it --name camera -v `pwd`/.cache/sample_videos:/tmp alphawise/camera  \n" \
               "2. VIEWING THE RTSP STREAM -- \n" \
               "Install ffmpeg: $ sudo apt-get install -y ffmpeg\n" \
               "View stream:    $ ffplay rtsp://172.23.0.2:8554/test\n" \
               "***\n"


class RTSPServer(GstRtspServer.RTSPServer):
    def __init__(self):
        super(RTSPServer, self).__init__()

        video_file = "/tmp/sample_videos/test.mp4"

        # Set the RTSP server properties
        self.set_service("8554")
        factory = GstRtspServer.RTSPMediaFactory()

        # Set the GStreamer pipeline string with filesrc element to stream a video file
        pipeline_str = f"(filesrc location={video_file} name=source ! decodebin ! videoconvert ! x264enc ! rtph264pay name=pay0 pt=96 )"
        factory.set_launch(pipeline_str)

        # Bind the RTSP server to all available interfaces
        factory.set_shared(True)

        # Attach the factory to the default media mapping
        self.get_mount_points().add_factory("/test", factory)
        # # Set a callback function for when the video reaches the end
        factory.connect("media-configure", self.on_media_configure)

    @staticmethod
    def on_media_configure(factory, media):
        logging.info("End of stream, restarting...")
        pipeline = media.get_element()
        pipeline.set_state(Gst.State.NULL)
        pipeline.set_state(Gst.State.READY)
        pipeline.seek_simple(Gst.Format.TIME, Gst.SeekFlags.FLUSH | Gst.SeekFlags.KEY_UNIT, 0)
        pipeline.set_state(Gst.State.PLAYING)

def main():
    # Initialize GStreamer
    Gst.init(None)

    # Create and run the RTSP server
    server = RTSPServer()
    thread = Thread(target=server.attach, args=(None,))
    thread.start()

    logging.info(USER_MESSAGE)

    try:
        # Run the GMainLoop to handle GStreamer events
        logging.info("Starting the main thread")
        loop = GLib.MainLoop()
        loop.run()
    except KeyboardInterrupt:
        logging.info("Terminating the thread")
    except Exception as err:
        logging.error(f"Caught runtime error: {err}")
    finally:
        # Clean up and stop the server
        thread.join()


if __name__ == "__main__":
    main()
