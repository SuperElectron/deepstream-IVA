#include <gtest/gtest.h>
// define private as public so that we can make unit tests on private class members and attributes
#define private public
#include "Pipeline.h"


namespace test_suite
{
namespace pipeline_test
{
namespace
{
// The fixture for testing class.
class PipelineTest : public ::testing::Test
{
 protected:
  // declarations to be used across test cases
  core::Pipeline *pipeline;
  njson settings;
  PipelineTest()
  {
    // class wide instantiation shared for use in each test factory, TEST_F.
    std::ifstream ifs("/src/configs/test_config.json");
    njson app_settings = njson::parse(ifs);
    this->settings = app_settings["pipeline"];
  }

  ~PipelineTest() override
  {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override
  {
    // Code here will be called immediately after the constructor of each test factory, TEST_F, right before each test.
    this->pipeline = new core::Pipeline();
  }

  void TearDown() override
  {
    // Code here will be called immediately after each test (right before the destructor).
  }

  // Class members declared here can be used by all tests in the test suite
};

TEST_F(PipelineTest, check_initial_attributes)
{
  // test endpoints are correct in self.config_path
  EXPECT_TRUE(this->pipeline->_configs.empty()) << "Validate _configs is are empty";
}

TEST_F(PipelineTest, member_set_configs)
{
  njson empty_conf;
  EXPECT_FALSE(this->pipeline->set_configs(empty_conf)) << "Validate no arg returns false";
  EXPECT_TRUE(this->pipeline->set_configs(this->settings)) << "Validate can set configs";
  EXPECT_FALSE(this->pipeline->_configs.empty()) << "Validate _configs is are not empty";
}

TEST_F(PipelineTest, member_set_up)
{
  this->pipeline->set_configs(this->settings);
  bool ret = this->pipeline->_set_up();
  EXPECT_TRUE(ret) << "Validate set_up member works";
}

TEST_F(PipelineTest, member_set_up_NoConfig)
{
  bool ret;
  try {
    ret = this->pipeline->_set_up();
    FAIL() << "Expected 'bad file'";
  }
  catch(const std::exception &e) {
    EXPECT_EQ(e.what(),std::string("bad file"));
  }
  catch(...) {
    FAIL() << "Expected 'bad file'";
  }
}


}  // namespace
}  // namespace pipeline_test
}  // namespace test_suite