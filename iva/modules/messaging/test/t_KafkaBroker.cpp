#include <gtest/gtest.h>
// define private as public so that we can make unit tests on private class members and attributes
#define private public
#include "KafkaBroker.h"

namespace test_suite {
namespace messaging_test {
namespace {
// The fixture for testing class.
class MessagingTest : public ::testing::Test {
 protected:
  // declarations to be used across test cases
  core::KafkaBroker *kafka;
  njson settings;
  njson sample_payload;
  MessagingTest()
  {
    // class wide instantiation shared for use in each test factory, TEST_F.
    std::ifstream ifs("/src/configs/test_config.json");
    njson app_settings = njson::parse(ifs);
    this->settings = app_settings["messaging"];

    std::ifstream ifs2("/src/utils/payload-sample.json");
    this->sample_payload = njson::parse(ifs2);
  }

  ~MessagingTest() override
  {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override
  {
    // Code here will be called immediately after the constructor of each test factory, TEST_F, right before each test.
    this->kafka = new core::KafkaBroker();
  }

  void TearDown() override
  {
    // Code here will be called immediately after each test (right before the destructor).
  }

  // Class members declared here can be used by all tests in the test suite
};

TEST_F(MessagingTest, check_initial_attributes)
{
  // test endpoints are correct in self.config_path
  EXPECT_FALSE(this->kafka->_broker_connected) << "Validate _broker_connected is FALSE";
  EXPECT_FALSE(this->kafka->_producer_run) << "Validate _producer_run is FALSE";
  EXPECT_FALSE(this->kafka->_consumer_run) << "Validate _consumer_run is FALSE";
  EXPECT_FALSE(this->kafka->_consumer_failed) << "Validate _consumer_run is FALSE";
  EXPECT_FALSE(this->kafka->get_run_state()) << "Validate get_run_state is FALSE";
}

TEST_F(MessagingTest, member_set_configs)
{
  njson empty_conf;
  EXPECT_FALSE(this->kafka->set_configs(empty_conf)) << "Validate no arg returns false";
  EXPECT_TRUE(this->kafka->set_configs(this->settings)) << "Validate can set configs";

  // validate initial configs are loaded correctly
  EXPECT_EQ(this->kafka->_configs.consumer.topic, "spyder-distance") << "Validate _configs.consumer.topic=spyder-distance";
  EXPECT_EQ(this->kafka->_configs.producer.topic, "overlay-bbox") << "Validate _configs.producer.topic=overlay-bbox";

  EXPECT_FALSE(this->kafka->_configs.consumer.bootstrap.empty()) << "Validate _configs.consumer.bootstrap is not empty";
  EXPECT_FALSE(this->kafka->_configs.producer.bootstrap.empty()) << "Validate _configs.producer.bootstrap is not empty";

  auto kafka_ip = this->kafka->_configs.consumer.bootstrap[0];
  EXPECT_EQ(kafka_ip["bootstrap.servers"], "192.168.1.73:9092") << "Validate _configs.consumer.bootstrap IP";
  kafka_ip = this->kafka->_configs.producer.bootstrap[0];
  EXPECT_EQ(kafka_ip["bootstrap.servers"], "192.168.1.73:9092") << "Validate _configs.producer.bootstrap IP";
}

 TEST_F(MessagingTest, member_consumer_queue)
{
  EXPECT_TRUE(this->kafka->set_configs(this->settings)) << "Validate can set configs";
  EXPECT_FALSE(this->sample_payload.empty()) << "Validate sample payload is not empty";
  EXPECT_FALSE(this->sample_payload.is_null()) << "Validate sample payload is not null";
  EXPECT_TRUE(this->sample_payload.contains("meta")) << "Validate sample payload has 'meta' field";
  EXPECT_TRUE(this->sample_payload.contains("inference")) << "Validate sample payload has 'inference' field";
  EXPECT_TRUE(this->sample_payload.contains("topic")) << "Validate sample payload has 'topic' field";
//  EXPECT_EQ(this->sample_payload.dump(), "true") << "Validate sample payload has meta field";

  // add payload to queue
  this->kafka->consumer_lock.lock();
  this->kafka->consumer_q.push(this->sample_payload);
  this->kafka->consumer_lock.unlock();

  njson sample_p = this->kafka->consume();
  EXPECT_FALSE(sample_p.is_null());
  EXPECT_FALSE(sample_p.empty());
  EXPECT_EQ(sample_p.dump(), this->sample_payload.dump());
  njson empty = this->kafka->consume();
  EXPECT_TRUE(empty.is_null());
  EXPECT_TRUE(empty.empty());
}


}  // namespace
}  // namespace messaging_test
}  // namespace test_suite