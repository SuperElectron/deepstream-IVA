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
    core::ProcessingSettings configs = {
        .topic = conf["topic"],
        .device_id = conf["device_id"],
        .model = conf["model"],
        .model_type = conf["model_type"],
        .publish = conf["publish"],
        .save = conf["save"],
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
  this->_cb_lock.lock();
  this->_cb_data.resize(source_count);
  for (int q=0; q< source_count; q++)
  {
    this->_cb_data[q] = new std::queue<njson>();
  }
  this->_cb_lock.unlock();

  LOG(INFO) << "Set up CBstore:  size=" << this->_cb_data.size();

  LOG(INFO) << "Setting up the cb_stores for sources=(" << source_count << ")";
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
  GstBuffer *buf = (GstBuffer *)info->data;
  // Needed to get image width and height
  GstMapInfo in_map_info;
  memset(&in_map_info, 0, sizeof(in_map_info));
  /* Map the buffer contents and get the pointer to NvBufSurface. */
  if (!gst_buffer_map(GST_BUFFER(info->data), &in_map_info, GST_MAP_READWRITE)) {
    LOG(ERROR) << "Error Failed to map gst buffer. Skipping";
    return false;
  }

  // get width, height and format (e.g. video/x-raw) from GstBuffer
  int width, height;
  GstCaps *caps = gst_pad_get_current_caps(pad);
  if (!caps)
    caps = gst_pad_query_caps(pad, NULL);
  GstStructure *s = gst_caps_get_structure(caps, 0);
  bool res = gst_structure_get_int(s, "width", &width);
  res |= gst_structure_get_int(s, "height", &height);
  std::string video_format = (std::string)gst_structure_get_string(s, "format");
  gst_caps_unref(caps);
  if (!res)
    LOG(FATAL) << "The pad doesn't have image dimensions!";
  VLOG(DEBUG) << "GST_EVENT_CAPS (video_format=" << video_format << ",width=" << width << ",height=" << height << ")";

  njson payload;

  NvDsMetaList *frame_list = NULL;
  NvDsMetaList *object_list = NULL;

  // EXTRACT FRAMES: deconstruct the NvDsFrameMeta
  NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);

  // loop over sources (a batch_meta exists for each video source)
  for (frame_list = batch_meta->frame_meta_list; frame_list != NULL; frame_list = frame_list->next)
  {
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
    for (object_list = frame_meta->obj_meta_list; object_list != NULL; object_list = object_list->next) {
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
      payload["inference"][objects_detected]["confidence"] = (int)confidence;
      payload["inference"][objects_detected]["label"] = obj_meta->obj_label;
      payload["inference"][objects_detected]["tracking_id"] = obj_meta->object_id;
      payload["inference"][objects_detected]["camera_id"] = frame_meta->source_id;

      try{
        this->_cb_data[(int)frame_meta->source_id]->push(payload);
      } catch (const std::exception &e) {
        LOG(ERROR) << "Error adding to queue: " << e.what();
      }

//      queue.push(payload);
//      this->_cb_data.osd_data[frame_meta->source_id].push(payload);
    }
    // parse next stream_id
  }
  VLOG(DEBUG) << "Detection njson: " << payload.dump(4);
  if (payload.contains("inference")) {
    if (this->_configs.publish)
    {
      this->_add_meta_queue(payload);
      this->_create_kafka_publish_event();
    }
    if (this->_configs.save) {
      std::stringstream ss;
      ss << "/tmp/.cache/payload/frame_" << std::setw(4) << std::setfill('0') << payload["meta"]["frame"] << ".json";
      std::string file_name = ss.str();
      std::ofstream o(file_name.c_str());
      o << std::setw(4) << payload << std::endl;
    }
  }
  return true;
};

/**
 * @brief write payload bounding box + text onto the image
 *
 * @param frame the video frame converted into cv::Mat from Gstreamer buffer
 */
void core::Processing::_write_detections_to_image(cv::Mat frame, njson detection, std::string text)
{
  // if detected confidence is greater than out desired confidence to display, write bbox on the image with text
  if(detection["confidence"] > this->_configs.min_confidence_to_display)
  {
    cv::rectangle(
        frame,
        cv::Point((int)detection["bbox"]["xmin"], (int)detection["bbox"]["ymin"]),
        cv::Point((int)detection["bbox"]["xmax"], (int)detection["bbox"]["ymax"]),
        cv::Scalar(DISPLAY_RED, DISPLAY_GREEN, DISPLAY_BLUE),
        this->_configs.bbox_line_thickness,
        cv::LINE_8
    );

    // creating text to go above bounding box
    cv::Point text_position((int)detection["bbox"]["xmin"], (int)detection["bbox"]["ymin"] - 20);
    cv::putText(
        frame,
        text.c_str(),
        text_position,
        cv::FONT_HERSHEY_COMPLEX,
        this->_configs.font_size,
        CV_RGB(DISPLAY_RED, DISPLAY_GREEN, DISPLAY_BLUE),
        this->_configs.bbox_line_thickness,
        cv::LINE_AA
    );
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