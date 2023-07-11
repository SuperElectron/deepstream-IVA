#pragma once

// modern-cpp-kafka headers
#include <kafka/AdminClient.h>
#include <kafka/KafkaConsumer.h>
#include <kafka/KafkaProducer.h>

#include <BS_thread_pool.hpp>
#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>
#include <thread>
#include <set>

#include "BaseComponent.h"
#include "logging.hpp"

// include namespace for json
using njson = nlohmann::json;
// include namespace for modern-cpp-kafka
using namespace kafka::clients;

namespace core {

class BaseComponent;

class Mediator;

/**
 * @struct ProducerSettings
 * @brief Kafka settings for producer
 * @var topic
 * the default topic when a payload doesn't have a topic in it
 * @var bootstrap
 * kafka settings to setup
 */
struct ProducerSettings {
  std::string topic;
  njson bootstrap;
};

/**
 * @struct KafkaSettings
 * @brief Kafka module settings from config.json
 * @var consumer
 * the kafka consumer settings
 * @var producer
 * the kafka producer settings
 */
struct KafkaSettings {
  ProducerSettings producer;
};

/**
 * @class KafkaBroker
 * @brief Derived from BaseComponent and responsible for all messaging with external clients
 *
 * @vars _configs
 * the class configs
 * @var _pool
 * the thread pool to manage the admin client, producer, and consumer
 * @var _broker_connected
 * boolean that holds connection status with kafka server
 * @var _producer_run
 * control flag to enable or disable from another module (via stop() or start())
 * @var producer_lock
 * thread safe lock on all producer objects (mainly its queue)
 * @var producer_q
 * all data to be produced is added to this queue from other modules (via publish())
 */
class KafkaBroker : public BaseComponent {
 public:
  // constructor
  KafkaBroker();

  // external interface to push data (publish) and pull data (consume)
  void publish(njson);

  // set module configs (must do before starting)
  bool set_configs(njson);

  // main thread members
  void start();
  bool get_run_state();
  void stop();

  // de-constructor
  ~KafkaBroker();

 private:
  KafkaSettings _configs;
  // thead pool to run producer and consumer
  BS::thread_pool _pool = BS::thread_pool(2);

  // general connection to the kafka server
  bool _broker_connected = false;

  // producer members and attributes
  bool _producer_run = false;
  std::mutex producer_lock;
  std::queue<njson> producer_q;

  // threaded members to get data in and out of application
  void _poll_producer();

  // kafka admin client connects with server and sets _broker_connected
  void _validate_broker_connection();

};

}  // namespace core
