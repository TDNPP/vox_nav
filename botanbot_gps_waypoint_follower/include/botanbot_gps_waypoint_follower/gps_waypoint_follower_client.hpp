// Copyright (c) 2020 Fetullah Atas
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
#ifndef BOTANBOT_GPS_WAYPOINT_FOLLOWER__WAYPOINT_FOLLOWER_CLIENT_HPP_
#define BOTANBOT_GPS_WAYPOINT_FOLLOWER__WAYPOINT_FOLLOWER_CLIENT_HPP_

#include "nav2_lifecycle_manager/lifecycle_manager_client.hpp"
#include "nav2_msgs/action/follow_waypoints.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "geometry_msgs/msg/point32.hpp"
#include "nav2_waypoint_follower/waypoint_follower.hpp"
#include "nav2_msgs/action/follow_gps_waypoints.hpp"
/**
 * @brief namespace for way point following, points are from a yaml file
 *
 */
namespace botanbot_gps_waypoint_follower
{
enum class ActionStatus
{
  UNKNOWN = 0,
  PROCESSING = 1,
  FAILED = 2,
  SUCCEEDED = 3
};

/**
 * @brief A ros node that drives robot through gievn way points from YAML file
 *
 */
class GPSWayPointFollowerClient : public rclcpp::Node
{
public:
  using ClientT = nav2_msgs::action::FollowGPSWaypoints;
  // shorten the Goal handler Client type
  using GPSWaypointFollowerGoalHandle =
    rclcpp_action::ClientGoalHandle<ClientT>;

  /**
   * @brief Construct a new WayPoint Folllower Demo object
   *
   */
  GPSWayPointFollowerClient();

  /**
   * @brief Destroy the Way Point Folllower Demo object
   *
   */
  ~GPSWayPointFollowerClient();

  /**
   * @brief send robot through each of the pose in poses vector
   *
   * @param poses
   */
  void startWaypointFollowing();

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  ActionStatus current_goal_status() const;

  /**
 * @brief given a parameter name on the yaml file, loads this parameter as sensor_msgs::msg::NavSatFix
 *  Note that this parameter needs to be an array of doubles
 *
 * @param param_name
 * @return sensor_msgs::msg::NavSatFix
 */
  std::vector<sensor_msgs::msg::NavSatFix>
  loadGPSWaypointsFromYAML(std::string waypoint_name_prefix, int num_waypoints);

  void resultCallback(
    const rclcpp_action::ClientGoalHandle
    <ClientT>::WrappedResult & result);

  void goalResponseCallback(
    std::shared_future<rclcpp_action::ClientGoalHandle
    <ClientT>::SharedPtr> future);

protected:
  ActionStatus current_goal_status_;

  // client to connect waypoint follower service(FollowWaypoints)
  rclcpp_action::Client<ClientT>::SharedPtr
    gps_waypoint_follower_action_client_;

  // goal handler to query state of goal
  ClientT::Goal gps_waypoint_follower_goal_;

  GPSWaypointFollowerGoalHandle::SharedPtr gps_waypoint_follower_goalhandle_;

  std::vector<sensor_msgs::msg::NavSatFix> gps_waypoints_from_yaml_;
};
}  // namespace botanbot_gps_waypoint_follower

#endif  // BOTANBOT_GPS_WAYPOINT_FOLLOWER__WAYPOINT_FOLLOWER_CLIENT_HPP_
