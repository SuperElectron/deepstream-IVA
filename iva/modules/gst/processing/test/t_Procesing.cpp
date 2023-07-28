#include <gtest/gtest.h>
// define private as public so that we can make unit tests on private class members and attributes
#define private public
#include "Processing.h"

namespace test_suite {
namespace processing_test {
namespace {
// The fixture for testing class.
class ProcessingTest : public ::testing::Test {
 protected:
  // declarations to be used across test cases
  core::Processing *processing;
  njson settings;
  ProcessingTest()
  {
    // class wide instantiation shared for use in each test factory, TEST_F.
    std::ifstream ifs("/src/configs/test_config.json");
    njson app_settings = njson::parse(ifs);
    this->settings = app_settings["pipeline"]["processing"];
  }

  ~ProcessingTest() override
  {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override
  {
    // Code here will be called immediately after the constructor of each test factory, TEST_F, right before each test.
    this->processing = new core::Processing();
  }

  void TearDown() override
  {
    // Code here will be called immediately after each test (right before the destructor).
  }

  // Class members declared here can be used by all tests in the test suite
};

TEST_F(ProcessingTest, check_initial_attributes)
{
  // test endpoints are correct in self.config_path
  EXPECT_FALSE(this->processing->_spyder_health) << "Validate _spyder_health is FALSE";
}

TEST_F(ProcessingTest, member_set_configs)
{
  njson empty_conf;
  EXPECT_FALSE(this->processing->set_configs(empty_conf)) << "Validate no arg returns false";
  EXPECT_TRUE(this->processing->set_configs(this->settings)) << "Validate can set configs";

  // validate initial configs are loaded correctly
  EXPECT_EQ(this->processing->_configs.topic, "overlay-bbox") << "Validate _configs.topic=overlay-bbox";
  EXPECT_EQ(this->processing->_configs.device_id, "overlay-bbox") << "Validate _configs.device_id=overlay-bbox";
  EXPECT_EQ(this->processing->_configs.paired_device_id, "spyder-1001") << "Validate _configs.device_id=spyder-1001";
  EXPECT_EQ(this->processing->_configs.model, "facenet") << "Validate _configs.model=facenet";
  EXPECT_EQ(this->processing->_configs.model_type, "face-detection") << "Validate _configs.model_type=face-detection";
  EXPECT_EQ(this->processing->_configs.publish, false) << "Validate _configs.device_id=false";
  EXPECT_EQ(this->processing->_configs.save, false) << "Validate _configs.device_id=false";
  EXPECT_EQ(this->processing->_configs.bbox_line_thickness, 4) << "Validate _configs.device_id=4";
  EXPECT_EQ(this->processing->_configs.minimum_iou_score, 60) << "Validate _configs.device_id=60";
  EXPECT_EQ(this->processing->_configs.min_confidence_to_display, 40) << "Validate _configs.device_id=40";
  EXPECT_EQ(this->processing->_configs.font_size, 2) << "Validate _configs.device_id=2";
}

TEST_F(ProcessingTest, member_set_up)
{
  this->processing->set_configs(this->settings);
  this->processing->set_up();
  EXPECT_EQ(this->processing->_processor.frame_counter, 0) << "validate that the Processing struct VideoSourceData is set";
}

TEST_F(ProcessingTest, member_set_up_NoConfig)
{
  bool ret;
  njson empty_conf;
  EXPECT_FALSE(this->processing->set_configs(empty_conf)) << "Validate no arg returns false";
}

TEST_F(ProcessingTest, validate_output_queue)
{
  njson empty_payload;
  this->processing->set_configs(this->settings);
  this->processing->set_up();
  EXPECT_FALSE(this->processing->check_output_queue()) << "Validate the output queue is empty";
  EXPECT_EQ(this->processing->_check_input_queue(), 0) << "Validate the input queue is empty";
  EXPECT_FALSE(this->processing->check_meta_queue()) << "Validate the meta queue is empty";
}

}  // namespace
}  // namespace processing_test
}  // namespace test_suite