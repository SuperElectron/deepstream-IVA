#include "Processing.h"

#define DISPLAY_RED 0
#define DISPLAY_GREEN 0
#define DISPLAY_BLUE 255

using namespace core;

core::Processing::Processing()
{
  this->_module_id = core::Module::GST_PROCESSOR;
  LOG(INFO) << "CREATED: " << *this << " with ID=" << core::Module::GST_PROCESSOR;
}

core::Processing::~Processing() {}

/**
 * @brief loads modules settings from config.json
 *
 * @return bool true if successful
 */
bool Processing::set_configs(njson conf)
{
  try {

    if(!conf["topic"].is_string()){
      LOG(WARNING) << "Invalid config.json element! processing['topic'] must be a string";
      return false;
    }
    if(!conf["device_id"].is_string()) {
      LOG(WARNING) << "Invalid config.json element! processing['device_id'] must be a string";
      return false;
    }
    if(!conf["model"].is_string()) {
      LOG(WARNING) << "Invalid config.json element! processing['model'] must be a string";
      return false;
    }
    if(!conf["model_type"].is_string()){
      LOG(WARNING) << "Invalid config.json element! processing['model_type'] must be a string";
      return false;
    }
    if(!conf["publish"].is_boolean()){
      LOG(WARNING) << "Invalid config.json element! processing['publish'] must be a boolean";
      return false;
    }
    if(!conf["save"].is_boolean()){
      LOG(WARNING) << "Invalid config.json element! processing['save'] must be a boolean";
      return false;
    }
    if(!conf["display_detections"].is_boolean()){
      LOG(WARNING) << "Invalid config.json element! processing['display_detections'] must be a boolean";
      return false;
    }
    if(!conf["bbox_line_thickness"].is_number_integer()){
      LOG(WARNING) << "Invalid config.json element! processing['bbox_line_thickness'] must be an integer";
      return false;
    }
    if(!conf["min_confidence_to_display"].is_number_integer()){
      LOG(WARNING) << "Invalid config.json element! processing['min_confidence_to_display'] must be an integer";
      return false;
    }
    if(!conf["font_size"].is_number_integer()){
      LOG(WARNING) << "Invalid config.json element! processing['font_size'] must be an integer";
      return false;
    }

    core::ProcessingSettings configs = {
        .topic = conf["topic"],
        .device_id = conf["device_id"],
        .model = conf["model"],
        .model_type = conf["model_type"],
        .publish = conf["publish"],
        .save = conf["save"],
        .display_detections = conf["display_detections"],
        .bbox_line_thickness = conf["bbox_line_thickness"],
        .min_confidence_to_display = conf["min_confidence_to_display"],
        .font_size = conf["font_size"],
    };
    this->_configs = configs;
    VLOG(DEBUG) << "Processing configs: " << conf.dump(4);
  }
  catch (const std::exception &e) {
    LOG(ERROR) << "Error setting Processing configs: " << e.what();
    return false;
  }

  return true;
}

/**
 * @brief creates a data structure for each stream (i.e. each video source) for use in callbacks
 *
 */
void core::Processing::set_up(int source_count)
{
  this->_processor = (core::VideoSourceData){
      .lock = new std::mutex(),
      .meta_queue = new std::queue<njson>()};

  // instantiate callback data (for each stream)
  this->_display_lock.lock();
  this->_display_queue.resize(source_count);
  for (int q=0; q< source_count; q++)
  {
    this->_display_queue[q] = new std::queue<njson>();
  }
  this->_display_lock.unlock();

  LOG(INFO) << "Processing set up for source=(" << this->_display_queue.size() << ")";
}


/// PROCESSING CALLBACKS TO UNPACK GSTREAMER BUFFER

/**
 * @brief extracts metadata from src pad of NvInfer, NvTracker, or NvDsOSD elements and creates a kafka payload with its items
 * @copydoc configure the application config (/src/configs/config.json) fields processing['save'] to save images and processing['publish'] to publish results
 * with kafka
 * @param *info the GstBuffer wrapped when taken from Probe callback on a pad
 */
bool core::Processing::probe_callback(GstPad *pad, GstPadProbeInfo *info)
{

  GstVideoInfo video_info;
  GstCaps *caps = gst_pad_get_current_caps(pad);
  if (!gst_video_info_from_caps(&video_info, caps)) {
    LOG(FATAL) << "[probe_callback] Could not get caps from pad";
    gst_caps_unref(caps);
    return GST_PAD_PROBE_OK;
  }
  // extract parameters from the image, then clean up references
  gint width = GST_VIDEO_INFO_WIDTH(&video_info);
  gint height = GST_VIDEO_INFO_HEIGHT(&video_info);
  GstVideoFormat format = GST_VIDEO_INFO_FORMAT(&video_info);
  std::string video_format = (std::string) gst_video_format_to_string(format);
  gst_caps_unref(caps);
  VLOG(DEBUG) << "VIDEO_CAPS (video_format=" << video_format << ",width=" << width << ",height=" << height << ")";

  GstBuffer *buf;
  GstMapInfo map;
  try {
    buf = (GstBuffer *)info->data;
    memset(&map, 0, sizeof(map));
    /* Map the buffer contents and get the pointer to NvBufSurface. */
    if (!gst_buffer_map(GST_BUFFER(info->data), &map, GST_MAP_READ)){
      LOG(ERROR) << "[probe_callback] Error Failed to map gst buffer. Skipping";
      return false;
    }
  } catch (const std::exception &e) {
    LOG(ERROR) << "[probe_callback] Error: " << e.what();
    return false;
  }

  NvDsMetaList *frame_list = NULL;
  NvDsMetaList *object_list = NULL;

  // EXTRACT FRAMES: deconstruct the NvDsFrameMeta
  NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);

  // loop over sources (a batch_meta exists for each video source)
  for (frame_list = batch_meta->frame_meta_list; frame_list != NULL; frame_list = frame_list->next)
  {
    njson payload;
    NvDsFrameMeta *frame_meta = (NvDsFrameMeta *)(frame_list->data);

    // save meta information for all objects detected
    payload["topic"] = this->_configs.topic;
    payload["meta"]["device_id"] = this->_configs.device_id;
    payload["meta"]["frame"] = frame_meta->frame_num;
    payload["meta"]["utc"] = processUtils::generate_ts_epoch();
    payload["meta"]["timestamp"] = processUtils::generate_timestamp();
    payload["meta"]["model"] = this->_configs.model;
    payload["meta"]["detection_type"] = this->_configs.model_type;
    payload["meta"]["uuid"] = processUtils::generate_uuid();
    payload["meta"]["resolution"]["height"] = height;
    payload["meta"]["resolution"]["width"] = width;

    // loop through detected objects
    int objects_detected = 0;
    for (object_list = frame_meta->obj_meta_list; object_list != NULL; object_list = object_list->next)
    {
      // cast data and add inference objects to payload
      NvDsObjectMeta *obj_meta = (NvDsObjectMeta *)(object_list->data);
      NvDsComp_BboxInfo tracker_bbox_info = obj_meta->tracker_bbox_info;
      NvBbox_Coords tracker_boxes = tracker_bbox_info.org_bbox_coords;

      /*
       * convert from left, top, width, height
       *  where the coordinates start from top left, and go to bottom right (0,0) --> (1,1)
       *  for a plane where (x,y) ranges from {x: 0->1, y: 0-1} as shown below.
       *
       *  (0,0)               (1,0)
       *    __________________
       *   |                  |
       *   |  P1       P2     |           xmin=LEFT, xmax = LEFT+WIDTH
       *   |    _______       |           ymin=TOP , ymax = TOP+HEIGHT
       *   |   |       |      |
       *   |   |_______|      |           P1=(xmin, ymin) = (LEFT, TOP)
       *   |  P3       P4     |           P2=(xmax, ymin) = (LEFT+WIDTH, TOP)
       *   |__________________|           P3=(xmin, ymax) = (LEFT, TOP + HEIGHT)
       *                                  P3=(xmax, ymax) = (LEFT+WIDTH, TOP + HEIGHT)
       *
       *  (0,1)                (1,1)
       */
      float xmax = tracker_boxes.left + tracker_boxes.width;
      float xmin = tracker_boxes.left;
      float ymax = tracker_boxes.top + tracker_boxes.height;
      float ymin = tracker_boxes.top;
      float confidence = obj_meta->confidence * 100;

      payload["inference"][objects_detected]["bbox"] = {{"x_max", (int)xmax}, {"x_min", (int)xmin}, {"y_max", (int)ymax}, {"y_min", (int)ymin}};
      payload["inference"][objects_detected]["confidence"] = (int) confidence;
      payload["inference"][objects_detected]["label"] = (std::string) obj_meta->obj_label;
      payload["inference"][objects_detected]["tracking_id"] = (int) obj_meta->object_id;
      payload["inference"][objects_detected]["camera_id"] = (int) frame_meta->source_id;
      objects_detected += 1;
    } // parse next detection for this streamId

    // if this source has inference detections, act on it
    if (payload.contains("inference"))
    {
      // send payload to kafka producer
      if (this->_configs.publish)
      {
        this->_add_meta_queue(payload);
        this->_create_kafka_publish_event();
      }

      // save detection data to json (for debugging)
      if (this->_configs.save) {
        std::stringstream ss;
        ss << "/tmp/.cache/payload/frame_" << std::setw(4) << std::setfill('0') << payload["meta"]["frame"] << ".json";
        std::string file_name = ss.str();
        std::ofstream o(file_name.c_str());
        o << std::setw(4) << payload << std::endl;
      }

      // add data to display queue (which writes data onto the screen)
      if(this->_configs.display_detections)
      {
        this->_display_lock.lock();
        try {
          // LOG(INFO) << "[probe_callback] " << payload.dump(4);
          this->_display_queue[(int)frame_meta->source_id]->push(payload);
        } catch (const std::exception &e) {
          LOG(ERROR) << "Error adding to queue: " << e.what();
        }
        this->_display_lock.unlock();
      }
    }

  } // parse next stream_id
  gst_buffer_unmap(buf, &map);
  return true;
};


bool core::Processing::osd_callback(GstPad *pad, GstPadProbeInfo *info)
{

  // Get the parent object of the pad
  GstElement *parent_element = GST_ELEMENT(gst_pad_get_parent(pad));
  const gchar *parent_name = gst_element_get_name(parent_element);

  GstElement *bin_element = GST_ELEMENT(gst_element_get_parent(parent_element));
  std::string binName;
  if (GST_IS_BIN(bin_element)) {
    // The element belongs to a bin
    GstBin *bin = GST_BIN(bin_element);
    binName = (std::string) gst_element_get_name(GST_ELEMENT(bin));
    VLOG(DEBUG) << "BinName=" << binName;
  } else {
    LOG(FATAL) << "GST_IS_BIN(bin_element) is not true";
  }
  // get streamId from the last character in the bin: name schema={sink0, sink1, sinkN}
  int sourceStreamId = std::stoi(binName.substr(binName.length() - 1));
//  LOG(WARNING) << "The pad belongs to GStElement=" << parent_name << " and GstBin=" << binName << " (sourceStreamId=" << sourceStreamId << ")";

  // check if data is available on the queue
  njson detection;
  this->_display_lock.lock();
  int size = this->_display_queue[sourceStreamId]->size();
  if(size > 0)
  {
    detection = this->_display_queue[sourceStreamId]->front();
    this->_display_queue[sourceStreamId]->pop();
  } else {
    this->_display_lock.unlock();
    return true;
  }
  this->_display_lock.unlock();

  // error out if no payload is available
  if (detection.empty())
    LOG(FATAL) << "[!BUG!] Empty detection (invalid data) pulled from _display_queue";

  std::string video_format;
  int width, height;
  this->get_pad_video_caps(pad, video_format, width, height);
  VLOG(DEBUG) << "Bin=" << binName << " with video format=" << video_format << ",width=" << width << ",height=" << height;

  // extract the Buffer and operate on its video type
  GstBuffer *buf = (GstBuffer *)info->data;
  // Needed to get image width and height
  GstMapInfo map;
  memset(&map, 0, sizeof(map));
  /* Map the buffer contents and get the pointer to NvBufSurface. */
  if (!gst_buffer_map(GST_BUFFER(info->data), &map, GST_MAP_READWRITE)) {
    LOG(ERROR) << "Error Failed to map gst buffer. Skipping";
    return false;
  }

  if (video_format.compare("RGB") == 0) {
    VLOG(DEBUG) << "Detected RGB caps format=" << video_format;
    cv::Mat input_frame(cv::Size(width, height), CV_8UC3, (char *)map.data, cv::Mat::AUTO_STEP);
    this->_write_detections_to_image(input_frame, detection);

  }
  else if (video_format.compare("YV12") == 0) {
    /**
      * openCV type conversions
      * @reference: https://docs.opencv.org/3.4/d8/d01/group__imgproc__color__conversions.html
      *
     */
    VLOG(DEBUG) << "Detected YV12 caps format=" << video_format;
    cv::Mat input_frame(height + height / 2, width, CV_8UC1, (char *)map.data, cv::Mat::AUTO_STEP);
    cv::Mat bgr_frame(width, height, CV_8UC3);
    cv::cvtColor(input_frame, bgr_frame, cv::COLOR_YUV2BGR_YV12, 3);
    this->_write_detections_to_image(bgr_frame, detection);
    cv::cvtColor(bgr_frame, input_frame, cv::COLOR_BGR2YUV_YV12, 3);
  }
  else {
    LOG(FATAL) << "Detected caps that we cannot convert for overlay writing:" << video_format;
  }
  gst_buffer_unmap(buf, &map);
  return true;
}


void core::Processing::get_pad_video_caps(GstPad *pad, std::string &video_format, int &width, int &height)
{
  // get format (e.g. video/x-raw) from GstPad
  GstCaps *caps = gst_pad_get_current_caps(pad);
  if (!caps)
    caps = gst_pad_query_caps(pad, NULL);

  GstStructure *s = gst_caps_get_structure(caps, 0);
  bool res = gst_structure_get_int(s, "width", &width);
  res |= gst_structure_get_int(s, "height", &height);
  video_format = (std::string)gst_structure_get_string(s, "format");
}

/**
 * @brief write payload bounding box + text onto the image
 *
 * @param frame the video frame converted into cv::Mat from Gstreamer buffer
 */
void core::Processing::_write_detections_to_image(cv::Mat frame, njson payload)
{

  if (!payload.contains("inference"))
    LOG(FATAL) << "[_write_detections_to_image] Payload entered function when it shouldn't!";

  njson detection = payload["inference"];
  // LOG(INFO) << "[_write_detections_to_image] " << detection.dump(4);

  for (int d = 0; d < detection.size(); d++)
  {
    int tracking_id, confidence, xmin, ymin, xmax, ymax;
    std::string label, description;
    try
    {
      // set the text to display
      tracking_id = detection[d]["tracking_id"].get<int>();
      label = detection[d]["label"].get<std::string>();
      confidence = detection[d]["confidence"].get<int>();
      xmin = (int)detection[d]["bbox"]["x_min"].get<int>();
      ymin = (int)detection[d]["bbox"]["y_min"].get<int>();
      xmax = (int)detection[d]["bbox"]["x_max"].get<int>();
      ymax = (int)detection[d]["bbox"]["y_max"].get<int>();
    } catch (const std::exception &e) {
      LOG(FATAL) << "[_write_detections_to_image] NJSON FAIL: " << e.what();
    }

    description = label + (std::string) "[" + std::to_string(tracking_id) + "]" +
                  (std::string) " % " + std::to_string(confidence);

    // if detected confidence is greater than out desired confidence to display, write bbox on the image with text
    if(confidence > this->_configs.min_confidence_to_display)
    {
      cv::rectangle(
          frame,
          cv::Point(xmin, ymin),
          cv::Point(xmax, ymax),
          cv::Scalar(DISPLAY_RED, DISPLAY_GREEN, DISPLAY_BLUE),
          this->_configs.bbox_line_thickness,
          cv::LINE_8
      );

      // creating text to go above bounding box
      cv::Point text_position(xmin, ymin - 20);
      cv::putText(
          frame,
          description.c_str(),
          text_position,
          cv::FONT_HERSHEY_COMPLEX,
          this->_configs.font_size,
          CV_RGB(DISPLAY_RED, DISPLAY_GREEN, DISPLAY_BLUE),
          this->_configs.bbox_line_thickness,
          cv::LINE_AA
      );
    }

  }

}

/// MANAGING DATA FLOW

/**
 * @brief   add data to the queue for overlay updates to video frames
 *
 * @param   payload   njson payload to add to the queue (from callback)
 *
 */
void core::Processing::_add_meta_queue(njson payload)
{
  // safely access struct to push data into output_queue
  this->_processor.lock->lock();
  this->_processor.meta_queue->push(payload);
  this->_processor.lock->unlock();
}

/**
 * @brief   check if queue has metadata to write to overlay
 *
 * @return  bool        true if data is available
 */
bool core::Processing::check_meta_queue()
{
  this->_processor.lock->lock();
  int size = this->_processor.meta_queue->size();
  this->_processor.lock->unlock();
  if (size > 0)
    return true;
  else
    return false;
}

/**
 * @brief   get the first payload object detection metadata queue (used for updating overlay with bbox info)
 * @return  njson   first payload on the queue
 */
njson core::Processing::get_meta_queue()
{
  njson payload;

  // safely pull from stream struct
  this->_processor.lock->lock();
  payload = this->_processor.meta_queue->front();
  this->_processor.meta_queue->pop();
  this->_processor.lock->unlock();

  // error out if no payload is available
  if (payload.empty()) {
    LOG(ERROR) << "[!BUG!] Empty payload (invalid data) pulled from _processor.meta_queue";
    throw std::runtime_error("Empty payload on meta queue");
  }
  return payload;
}

/// PROCESSING EVENTS (used in mediator.cpp)

/**
 * @brief   tell the mediator that payload is ready to send to kafka producer
 *
 */
void core::Processing::_create_kafka_publish_event()
{
  // save stream_id to avoid loosing value from function chaining
  guint s_id = 0;
  // simple event to print pipeline name
  KafkaEvent *event = new KafkaEvent(events::Actions::KAFKA_PRODUCE_PAYLOAD, s_id, this->_module_id);
  this->_mediator->notify(event);
}