#include "Pipeline.h"

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
  this->processor->set_up();
  if (!this->_setup_pipeline_bus())
    return false;
  if(!this->_create_pipeline())
    return false;
  return true;
}


bool Pipeline::_create_pipeline()
{
  // use configs to set pipeline


  // create sourceBins
  GstElement *srcBin = pipelineUtils::createMp4SrcBin("src0", "/src/sample_videos/head-pose-face-detection-female.mp4");
//  GstElement *srcRight = pipelineUtils::createMp4SrcBin("srcRight", "/src/sample_videos/head-pose-face-detection-female.mp4");

  // create inferenceBin
  GstElement *inferenceBin = pipelineUtils::createInferenceBinToVideoDisplay("inferenceBin", "sink_", 1);

  gst_bin_add_many(GST_BIN(this->pipeline), srcBin, inferenceBin, NULL);
  // Add callbacks
  GstElement *cb_element = gst_bin_get_by_name(GST_BIN(inferenceBin), "nv_tracker");
  if(cb_element == NULL)
    LOG(FATAL) << "Could not find nv_tracker in inferenceBin";

  GstPad *probe_pad = gst_element_get_static_pad(cb_element, "src");
  if(!gst_pad_add_probe(probe_pad, GST_PAD_PROBE_TYPE_BUFFER, core::GstCallbacks::probe_callback, (gpointer)this->processor, NULL))
    LOG(FATAL) << "Could not add pad probe to nv_tracker";
  gst_object_unref(probe_pad);

  // set element state to READY
  gst_element_set_state(GST_ELEMENT(this->pipeline), GST_STATE_NULL);
  // create picture diagram of the pipeline in its current state
  pipelineUtils::save_debug_dot(this->pipeline, "/src/logs", "NULL");

  // link the srcLeft to disparityLeft and inferenceBin
  GstPad* srcPad = gst_element_get_static_pad(srcBin, "output0");
  GstPad* inferenceBinPad = gst_element_get_static_pad(inferenceBin, "input0");
  if(srcPad == NULL)
    LOG(FATAL) << "Could not get ghostPad from srcPad (output0)";
  if(inferenceBinPad == NULL)
    LOG(FATAL) << "Could not get ghostPad from inferenceBin (input0)";

  int ret = gst_pad_link (srcPad, inferenceBinPad);
  if (ret != GST_PAD_LINK_OK)
  {
    LOG(ERROR) << "LINK ERROR:\t" << pipelineUtils::get_link_status(ret);
    LOG(FATAL) << "Could not link srcPad to inferenceBinPad";
  }
  gst_object_unref (srcPad);
  gst_object_unref (inferenceBinPad);

  // set element state to READY
  gst_element_set_state(GST_ELEMENT(this->pipeline), GST_STATE_READY);
  // create picture diagram of the pipeline in its current state
  pipelineUtils::save_debug_dot(this->pipeline, "/src/logs", "NULL_READY");

  return true;
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
    njson src_list = conf["sources"];
    this->_configs = (PipelineConfigs){
        .source_count = (unsigned int) conf["sources"].size(),
        .sources = njson::array(),
        .inference = conf["inference"],
        .tracker = conf["tracker"]
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
  this->pipeline = gst_pipeline_new("video-player");
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

  if (!this->_set_up()) {
    LOG(WARNING) << "Could not set up the pipeline module";
    throw -1;
  }

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