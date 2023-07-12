#include <nlohmann/json.hpp>

/// include all module imports here to pass classes as args --> Mediator::Mediator(..args)
#include "ApplicationContext.h"
#include "KafkaBroker.h"
#include "Mediator.h"
#include "Pipeline.h"
#include "Processing.h"
#include "callbacks.hpp"

using njson = nlohmann::json;
using namespace core;

/**
 * @brief Mediator design pattern that manages data flow and control signals between modules
 * @param app_context core::ApplicationContext which is the mediator interface for the application module
 * @param kafka core::KafkaBroker to interface kafka with mediator
 * @param pipeline core::Pipeline to interface pipeline with mediator
 * @param disparity core::DisparityDepth to interface disparity with mediator
 */
core::Mediator::Mediator(
    core::ApplicationContext *app_context,
    core::KafkaBroker *kafka,
    core::Pipeline *pipeline
    )
{
  LOG(INFO) << "Setting application and assigning mediator to all classes";
  this->app_context = app_context;
  this->kafka = kafka;
  this->pipeline = pipeline;
  this->app_context->set_mediator(this);
  this->kafka->set_mediator(this);
  this->pipeline->set_mediator(this);
  this->pipeline->processor->set_mediator(this);
  LOG(INFO) << "Finished assigning mediator to all classes";
}

/**
 * @brief this is the main action handler for all mediator events
 * @note no data should be stored in this class member, and data should only be passed to its appropriate end-point.
 *      This means the sender has a getter, and the receiver has a setter so that the Mediator only facilitates the
 *      flow of data.
 * @copydoc
 *         GETTER: njson <module>::<module>.send_json() returns njson as a getter
 *         SETTER: Kafka::Kafka.receive_json(njson) stores a json file in a queue
 *         USAGE :
 *         kafka.receive_json(<module>.send_json());
 *
 *         View how no data is stored in the mediator, it only passes data from one module to the next.
 *         THIS IS THE WAY!
 *
 *
 * @param event the Event class which contains all information needed to trigger an action
 */
void core::Mediator::notify(core::EventBase *event)
{
  VLOG(EVENT)  << "Event Notification: \n" << *event;

  int action = event->action();
  // signal
  switch (action) {
    case events::Actions::CONFIGURE_MODULES: {
      /**
       * @brief parses config.json and distributes settings to each module
       */
      VLOG(EVENT) << "Called: events::Actions::CONFIGURE_MODULES ";
      event->own();
      // TODO: set_configs returns a bool; safely close application is false
      if(!this->pipeline->set_configs(this->app_context->get_configs(core::events::Type::PIPELINE))
          || !this->pipeline->processor->set_configs(this->app_context->get_configs(core::events::Type::PROCESSING))
          || !this->kafka->set_configs(this->app_context->get_configs(core::events::Type::KAFKA))
          )
        this->app_context->run_state = false;
      else
        this->app_context->run_state = true;
      event->end();
      break;
    }
    case events::Actions::START_MODULES: {
      /**
       * @brief used by application.cpp to start all modules
       */

      event->own();
      if (this->app_context->run_state)
      {
        LOG(INFO) << "Called: events::Actions::START_MODULES ";
        this->kafka->start();
        this->pipeline->start();
      }
      else {
        LOG(WARNING) << "[FAILED] Called: events::Actions::START_MODULES ";
      }
      event->end();
      break;
    }
    case events::Actions::STOP_MODULES: {
      /**
       * @brief used by application.cpp or pipeline.cpp to stop all modules
       */
      LOG(INFO) << "Called: events::Actions::STOP_MODULES ";
      event->own();

      if (this->kafka->get_run_state()) {
        LOG(INFO) << "Mediator closing kafka";
        this->kafka->stop();
      }
      else
        LOG(WARNING) << "Mediator found that Kafka is already terminated";

      LOG(INFO) << "Mediator closing app_context";
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      this->app_context->kill_app();
      LOG(INFO) << "Mediator updated app_context to close app (state=" << this->app_context->app_state << ")";
      event->end();
      break;
    }
    case events::Actions::KAFKA_PRODUCE_PAYLOAD: {
      /**
       * @brief sends payload from processing module to kafka producer
       */
      VLOG(EVENT) << "Called: events::Actions::KAFKA_PRODUCE_PAYLOAD ";
      KafkaEvent *poly_event = dynamic_cast<KafkaEvent *>(event);
      poly_event->own();
      this->kafka->publish(this->pipeline->processor->get_meta_queue());
      event->end();
      break;
    }
  }
  // clean up
  if (event->completed() && !event->owned()) {
    delete event;
  }
}
