#include "KafkaBroker.h"

using namespace core;

//////////////////////////////////////////////////////////////
// UTILITIES
//////////////////////////////////////////////////////////////

/**
 * @brief   Format and send payload to kafka producer thread
 * @param   json_payload    @description json with "inference" and "meta" objects
 * @return  string          @description object that can have str.size() and st.c_str() for kafka producer
 */
std::string njson_to_string(njson payload)
{
  std::ostringstream oss;
  // add opening curly brace
  oss << "{";

  // LOG(INFO) << "Kafka payload to producer";
  for (njson::iterator it = payload.begin(); it != payload.end(); ++it) {
    // LOG(INFO) << "{ " << it.key() << ":" << it.value() << "}";
    oss << " \"" << it.key() << "\":" << it.value() << ",";
  }

  std::string str = oss.str();
  // remove trailing ','
  str.pop_back();
  // add closing curly brace
  str.append("}");
  return str;
}

/**
 * @brief a function to create a configuration argument for creating a consumer
 *
 * @param bootstrap_json    @description `clients["xxx"]["bootstrap"]` object of app_settings.json
 * @note this->_module_settings["producer"]["bootstrap"]
 * @note this->_module_settings["consumer"]["bootstrap"]
 *
 */
kafka::Properties json_props(njson bootstrap_json)
{
  kafka::Properties props;
  for (auto item : bootstrap_json.items()) {
    auto arr = item.value();
    for (auto &[key, value] : arr.items()) {
      props.put(key, value);
    }
  }
  return props;
}

//////////////////////////////////////////////////////////////

KafkaBroker::KafkaBroker()
{
  this->_module_id = core::Module::KAFKA;
  LOG(INFO) << "CREATED: " << *this;
}

KafkaBroker::~KafkaBroker() {}

/**
 * @brief loads modules settings from config.json
 * @return bool true if successful
 */
bool KafkaBroker::set_configs(njson conf)
{
  try {
    if(!conf["topic"].is_string()){
      LOG(WARNING) << "Invalid config.json element! messaging['topic'] must be a string";
      return false;
    }
    if(!conf["kafka_server_ip"].is_string()) {
      LOG(WARNING) << "Invalid config.json element! messaging['kafka_server_ip'] must be a string";
      return false;
    }
    if(!conf["enable"].is_boolean()) {
      LOG(WARNING) << "Invalid config.json element! messaging['enable'] must be a boolean";
      return false;
    }

    // unpack array of topics from config.json
    std::set<std::string> topics;
    std::string topics_display;
    ProducerSettings producerSettings = {
        .topic = "test",
        .kafka_server_ip = conf["kafka_server_ip"],
    };

    KafkaSettings configs = {.producer = producerSettings};
    this->_producer_enable = conf["enable"].get<bool>();
    this->_configs = configs;
  }
  catch (const std::exception &e) {
    LOG(ERROR) << "Error setting Kafka configs: " << e.what();
    return false;
  }
  return true;
};

/**
 *  @brief send a single dummy payload to kafka broker to ensure that the connection to the kafka broker is healthy
 */
void KafkaBroker::_validate_broker_connection()
{
  if (!this->_producer_enable)
    return;

  this->_broker_connected = false;
  while (!this->_broker_connected) {
    kafka::Properties props;
    props.put("bootstrap.servers", this->_configs.producer.kafka_server_ip);
    AdminClient adminClient(props);
    auto listResult = adminClient.listTopics();
    if (listResult.error) {
      LOG(ERROR) << "Cannot connect to kafka server: (" << this->_configs.producer.kafka_server_ip << ")" << listResult.error.message();
      continue;
    }
    VLOG(DEBUG) << "Searching for available topics ";
    std::string found_topics;

    for (const auto &topic : listResult.topics) {
      found_topics = found_topics + " | " + (std::string)topic;
    }
    LOG(INFO) << "The following topics are available: " << found_topics;
    this->_broker_connected = true;
  }
  // Log start up diagnostics
  VLOG(DEBUG) << "Started thread pool (threads = " << this->_pool.get_thread_count() << ")";

  // if the producer is not running, then start it up
  if (!this->_producer_run) {
    this->_pool.push_task(&KafkaBroker::_poll_producer, this);
  }
}

/**
 *  @brief queues up data for the producer thread (poll_producer)
 *  @param payload  a single payload to be added to the queue (which producer reads to publish messages)
 */
void KafkaBroker::publish(njson payload)
{
  VLOG(DEEP) << "Payload added to producer queue: " << payload.dump();
  this->producer_lock.lock();
  this->producer_q.push(payload);
  this->producer_lock.unlock();
}

/**
 *  @brief the main producer thread that reads from this->producer_q and publishes messages
 *
 *  @warning the topics in this->producer_q must include a "topic" the payload will be dropped!
 *
 */
void KafkaBroker::_poll_producer()
{
  LOG(INFO) << "Starting producer thread";
  // Get the producer store which is used to pass data from other threads to this thread
  this->_producer_run = true;
  VLOG(DEBUG) << "starting to poll";
  std::string send_topic;
  kafka::Properties props;
  props.put("bootstrap.servers", this->_configs.producer.kafka_server_ip);
  KafkaProducer publisher(props);
  try {
    while (this->_producer_run) {
      // check for available data
      this->producer_lock.lock();
      njson payload;
      int num_payloads = this->producer_q.size();
      this->producer_lock.unlock();
      if (num_payloads == 0)
        continue;

      // pull the first item off the queue
      this->producer_lock.lock();
      payload = this->producer_q.front();
      this->producer_q.pop();
      this->producer_lock.unlock();

      // ensure the payload contains a topic field
      if (!payload.contains("topic")) {
        LOG(ERROR) << "Payload does not include a topic";
        LOG(ERROR) << "Dropped payload: " << payload.dump();
        continue;
      }

      // get topic from payload for the producer
      send_topic = payload["topic"].get<std::string>();

      // convert the payload to a serialized binary stream for kafka
      std::string str = njson_to_string(payload);
      // add the payload to the producer record
      VLOG(DEEP) << "Producing" << str;
      // convert the payload to a kafka record
      producer::ProducerRecord record = producer::ProducerRecord(send_topic, kafka::NullKey, kafka::Value(str.c_str(), str.size()));

      // publish the record
      publisher.send(
          record,
          [](const producer::RecordMetadata &metadata, const kafka::Error &error) {
            if (error)
              throw error;
          },
          KafkaProducer::SendOption::ToCopyRecordValue);
    }
  }
  catch (const std::exception &e) {
    LOG(ERROR) << "Kafka Producer Subscribe Error [exit thread] : " << e.what();
    publisher.close();
    this->_producer_run = false;
    this->_pool.push_task(&KafkaBroker::_validate_broker_connection, this);
  }
  publisher.close();
  LOG(INFO) << "Kafka Producer thread inactive, ready to join.";
}

/**
 *  @brief starts the producer and consumer threads
 */
void KafkaBroker::start()
{
  LOG(INFO) << "Starting module";
  VLOG(DEBUG) << "Started thread pool (threads = " << this->_pool.get_thread_count() << ")";
  VLOG(DEBUG) << "Staring thread pool task: _validate_broker_connection";
  this->_pool.push_task(&KafkaBroker::_validate_broker_connection, this);
}

/**
 *  @brief set run flags to FALSE.  Class run flags are in main threads (for consumer and producer)
 *      to continually poll kafka's internal message queue. Setting this to false means that the
 *      thread will no longer loop over while(this->flag) which reads from the kafka internal message queue.
 */
void KafkaBroker::stop()
{
  LOG(INFO) << "Stopping module";

  // check how many messages are on the queue
  int num_producer_payloads = 1;
  while (num_producer_payloads != 0) {
    LOG(INFO) << "Checking kafka queue to see if unsent messages exist";
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // check number of payloads in producer queue
    this->producer_lock.lock();
    num_producer_payloads = this->producer_q.size();
    this->producer_lock.unlock();
  }
  this->_producer_run = false;
}

/**
 * @brief returns state of the threads
 *
 * @return bool     true if thread is running
 */
bool KafkaBroker::get_run_state() { return this->_producer_run;}
