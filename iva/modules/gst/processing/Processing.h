#pragma once
#include <cuda.h>
#include <cuda_runtime.h>
#include <cuda_runtime_api.h>
#include <glib.h>
#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>
#include <gst/video/video.h>

#include "nvbufsurftransform.h"
#include <gstnvdsmeta.h>

#include <algorithm>
#include <array>
#include <map>
#include <mutex>
#include <queue>
#include <fstream>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>

#include "gstnvdsmeta.h"
#include "nvbufsurface.h"

#include "BaseComponent.h"
#include "Mediator.h"
#include "Event.h"
#include "errors.hpp"
#include "logging.hpp"
#include "processUtils.hpp"

using njson = nlohmann::json;


namespace core
{

// must forward declare for the following inheritance
class Mediator;

class BaseComponent;

class Event;


/**
 * @struct ProcessingSettings
 * @brief Processing module settings from config.json
 *
 * @var topic
 * description
 * @var device_id
 * description
 * @var model
 * description
 * @var model_type
 * description
 * @var publish
 * publish metadata to topic in save_metadata callback
 * @var save
 * save metadata in save_metadata callback
 * @var bbox_line_thickness
 * thickness of the bounding box when writing to cv::Mat
 * @var min_confidence_to_display
 * when writing to cv::Mat, do not display detections with confidence less than this
 * @var font_size
 * cv::Mat font size for descriptors above bounding box
 */
struct ProcessingSettings
{
    std::string topic;
    std::string device_id;
    std::string model;
    std::string model_type;
    bool publish;
    bool save;
    bool display_detections;
    int bbox_line_thickness;
    int min_confidence_to_display;
	int font_size;
};

struct CbStore
{
    std::mutex *lock;
    std::vector<std::queue<njson>*> *osd_data;
};



/**
 * @struct VideoSourceData
 * @brief Manages dataflow for callback probe
 *
 * @var lock
 * mutex lock to ensure safe access while changing information in this struct
 * @var frame_counter
 * number of frames that have been processed
 * @var output_queue
 * when publish=true in ProcessingSettings, data will be sent to producer via this queue
 * @var meta_queue
 * all processed metadata is added to this queue
 * @var input_queue
 * all data received from spyder (distance data) is added to this queue by the kafka consumer
 */
struct VideoSourceData
{
    // access protection between shared callbacks
    std::mutex *lock;
    // data to be sent off-board via kafka consumer
    std::queue <njson> *meta_queue;
};

/**
 * @class Processing
 * @brief derived from BaseComponent, and responsible for processing all callbacks in pipeline module
 *
 * @var _module_id
 * module id from BaseComponent.h
 * @var _kafka_health
 * displays WIFI on screen in green or red, depending on weather Kafka is connected to spyder
 * @var _calibration_status
 * displays calibration status on screen
 * @var _detection_type
 * displays detection type on screen based on the payload from spyder
 */
class Processing : public BaseComponent
{
public:
    Processing();

    // class setup
    void set_up(int source_count);

    /// PROCESSING METADATA
    bool probe_callback(GstPad *pad, GstPadProbeInfo *info);
    bool osd_callback(GstPad *pad, GstPadProbeInfo *info);
    void get_pad_video_caps(GstPad *pad, std::string &video_format, int &width, int &height);
    /// MANAGING DATA FLOW
    njson get_meta_queue();
    bool check_meta_queue();

    /// MODULE SETTINGS
    bool set_configs(njson);

    ~Processing();

private:
    // core::Modules::<module-id>=core::Modules::MODULE_PROCESSING
    int _module_id;

    /// write detection data onto screen
    void _write_detections_to_image(cv::Mat frame, njson detection);

    /// MANAGING DATA FLOW
    // processing container for each video source
    VideoSourceData _processor;
    std::mutex _display_lock = std::mutex();
    std::vector<std::queue<njson>*> _display_queue;

    void _add_meta_queue(njson payload);
    void _create_kafka_publish_event();

    /// MODULE SETTINGS
    ProcessingSettings _configs;
};
}  // namespace core
