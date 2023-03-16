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

#ifndef CONTROL_FILTERS__GRAVITY_COMPENSATION_HPP_
#define CONTROL_FILTERS__GRAVITY_COMPENSATION_HPP_

#include <memory>
#include <string>
#include <vector>

#include "gravity_compensation_filter_parameters.hpp"
#include "filters/filter_base.hpp"
#include "geometry_msgs/msg/vector3_stamped.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"

namespace control_filters
{



template <typename T>
class GravityCompensation : public filters::FilterBase<T>
{
public:
  /** \brief Constructor */
  GravityCompensation();

  /** \brief Destructor */
  ~GravityCompensation();

  /** @brief Configure filter parameters  */
  bool configure() override;

  /** \brief Update the filter and return the data separately
   * \param data_in T array with length width
   * \param data_out T array with length width
   */
  bool update(const T & data_in, T & data_out) override;

protected:
  void compute_internal_params()
  {
    cog_.vector.x = parameters_.CoG.pos[0];
    cog_.vector.y = parameters_.CoG.pos[1];
    cog_.vector.z = parameters_.CoG.pos[2];
    force_.vector.x = parameters_.CoG.force[0];
    force_.vector.y = parameters_.CoG.force[1];
    force_.vector.z = parameters_.CoG.force[2];
  };

private:
  rclcpp::Clock::SharedPtr clock_;
  std::shared_ptr<rclcpp::Logger> logger_;
  std::shared_ptr<gravity_compensation_filter::ParamListener> parameter_handler_;
  gravity_compensation_filter::Params parameters_;

  // Frames for Transformation of Gravity / CoG Vector
  std::string world_frame_;  // frame in which computation occur
  std::string sensor_frame_;  // frame in which Cog is given
  std::string force_frame_;  // frame in which external force is given

  // Storage for Calibration Values
  geometry_msgs::msg::Vector3Stamped cog_;  // Center of Gravity Vector (wrt Sensor Frame)
  geometry_msgs::msg::Vector3Stamped force_;  // Gravity Force Vector (wrt Force Frame)

  // Filter objects
  std::unique_ptr<tf2_ros::Buffer> p_tf_Buffer_;
  std::unique_ptr<tf2_ros::TransformListener> p_tf_Listener_;
  geometry_msgs::msg::TransformStamped transform_datain_world_, transform_world_dataout_,
    transform_cog_world_, transform_force_world_;
};

template <typename T>
GravityCompensation<T>::GravityCompensation()
{
}

template <typename T>
GravityCompensation<T>::~GravityCompensation()
{
}

template <typename T>
bool GravityCompensation<T>::configure()
{
  clock_ = std::make_shared<rclcpp::Clock>(RCL_SYSTEM_TIME);
  p_tf_Buffer_.reset(new tf2_ros::Buffer(clock_));
  p_tf_Listener_.reset(new tf2_ros::TransformListener(*p_tf_Buffer_.get(), true));

  logger_.reset(
    new rclcpp::Logger(this->logging_interface_->get_logger().get_child(this->filter_name_)));

  // Initialize the parameter_listener once
  if (!parameter_handler_)
  {
    try
    {
      parameter_handler_ =
        std::make_shared<gravity_compensation_filter::ParamListener>(this->params_interface_,
                                                                     this->param_prefix_);
    }
    catch (rclcpp::exceptions::ParameterUninitializedException & ex) {
      RCLCPP_ERROR((*logger_), "GravityCompensation filter cannot be configured: %s", ex.what());
      parameter_handler_.reset();
      return false;
    }
    catch (rclcpp::exceptions::InvalidParameterValueException & ex)  {
      RCLCPP_ERROR((*logger_), "GravityCompensation filter cannot be configured: %s", ex.what());
      parameter_handler_.reset();
      return false;
    }
  }
  parameters_ = parameter_handler_->get_params();
  compute_internal_params();

  return true;
}

}  // namespace control_filters

#endif  // CONTROL_FILTERS__GRAVITY_COMPENSATION_HPP_