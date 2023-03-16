// Copyright (c) 2023, Stogl Robotics Consulting UG (haftungsbeschränkt)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "test_gravity_compensation.hpp"
#include <vector>

TEST_F(GravityCompensationTest, TestGravityCompensationMissingParameters)
{
  std::shared_ptr<filters::FilterBase<geometry_msgs::msg::WrenchStamped>> filter_ =
    std::make_shared<control_filters::GravityCompensation<geometry_msgs::msg::WrenchStamped>>();

  node_->declare_parameter("world_frame", "world");
  node_->declare_parameter("sensor_frame", "sensor");

  // one mandatory param missing, should fail
  ASSERT_FALSE(filter_->configure("", "TestGravityCompensation",
    node_->get_node_logging_interface(), node_->get_node_parameters_interface()));
  /* NOTE: one cannot declare or set the missing param afterwards, to then test if configure works,
   * because the param is read only and cannot be set anymore.
   */
}

TEST_F(GravityCompensationTest, TestGravityCompensationParameters)
{
  std::shared_ptr<filters::FilterBase<geometry_msgs::msg::WrenchStamped>> filter_ =
    std::make_shared<control_filters::GravityCompensation<geometry_msgs::msg::WrenchStamped>>();

  double gravity_acc = 9.81;
  double mass = 5.0;
  node_->declare_parameter("world_frame", "world");
  node_->declare_parameter("sensor_frame", "sensor");
  node_->declare_parameter("force_frame", "world");
  node_->declare_parameter("CoG.force", std::vector<double>({0.0, 0.0, -gravity_acc * mass}));

  node_->declare_parameter("CoG.pos", std::vector<double>({0.0, 0.0}));
  // wrong vector size, should fail
  ASSERT_FALSE(filter_->configure("", "TestGravityCompensation",
    node_->get_node_logging_interface(), node_->get_node_parameters_interface()));

  node_->set_parameter(rclcpp::Parameter("CoG.pos", std::vector<double>({0.0, 0.0, 0.0})));
  // all parameters correctly set AND second call to yet unconfigured filter
  ASSERT_TRUE(filter_->configure("", "TestGravityCompensation",
    node_->get_node_logging_interface(), node_->get_node_parameters_interface()));

  // change a parameter
  node_->set_parameter(rclcpp::Parameter("CoG.pos", std::vector<double>({0.0, 0.0, 0.2})));
  // accept second call to configure with valid parameters to already configured filter
  ASSERT_TRUE(filter_->configure("", "TestGravityCompensation",
    node_->get_node_logging_interface(), node_->get_node_parameters_interface()));
}

TEST_F(GravityCompensationTest, TestGravityCompensation)
{
  std::shared_ptr<filters::FilterBase<geometry_msgs::msg::WrenchStamped>> filter_ =
    std::make_shared<control_filters::GravityCompensation<geometry_msgs::msg::WrenchStamped>>();

  double gravity_acc = 9.81;
  double mass = 5.0;
  node_->declare_parameter("world_frame", "world");
  node_->declare_parameter("sensor_frame", "sensor");
  node_->declare_parameter("force_frame", "world");
  node_->declare_parameter("CoG.pos", std::vector<double>({0.0, 0.0, 0.0}));
  node_->declare_parameter("CoG.force", std::vector<double>({0.0, 0.0, -gravity_acc * mass}));

  ASSERT_TRUE(filter_->configure("", "TestGravityCompensation",
    node_->get_node_logging_interface(), node_->get_node_parameters_interface()));

  geometry_msgs::msg::WrenchStamped in, out;
  in.header.frame_id = "world";
  in.wrench.force.x = 1.0;
  in.wrench.torque.x = 10.0;

  // should fail due to missing sensor frame to world transform
  ASSERT_FALSE(filter_->update(in, out));
  node_->set_parameter(rclcpp::Parameter("sensor_frame", "world"));
  // should pass (now transform is identity)
  ASSERT_TRUE(filter_->update(in, out));

  ASSERT_EQ(out.wrench.force.x, 1.0);
  ASSERT_EQ(out.wrench.force.y, 0.0);
  ASSERT_EQ(out.wrench.force.z, gravity_acc * mass);

  ASSERT_EQ(out.wrench.torque.x, 10.0);
  ASSERT_EQ(out.wrench.torque.y, 0.0);
  ASSERT_EQ(out.wrench.torque.z, 0.0);

  out.header.frame_id = "base";
  // should fail due to missing transform for desired output frame
  ASSERT_FALSE(filter_->update(in, out));

  // TODO(guihomework) Add a test with real lookups
}