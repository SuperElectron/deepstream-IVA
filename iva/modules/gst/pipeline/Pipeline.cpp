#include "Pipeline.h"
#include "yamlParser.hpp"


using namespace core;

Pipeline::Pipeline()
{
  this->_module_id = core::Module::GST_PIPELINE;
  LOG(INFO) << "CREATED: " << *this;
}

Pipeline::~Pipeline() {}

/**
 * @brief set up the class to it can run a gstreamer pipeline
 * @return
 */
bool Pipeline::_set_up()
{
  this->processor->set_up(this->_configs.source_count);
  if (!this->_setup_pipeline_bus())
    return false;

  bool ret = false;
  // check if using yaml builder or production builder
  if(this->_configs.src_type == "v4l2src" || this->_configs.src_type == "mp4") {
    LOG(INFO) << "Detected pipeline.type=(v4l2src, file)=" << this->_configs.src_type;
    ret = this->_create_pipeline();
  } else if (this->_configs.src_type == "yaml") {
    LOG(INFO) << "Detected pipeline.type=(yaml)=" << this->_configs.src_type;
    ret = this->_create_pipeline_from_yaml(this->_configs.configs);
  } else {
    LOG(ERROR) << "Invalid config.json for pipeline. Unknown type=" << this->_configs.src_type;
  }
  return ret;
}

/**
 * @brief set module configurations via mediator event
 * @param config_path read this json file
 * @return bool true is success
 */
bool Pipeline::set_configs(njson conf)
{
  if (conf.empty()) {
    LOG(ERROR) << "config.json is empty!";
    return false;
  }

  try {
    // check that appropriate fields are included in the config.json file
    this->_configs = (PipelineConfigs){
        .configs = conf["configs"].get<std::string>(),
        .src_type = conf["src_type"].get<std::string>(),
        .sink_type = conf["sink_type"].get<std::string>(),
        .source_count = (int) conf["sources"].size(),
        .sources = conf["sources"],
        .sinks = conf["sinks"],
        .img_height = conf["input_height"].get<int>(),
        .img_width = conf["input_width"].get<int>(),
        .live_source = conf["live_source"].get<bool>(),
        .sync = conf["sync"].get<bool>()
    };
  } catch (const std::exception &e) {
    LOG(ERROR) << "Error setting Kafka configs: " << e.what();
    return false;
  }
  return true;
}

/**
 * @brief setup the pipeline and gstreamer bus callback (bus, bus_watch, bus callback)
 * @return true if good
 */
bool Pipeline::_setup_pipeline_bus()
{
  // Create gstreamer elements
  gst_init(NULL, NULL);
  this->pipeline = gst_pipeline_new("video-player0");
  if (!pipeline) {
    LOG(ERROR) << "Pipeline could not be created: [pipeline]. Exiting";
    return false;
  }
  this->bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  this->bus_struct = {.loop = this->loop, .timeout_counter = 0, .timeout_counter_max = 50};
  this->bus_watch_id = gst_bus_add_watch(this->bus, pipelineUtils::bus_call, (gpointer) & this->bus_struct);
  gst_object_unref(bus);
  return true;
}

/**
 * @brief create gstreamer pipeline
 * @return bool true if successful
 */
bool Pipeline::_create_pipeline()
{
  // use configs to create sourceBins and add them to the pipeline
  if (this->_configs.src_type.compare("mp4") == 0) {
    for(int b = 0; b < this->_configs.source_count; b++) {
      std::string src_name = (std::string) "srcBin" + std::to_string(b);
      std::string file_src = this->_configs.sources[b];
      LOG(INFO) << "src_name=" << src_name << ", file_src=" << file_src;
      GstElement *srcBin = pipelineUtils::createMp4SrcBin(src_name, file_src);
      if(!gst_bin_add(GST_BIN(this->pipeline), srcBin))
      {
        LOG(ERROR) << "Failed to add srcBin[" << b << "] to pipeline";
        return false;
      }
    }
  } else if (this->_configs.src_type.compare("rtsp") == 0) {
    for(int b = 0; b < this->_configs.source_count; b++) {
      std::string src_name = (std::string) "srcBin" + std::to_string(b);
      std::string uri = this->_configs.sources[b];
      LOG(INFO) << "src_name=" << src_name << ", uri=" << uri;
      GstElement *srcBin = pipelineUtils::createRtspSrcBin(src_name, uri);
      if(!gst_bin_add(GST_BIN(this->pipeline), srcBin))
      {
        LOG(ERROR) << "Failed to add srcBin[" << b << "] to pipeline";
        return false;
      }
    }
  } else if (this->_configs.src_type.compare("v4l2") == 0) {
    for(int b = 0; b < this->_configs.source_count; b++) {
      std::string src_name = (std::string) "srcBin" + std::to_string(b);
      std::string deviceId = this->_configs.sources[b];
      LOG(INFO) << "src_name=" << src_name << ", deviceId=" << deviceId;
      GstElement *srcBin = pipelineUtils::createV4l2SrcBin(src_name, deviceId);
      if(!gst_bin_add(GST_BIN(this->pipeline), srcBin))
      {
        LOG(ERROR) << "Failed to add srcBin[" << b << "] to pipeline";
        return false;
      }
    }
  } else {
    LOG(FATAL) << "Type of source has not been configured";
  }

  // create inferenceBin and add it to the pipeline
  GstElement *inferenceBin = pipelineUtils::createInferenceBinToStreamDemux("inferenceBin", this->_configs.source_count, this->_configs.img_width, this->_configs.img_height, this->_configs.live_source);
  if(!gst_bin_add(GST_BIN(this->pipeline), inferenceBin))
  {
    LOG(ERROR) << "Failed to add inferenceBin to pipeline";
    return false;
  }

  // create sink bin
  if (this->_configs.sink_type.compare("display") == 0)
  {
    for (int b=0; b < this->_configs.source_count; b++) {
      std::string binName = (std::string) "sinkBin" + std::to_string(b);
      GstElement* sinkBin = pipelineUtils::createSinkBinToDisplay(binName, this->_configs.sync);
      if(!gst_bin_add(GST_BIN(this->pipeline), sinkBin))
      {
        LOG(ERROR) << "Failed to add sinkBin[" << b << "] to pipeline";
        return false;
      }

      // add callbacks to display bounding boxes
      GstElement *cb_element = gst_bin_get_by_name(GST_BIN(sinkBin), "sink_caps");
      if(cb_element == NULL)
        LOG(FATAL) << "Could not find sink_caps in sinkBin(" << b << ")";

      GstPad *probe_pad = gst_element_get_static_pad(cb_element, "src");
      if(!gst_pad_add_probe(probe_pad, GST_PAD_PROBE_TYPE_BUFFER, core::GstCallbacks::osd_callback, (gpointer)this->processor, NULL))
        LOG(FATAL) << "Could not add pad probe to sink_caps";
      gst_object_unref(probe_pad);
    }
  } else if (this->_configs.sink_type.compare("rtmp") == 0) {
    for (int b=0; b < this->_configs.source_count; b++) {
      std::string binName = (std::string) "sinkBin" + std::to_string(b);
      GstElement* sinkBin = pipelineUtils::createSinkBinToRTMP(binName, this->_configs.sinks[b], this->_configs.sync);
      if(!gst_bin_add(GST_BIN(this->pipeline), sinkBin))
      {
        LOG(ERROR) << "Failed to add sinkBin[" << b << "] to pipeline";
        return false;
      }

      // add callbacks to display bounding boxes
      GstElement *cb_element = gst_bin_get_by_name(GST_BIN(sinkBin), "sink_caps");
      if(cb_element == NULL)
        LOG(FATAL) << "Could not find sink_caps in sinkBin(" << b << ")";

      GstPad *probe_pad = gst_element_get_static_pad(cb_element, "src");
      if(!gst_pad_add_probe(probe_pad, GST_PAD_PROBE_TYPE_BUFFER, core::GstCallbacks::osd_callback, (gpointer)this->processor, NULL))
        LOG(FATAL) << "Could not add pad probe to sink_caps";
      gst_object_unref(probe_pad);
    }
  }
  else {
    LOG(FATAL) << "Invalid sink type";
  }

  // Add callbacks
  GstElement *cb_element = gst_bin_get_by_name(GST_BIN(inferenceBin), "nv_tracker");
  if(cb_element == NULL)
    LOG(FATAL) << "Could not find nv_tracker in inferenceBin";
  GstPad *probe_pad = gst_element_get_static_pad(cb_element, "src");
  if(!gst_pad_add_probe(probe_pad, GST_PAD_PROBE_TYPE_BUFFER, core::GstCallbacks::probe_callback, (gpointer)this->processor, NULL))
    LOG(FATAL) << "Could not add pad probe to nv_tracker";
  gst_object_unref(probe_pad);

  // set element state to NULL and save diagram
  gst_element_set_state(GST_ELEMENT(this->pipeline), GST_STATE_NULL);
  // create picture diagram of the pipeline in its current state
  pipelineUtils::save_debug_dot(this->pipeline, "/src/logs", "NULL");

  // link the source bins to inference bin
  for (int b=0; b<this->_configs.source_count; b++)
  {
    std::string src_name = (std::string) "srcBin" + std::to_string(b);
    GstElement *srcBin = gst_bin_get_by_name(GST_BIN(this->pipeline), src_name.c_str());
    GstPad* srcPad = gst_element_get_static_pad(srcBin, "output0");
    std::string inferenceBinPadName = (std::string) "input" + std::to_string(b);
    GstPad* inferenceBinPad = gst_element_get_static_pad(inferenceBin, inferenceBinPadName.c_str());
    if(srcPad == NULL)
      LOG(FATAL) << "Could not get ghostPad from bin=" << src_name << " ,pad=output0";
    if(inferenceBinPad == NULL)
      LOG(FATAL) << "Could not get ghostPad from inferenceBin (pad=" << inferenceBinPadName << ")";

    int ret = gst_pad_link (srcPad, inferenceBinPad);
    if (ret != GST_PAD_LINK_OK)
    {
      LOG(ERROR) << "LINK ERROR:\t" << pipelineUtils::get_link_status(ret);
      LOG(FATAL) << "Could not link srcPad to inferenceBinPad";
    }
    gst_object_unref (srcPad);
    gst_object_unref (inferenceBinPad);
  }

  // link the inferenceBin to sinkBins
  for (int b=0; b<this->_configs.source_count; b++)
  {
    // get the sinkBin and its input pad
    std::string bin_name = (std::string) "sinkBin" + std::to_string(b);
    GstElement *sinkBin = gst_bin_get_by_name(GST_BIN(this->pipeline), bin_name.c_str());
    GstPad* sinkBinPad = gst_element_get_static_pad(sinkBin, "input0");

    // get output pad from the inference bin
    std::string inferenceBinPadName = (std::string) "output" + std::to_string(b);
    GstPad* inferenceBinPad = gst_element_get_static_pad(inferenceBin, inferenceBinPadName.c_str());

    if(sinkBinPad == NULL)
      LOG(FATAL) << "Could not get ghostPad from bin=" << bin_name << " ,pad=input0";
    if(inferenceBinPad == NULL)
      LOG(FATAL) << "Could not get ghostPad from inferenceBin (pad=" << inferenceBinPadName << ")";

    int ret = gst_pad_link (inferenceBinPad, sinkBinPad);
    if (ret != GST_PAD_LINK_OK)
    {
      LOG(ERROR) << "LINK ERROR:\t" << pipelineUtils::get_link_status(ret);
      LOG(FATAL) << "Could not link inferenceBinPad to sinkBinPad";
    }
    gst_object_unref (inferenceBinPad);
    gst_object_unref (sinkBinPad);
  }
  // set element state to READY
  gst_element_set_state(GST_ELEMENT(this->pipeline), GST_STATE_READY);
  // create picture diagram of the pipeline in its current state
  pipelineUtils::save_debug_dot(this->pipeline, "/src/logs", "NULL_READY");

  return true;
}


/**
 * @brief parse a yaml file and create gstreamer pipeline elements
 * @param str std::string path to the iou_file_display.yml or config.yaml file
 * @return bool true is success
 */
bool Pipeline::_create_pipeline_from_yaml(std::string file_path)
{
  // Load the YAML file
  YAML::Node config = YAML::LoadFile(file_path.c_str());
  std::string last_element;

  // configure multiple sources if necessary, and keep track of their element properties
  int bins = 1;
  std::vector<std::pair<std::string, std::string>> property_vect;
  if (config["sources"]) {
    bins = config["sources"]["bins"].as<int>();
    if(config["sources"]["property"])
    {
      // loop through all entries under properties, and split into key,value pairs
      for (const auto &property : config["sources"]["property"]) {
        std::string prop;
        try {
          prop = property.as<std::string>();
        }
        catch (const std::exception &e) {
          LOG(ERROR) << "ERROR getting YAML field from property=" << property << ": ErrMsg=" << e.what();
          return false;
        }
        // Split the fruit item by "=" sign
        std::istringstream iss(prop);
        std::string key, value;
        std::getline(iss, key, '=');
        std::getline(iss, value);
        property_vect.push_back({key, value});
      }
    }
    else {
      LOG(ERROR) << "Invalid yaml file.  Do not include the sources key if you do not include properties";
      return false;
    }
  }
  LOG(INFO) << "Setting up pipeline source bins=(" << bins << ")";


  // Iterate over the 'source' elements in the YAML file
  for (const auto &element : config["source"]) {
    std::string element_name, name;
    try {
      element_name = element["name"].as<std::string>();
      name = element["alias"].as<std::string>();
    } catch (const std::exception &e) {
      LOG(ERROR) << "ERROR getting YAML field from 'element.name' or 'element.alias' ErrMsg=" << e.what();
      return false;
    }

    VLOG(DEBUG) << "Creating Element: " << element_name << " : " << name;
    GstElement *new_element = gst_element_factory_make(element_name.c_str(), name.c_str());
    if(!gst_bin_add(GST_BIN(this->pipeline), new_element))
    {
      LOG(ERROR) << "Could not add element to bin: name=" << element_name << ", alias=" << name;
      return false;
    }

    // set gstreamer properties for this element
    if(!yamlParser::set_element_properties(new_element, element["property"]))
      return false;

    // Iterate over the key-value pairs in the properties section
    if(!yamlParser::link_pipeline_elements(this->pipeline, new_element, last_element, element, config))
      return false;

    if(!this->_set_callbacks(new_element, element))
      return false;

    // set the last element so that the next element can link to it
    last_element = name;
  }

  // set element state to READY
  gst_element_set_state(GST_ELEMENT(this->pipeline), GST_STATE_READY);
  // create picture diagram of the pipeline in its current state
  pipelineUtils::save_debug_dot(this->pipeline, "/src/logs", "NULL_READY");
  return true;
}

/**
 * @brief set callback functions for pipeline elements
 * @param new_element the new gstreamer element that was created
 * @param element the element field of the YAML file
 * @return true if successful
 */
bool Pipeline::_set_callbacks(GstElement *new_element, YAML::Node element)
{
  std::string name;
  try {
    name = element["alias"].as<std::string>();
  }
  catch (const std::exception &e) {
    LOG(ERROR) << "ERROR getting YAML field from 'element.alias' or ErrMsg=" << e.what();
    return false;
  }
  if (element["callback"]) {
    std::string callback_type = element["callback"]["type"].as<std::string>();
    LOG(INFO) << "Setting up callback( type= " << callback_type << " on element=" << name << ")";

    // set up for a probe (used for getting access to Gstreamer buffer) or signal (custom callback when element signal is emitted)
    if (callback_type == "probe") {
      if (!element["callback"]["pad"] || !element["callback"]["function_name"]) {
        LOG(ERROR) << "Expects fields `pad` and `function_name` for callback.type=probe";
        return false;
      }

      std::string pad_name = element["callback"]["pad"].as<std::string>();
      std::string function_name = element["callback"]["function_name"].as<std::string>();
      VLOG(DEBUG) << "\t callback type= " << callback_type << ", pad_name=" << pad_name << ",function_name=" << function_name;
      if (function_name == "probe_callback") {
        // set callbacks on element probes
        GstPad *probe_pad = gst_element_get_static_pad(new_element, pad_name.c_str());
        gst_pad_add_probe(probe_pad, GST_PAD_PROBE_TYPE_BUFFER, core::GstCallbacks::probe_callback, (gpointer)this->processor, NULL);
        gst_object_unref(probe_pad);
      }
      else if (function_name == "osd_callback") {
        // set callbacks on element probes
        GstPad *probe_pad = gst_element_get_static_pad(new_element, pad_name.c_str());
        gst_pad_add_probe(probe_pad, GST_PAD_PROBE_TYPE_BUFFER, core::GstCallbacks::osd_callback, (gpointer)this->processor, NULL);
        gst_object_unref(probe_pad);
      }
      else {
        LOG(ERROR) << "Callback field `function_name` is not configured: " << function_name << "\t callback type= " << callback_type
                   << ", pad_name=" << pad_name << ",function_name=" << function_name;
        return false;
      }
    }
    else if (callback_type == "signal") {
      if (!element["callback"]["element_signal"] || !element["callback"]["function_name"]) {
        LOG(ERROR) << "Expects fields `element_signal` and `function_name` for callback.type=signal";
        return false;
      }
      std::string element_signal = element["callback"]["element_signal"].as<std::string>();
      std::string function_name = element["callback"]["function_name"].as<std::string>();
      VLOG(DEBUG) << "\t callback type= " << callback_type << ", element_signal=" << element_signal << ",function_name=" << function_name;
      if (function_name == "on_pad_added") {
        g_signal_connect(new_element, "pad-added", G_CALLBACK(pipelineUtils::on_pad_added), (gpointer)this->pipeline);
      }
      else {
        LOG(ERROR) << "Link type is not configured: " << function_name << "\t callback type= " << callback_type << ", element_signal=" << element_signal
                   << ",function_name=" << function_name;
        return false;
      }
    }
    else {
      LOG(ERROR) << "Invalid config field for callback ";
      return false;
    }
  }
  return true;
}


/**
 * @brief runs the main gstreamer thread, g_main_loop_run, which is the main thread on the pipeline's lifecycle
 *
 */
void Pipeline::_run_pipeline()
{
  /* Set the pipeline to "playing" state*/
  LOG(INFO) << "STARTING PIPELINE";
  VLOG(DEEP) << "[2]Reference count of pipeline: " << GST_OBJECT_REFCOUNT(this->pipeline);

  gst_element_set_state(GST_ELEMENT(this->pipeline), GST_STATE_PLAYING);
  pipelineUtils::save_debug_dot(this->pipeline, "/src/logs", "READY_PLAYING");
  /* Runs loop until completion */
  g_main_loop_run(this->loop);

  /* Out of the main loop, clean up nicely */
  LOG(INFO) << "FINISHED PIPELINE";
  gst_element_set_state(GST_ELEMENT(this->pipeline), GST_STATE_NULL);
  pipelineUtils::save_debug_dot(this->pipeline, "/src/logs", "PLAYING_NULL");

  gst_object_unref(GST_OBJECT(this->pipeline));
  g_source_remove(this->bus_watch_id);
  g_main_loop_unref(this->loop);

  LOG(INFO) << "Module finished ... notifying mediator to shut down";
  this->_pipeline_finished();
  return;
}

/**
 * @brief run pipeline threads
 */
void Pipeline::start()
{
  LOG(INFO) << "Starting module";

  if (!this->_set_up())
    LOG(FATAL) << "Could not set up the pipeline module";

  this->_pool.push_task(&Pipeline::_run_pipeline, this);
}

/**
 * EVENTS
 */

/**
 * @brief   tell the mediator that payload is ready to send to kafka producer
 *
 * @param stream_id     synonymous to camera id or source id, this describes the origin
 */
void Pipeline::_create_kafka_publish_event(const guint stream_id)
{
  // save stream_id to avoid loosing value from function chaining
  guint s_id = stream_id;
  // simple event to print pipeline name
  KafkaEvent *event = new KafkaEvent(events::Actions::KAFKA_PRODUCE_PAYLOAD, s_id, this->_module_id);
  this->_mediator->notify(event);
}

void Pipeline::_pipeline_finished()
{
  LOG(INFO) << "Stopping module";
  PipelineEvent *event = new PipelineEvent(core::events::Actions::STOP_MODULES, core::events::Module::MODULE_PIPELINE);
  this->_mediator->notify(event);
  LOG(WARNING) << "Pipeline is finished, send signal to close application is being sent";
  VLOG(DEEP) << "[3]Reference count of pipeline: " << GST_OBJECT_REFCOUNT(this->pipeline);
}