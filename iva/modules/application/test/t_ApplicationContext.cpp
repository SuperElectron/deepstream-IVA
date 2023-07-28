#include <gtest/gtest.h>
// define private as public so that we can make unit tests on private class members and attributes
#define private public
#include "ApplicationContext.h"


namespace test_suite
{
namespace applicationContext_test
{
namespace
{
// The fixture for testing class.
class ApplicationContextTest : public ::testing::Test
{
 protected:
  // declarations to be used across test cases
  core::ApplicationContext *app_context;
  ApplicationContextTest()
  {
    // class wide instantiation shared for use in each test factory, TEST_F.

  }

  ~ApplicationContextTest() override
  {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override
  {
    // Code here will be called immediately after the constructor of each test factory, TEST_F, right before each test.
    this->app_context = new core::ApplicationContext();
    this->app_context->_load_module_configs();
  }

  void TearDown() override
  {
    // Code here will be called immediately after each test (right before the destructor).
  }

  // Class members declared here can be used by all tests in the test suite
};

TEST_F(ApplicationContextTest, check_initial_attributes)
{
  // test endpoints are correct in self.config_path
  EXPECT_EQ(this->app_context->run_state, false) << "Validate run_state=false";
  EXPECT_EQ(this->app_context->app_state, core::APP_STATE::NO_APP_STATE) << "Validate run_state=false";
  EXPECT_FALSE(this->app_context->_configs.empty()) << "Validate that configs exist";
}

TEST_F(ApplicationContextTest, check_config_file)
{
  EXPECT_TRUE(this->app_context->_configs.contains("messaging")) << "Validate config has 'messaging' field";
  EXPECT_TRUE(this->app_context->_configs.contains("pipeline")) << "Validate config has 'pipeline' field";
}

TEST_F(ApplicationContextTest, check_app_states)
{
  EXPECT_EQ(core::APP_STATE::NO_APP_STATE, 0) << "Validate NO_APP_STATE value=0";
  EXPECT_EQ(core::APP_STATE::CONFIGURATION, 1) << "Validate CONFIGURATION value=1";
  EXPECT_EQ(core::APP_STATE::START, 2) << "Validate START value=2";
  EXPECT_EQ(core::APP_STATE::LIVE, 3) << "Validate LIVE value=3";
  EXPECT_EQ(core::APP_STATE::STOPPED, 4) << "Validate STOPPED value=4";
  EXPECT_EQ(core::APP_STATE::ERROR, 5) << "Validate ERROR value=5";
  EXPECT_EQ(core::APP_STATE::SHUT_DOWN, 1000) << "Validate SHUT_DOWN value=1000";
}

TEST_F(ApplicationContextTest, member_kill_app)
{
  this->app_context->kill_app();
  EXPECT_EQ(this->app_context->app_state, core::APP_STATE::SHUT_DOWN) << "validate kill_app() makes app_state=APP_STATE::SHUT_DOWN";
}

TEST_F(ApplicationContextTest, member_get_configs)
{
  njson pipeline_configs = this->app_context->get_configs(core::events::Type::PIPELINE);
  EXPECT_FALSE(pipeline_configs.empty()) << "Validate that pipeline configs are not empty";

  njson processing_configs = this->app_context->get_configs(core::events::Type::PROCESSING);
  EXPECT_FALSE(processing_configs.empty()) << "Validate that processing configs are not empty";

  njson kafka_configs = this->app_context->get_configs(core::events::Type::KAFKA);
  EXPECT_FALSE(kafka_configs.empty()) << "Validate that kafka configs are not empty";
}


}  // namespace
}  // namespace applicationContext_test
}  // namespace test_suite