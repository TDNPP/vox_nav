// Copyright (c) 2020 Fetullah Atas, Norwegian University of Life Sciences
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

#include <rclcpp/rclcpp.hpp>
#include <botanbot_control/controller_core.hpp>
#include <botanbot_utilities/tf_helpers.hpp>
#include <botanbot_control/mpc_controller/mpc_controller_core.hpp>

#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>
#include <visualization_msgs/msg/marker_array.hpp>

#include <string>
#include <memory>
#include <vector>

#ifndef BOTANBOT_CONTROL__MPC_CONTROLLER__MPC_CONTROLLER_ROS_HPP_
#define BOTANBOT_CONTROL__MPC_CONTROLLER__MPC_CONTROLLER_ROS_HPP_

namespace botanbot_control
{
namespace mpc_controller
{
class MPCControllerROS : public botanbot_control::ControllerCore
{
public:
  /**
   * @brief Construct a new MPCControllerROS object
   *
   * @param parent_node
   */
  MPCControllerROS(rclcpp::Node::SharedPtr parent_node);

  /**
   * @brief Destroy the MPCControllerROS object
   *
   */
  ~MPCControllerROS();

  /**
   * @brief
   *
   * @param parent_node
   */
  void configure(
    const rclcpp::Node::SharedPtr & parent_node) override;

  /**
   * @brief Set the Plan object
   *
   * @param path
   */
  void setPlan(const nav_msgs::msg::Path & path) override;

  /**
   * @brief
   *
   * @return geometry_msgs::msg::TwistStamped
   */
  geometry_msgs::msg::TwistStamped computeVelocityCommands() override;

  /**
   * @brief Solve th actual optimal control problem here
   *
   */
  void solve();

  /**
   * @brief get the index of nearest trajectory state to current robot pose
   *
   * @param reference_traj
   * @param curr_robot_pose
   * @return int
   */
  int nearestStateIndex(
    nav_msgs::msg::Path reference_traj,
    geometry_msgs::msg::PoseStamped curr_robot_pose);

  /**
   * @brief Get the Interpolated Refernce States object
   *
   * @param ref_traj
   * @param curr_robot_pose
   * @return std::vector<MPCControllerCore::States>
   */
  std::vector<MPCControllerCore::States> getInterpolatedRefernceStates(
    const nav_msgs::msg::Path ref_traj,
    geometry_msgs::msg::PoseStamped curr_robot_pose);

  /**
  * @brief Create a circular test trjectory, This function is only for simple testing
  *        to see whther robot is controlled correctly
  * @return nav_msgs::msg::Path
  */
  nav_msgs::msg::Path createTestTraj();

  /**
   * @brief Publish the test Trajectory prodiced as result of createTestTraj
   *
   */
  void publishTestTraj();

  /**
   * @brief return cloest reference trajectory to be feed into conrol sceheme
   *        The number of returned states will be determined according to time horizon(N)
   *
   * @param interpolated_ref_traj
   */
  void publishInterpolatedRefernceStates(
    std::vector<MPCControllerCore::States> interpolated_ref_traj);

private:
  // Shared pointer to parent node
  rclcpp::Node::SharedPtr node_;

  // ROS2 oublisher to publish velocity commands , for maual robot jogging
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_publisher_;
  // tf buffer to get access to transfroms
  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

  nav_msgs::msg::Path reference_traj_;
  geometry_msgs::msg::Twist twist_;
  rclcpp::Time previous_time_;

  std::shared_ptr<MPCControllerCore> mpc_controller_;

  MPCControllerCore::ControlInput previous_control_;

  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr plan_publisher_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr
    interpolated_ref_traj_publisher_;
};

}  // namespace mpc_controller
}  // namespace botanbot_control

#endif  // BOTANBOT_CONTROL__MPC_CONTROLLER__MPC_CONTROLLER_ROS_HPP_
