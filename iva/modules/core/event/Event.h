#pragma once

#include <glib.h>
#include <uuid/uuid.h>

#include <nlohmann/json.hpp>
#include <ostream>
#include <string>

#include "BaseComponent.h"
#include "errors.hpp"
#include "logging.hpp"

using njson = nlohmann::json;

namespace core {

/**
 * @namespace events
 * @brief mediator events to keep abstraction build up separate from mediator.
 *  All events should be derived from Event and set up with their appropriate Module and Type
 */
namespace events {

/**
 * @enum Module
 * @brief holds all modules in the program
 */
enum Module {
  MODULE_NONE = 0,
  MODULE_APPLICATION,
  MODULE_APPCONTEXT,
  MODULE_PIPELINE,
  MODULE_PROCESSING,
  MODULE_KAFKA,
  ERROR_MODULE = 9999
};

const std::map<Module, std::string> module_to_str{
    {Module::MODULE_NONE, "MODULE_NONE"},
    {Module::MODULE_APPLICATION, "MODULE_APPLICATION"},
    {Module::MODULE_APPCONTEXT, "MODULE_APPCONTEXT"},
    {Module::MODULE_PIPELINE, "MODULE_PIPELINE"},
    {Module::MODULE_PROCESSING, "MODULE_PROCESSING"},
    {Module::MODULE_KAFKA, "MODULE_KAFKA"},
    {Module::ERROR_MODULE, "ERROR_MODULE"},
};

std::ostream &operator<<(std::ostream &os, core::events::Module module);

/**
 * @enum Type
 * @brief describes the type of event that is called in Mediator.cpp
 */
enum Type {
  NO_TYPE = 0,
  APPLICATION,
  APPCONTEXT,
  PIPELINE,
  PROCESSING,
  KAFKA,
  ERROR_TYPE = 9999
};

/**
 * @enum Actions
 * @brief describes the actions being performed in mediator.cpp
 */
enum Actions {
  NO_ACTION = 0,
  START_MODULES,
  STOP_MODULES,
  CONFIGURE_MODULES,
  KAFKA_PRODUCE_PAYLOAD,
  KAFKA_CONSUME_PAYLOAD,
  ERROR_ACTION = 9999
};

const std::map<Actions, std::string> action_to_str{{Actions::NO_ACTION, "NO_ACTION"},
                                                   {Actions::START_MODULES, "START_MODULES"},
                                                   {Actions::STOP_MODULES, "STOP_MODULES"},
                                                   {Actions::CONFIGURE_MODULES, "CONFIGURE_MODULES"},
                                                   {Actions::KAFKA_PRODUCE_PAYLOAD, "KAFKA_PRODUCE_PAYLOAD"},
                                                   {Actions::KAFKA_CONSUME_PAYLOAD, "KAFKA_CONSUME_PAYLOAD"},
                                                   {Actions::ERROR_ACTION, "ERROR_ACTION"}};

std::ostream &operator<<(std::ostream &os, core::events::Actions action);

};  // namespace events

/**
 * @class EventBase
 * @brief a base class to for all events to fit form of mediator.cpp
 * @var t
 * the type
 * @var _source
 * the source of the event (who invoked this action)
 * @var _target
 * the target module of the event (where data is coming from)
 * @var _action
 * the type of action being called (for use in mediator.cpp)
 * @var _accepted
 * safe guard to ensure that the event can be started
 * @var _owned
 * safe guard to know that the event has been started
 * @var _completed
 * releases the event so that another event may be processed
 * @var _criteria_met
 * validates that necessary run criteria are good before starting event
 * @var _time_elapsed
 * the time elapsed for this event
 */
class EventBase {
 public:
  EventBase();

  explicit EventBase(events::Type type);

  EventBase(const int action, const int type, const int src = 0, const int dst = 0);

  virtual ~EventBase();

  inline events::Type type() const { return static_cast<events::Type>(this->t); }

  void accept() { this->_accepted = true; }

  void ignore() { this->_accepted = false; }

  bool accepted() const { return this->_accepted; }

  void set_accepted(bool accepted) { this->_accepted = accepted; }

  void end();

  void own() { this->_owned = true; }

  void free() { this->_owned = false; }

  bool owned() const { return _owned; }

  void set_owned(bool owned) { this->_owned = owned; }

  bool completed() const { return _completed; }

  int source() const { return _source; }

  int action() const { return _action; }

  /**
   * @brief override operator for "<<" so that you can print a numan readable string of the event to LOG(<level>)
   * @copydoc LOG(INFO) << *core::Event::<DerivedEvent>
   *
   * @param os placeholder (used by LOG(<level>)
   * @param rhs Event class being accessed
   * @return  std::osstream (of type string)
   */
  friend std::ostream &operator<<(std::ostream &os, const core::EventBase &rhs)
  {
    os << "type: " << rhs.t << "\naction: " << rhs._action << "\nsource_module: "
	   << rhs._source << "\ntarget_module: " << rhs._target;
    return os;
  }

 protected:
  int t = 0;
  int _source = 0;
  int _target = 0;
  int _action = 0;

 private:
  bool _accepted = false;
  bool _owned = false;
  bool _completed = false;
  bool _criteria_met = false;
  std::uint64_t _time_elapsed;
};

/* Derived Events */

/**
 * @class ApplicationEvent
 * @brief Derived from EventBase and responsible for all events started by the Application module
 */
class ApplicationEvent : public EventBase {
 public:
  using EventBase::EventBase;

  explicit ApplicationEvent(const gint action, const guint src = 0);
};

/**
 * @class PipelineEvent
 * @brief Derived from EventBase and responsible for all events started by the Pipeline module
 */
class PipelineEvent : public EventBase {
 public:
  using EventBase::EventBase;

  PipelineEvent(const gint action, const std::string uuid, const gint src = 0);
};

/**
 * @class KafkaEvent
 * @brief Derived from EventBase and responsible for all events started by the Kafka module
 * @var _requested_uuid
 * the UUID of the pipeline that called this event (if publishing data)
 */
class KafkaEvent : public EventBase {
 public:
  using EventBase::EventBase;

  KafkaEvent(const gint action, const std::string uuid, const guint src);

  KafkaEvent(const gint action);


  std::string _requested_uuid;
};

}  // namespace core