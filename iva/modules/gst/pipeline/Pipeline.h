#pragma once

#include <glib.h>
#include <gst/gst.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h>

#include <BS_thread_pool.hpp>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

// sleep
#include <chrono>

#include "BaseComponent.h"
#include "Processing.h"
#include "callbacks.hpp"
#include "pipelineUtils.hpp"

using namespace pipelineUtils;

namespace core {

// must forward declare for the following inheritance
class BaseComponent;

class Mediator;
class Processing;
class Event;

struct PipelineConfigs {
  std::string src_type;
  std::string sink_type;
  int source_count = 1;
  njson sources;
  njson sinks;
  int img_height;
  int img_width;
  bool live_source;
  bool sync;
};

/**
 * @class Pipeline
 * @brief derived from BaseComponent and responsible for running Gstreamer pipeline
 *
 * @var processor
 * the Processing module that accesses all callbacks to perform processing operations
 * @var _configs
 * the modules config.yml that describes the gstreamer pipeline
 * @var loop
 * the loop (or thread) that runs the gstreamer pipeline
 * @var pipeline
 * the gstreamer pipeline
 * @var bus
 * a watcher to catch all messages sent from elements to the application bus
 * @var bus_watch_id
 * unique id for this pipeline's bus
 * @var bus_struct
 * data passed to the callback on the bus (updates application data when bus messages occur)
 * @var _pool
 * a thead pool
 */
class Pipeline : public BaseComponent {
 public:
  Pipeline();

  void start();
  bool set_configs(njson conf);

  // create this->_store
  core::Processing *processor = new Processing();
  ~Pipeline();

 private:
  // config attributes
  PipelineConfigs _configs;
#ifdef YAML_CONFIGS
  std::string _yaml_configs;
#endif

  // pipeline attributes
  GMainLoop *loop = g_main_loop_new(NULL, FALSE);
  GstElement *pipeline = NULL;
  GstBus *bus;
  guint bus_watch_id;
  pipelineUtils::BusStruct bus_struct;

  // thread pool to run pipelines
  BS::thread_pool _pool = BS::thread_pool(1);

  bool _set_up();

  GstElement *_create_bin(YAML::Node config, njson bin_conf);

  bool _setup_pipeline();

  bool _setup_pipeline_bus();

  // create a pipeline (config.json or config.yml)
  bool _create_pipeline();

#ifdef YAML_CONFIGS
  bool _create_pipeline_from_yaml(std::string file_path);
  bool _set_callbacks(GstElement *new_element, YAML::Node element);
#endif

  void _run_pipeline();

  void _pipeline_finished();

  void _create_kafka_publish_event(const guint stream_id);
};
}  // namespace core
