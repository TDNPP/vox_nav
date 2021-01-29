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

#include <nav_msgs/msg/path.hpp>
#include <botanbot_control/mpc_controller/mpc_controller.hpp>


using namespace std;

namespace botanbot_control
{
namespace mpc_controller
{

MPCController::MPCController(rclcpp::Node::SharedPtr parent)
{
  node_ = parent;

  cmd_vel_publisher_ =
    node_->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);

  odom_subscriber_ = node_->create_subscription<nav_msgs::msg::Odometry>(
    "/odometry/global", rclcpp::SystemDefaultsQoS(),
    std::bind(&MPCController::globalOdometryCallback, this, std::placeholders::_1));

  // setup TF buffer and listerner to read transforms
  tf_buffer_ = std::make_shared<tf2_ros::Buffer>(node_->get_clock());
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

  timer_ = node_->create_wall_timer(
    std::chrono::milliseconds(100),
    std::bind(&MPCController::timerCallback, this));

  geometry_msgs::msg::PoseStamped pose;
  pose.header.frame_id = "map";

  pose.pose.position.x = -1.0;
  pose.pose.position.y = -1.0;
  ref_traj.poses.push_back(pose);

  pose.pose.position.x = -2.0;
  pose.pose.position.y = -2.0;
  ref_traj.poses.push_back(pose);

  pose.pose.position.x = -3.0;
  pose.pose.position.y = -3.0;
  ref_traj.poses.push_back(pose);

  pose.pose.position.x = -4.0;
  pose.pose.position.y = -4.0;
  ref_traj.poses.push_back(pose);

  pose.pose.position.x = -5.0;
  pose.pose.position.y = -5.0;
  ref_traj.poses.push_back(pose);

  pose.pose.position.x = -6.0;
  pose.pose.position.y = -6.0;
  ref_traj.poses.push_back(pose);

  xvals_base_link.resize(ref_traj.poses.size());
  yvals_base_link.resize(ref_traj.poses.size());


  if (flg_init == false) {
    printf("-------  initialized the acado ------- \n");
    control_output = init_acado();
    flg_init = true;
  }

  previous_time_ = node_->now();

}


MPCController::~MPCController()
{
}

void MPCController::globalOdometryCallback(
  const nav_msgs::msg::Odometry::ConstSharedPtr msg)
{
  latest_recived_odom_ = *msg;
}


void MPCController::timerCallback()
{

  rclcpp::Duration transfrom_tolerance(std::chrono::seconds(1));

  int index = 0;
  for (auto && path_pose_map : ref_traj.poses) {
    geometry_msgs::msg::PoseStamped path_pose_base_link;
    path_pose_map.header.frame_id = "map";
    path_pose_map.header.stamp.nanosec = node_->now().nanoseconds();
    path_pose_base_link.header.frame_id = "base_link";
    path_pose_base_link.header.stamp.nanosec = node_->now().nanoseconds();

    botanbot_utilities::transformPose(
      tf_buffer_,
      "base_link", path_pose_map, path_pose_base_link,
      transfrom_tolerance);

    //ptsx.push_back(path_pose_base_link.pose.position.x);
    //ptsy.push_back(path_pose_base_link.pose.position.y);

    xvals_base_link[index] = path_pose_base_link.pose.position.x;
    yvals_base_link[index] = path_pose_base_link.pose.position.y;
    std::cout << "PATH IN BASE LINK X Y " << xvals_base_link[index] << yvals_base_link[index] <<
      std::endl;

    index++;
  }

  double dt = node_->now().seconds() - previous_time_.seconds();
  // reject if step is too long
  if (dt > 0.5) {
    previous_time_ = node_->now();
    RCLCPP_WARN(node_->get_logger(), "A large step found but gonna igore it ");
    dt = node_->now().seconds() - previous_time_.seconds();
  }

  geometry_msgs::msg::PoseStamped curr_robot_pose;
  if (!botanbot_utilities::getCurrentPose(
      curr_robot_pose, *tf_buffer_, "map", "base_link", 0.1))
  {
    RCLCPP_DEBUG(node_->get_logger(), "Current robot pose is not available.");
  }

  int nearest_state_index = calculate_nearest_state_index(
    ptsx, ptsy,
    curr_robot_pose.pose.position.x,
    curr_robot_pose.pose.position.y);

  std::cout << "nearest point x: " << ptsx[nearest_state_index] << std::endl;
  std::cout << "nearest point y: " << ptsy[nearest_state_index] << std::endl;

  tf2::Quaternion q;
  tf2::fromMsg(curr_robot_pose.pose.orientation, q);
  double roll, pitch, yaw;

  tf2::Matrix3x3 m(q);
  m.getRPY(roll, pitch, yaw);

  // current robot states
  double x = curr_robot_pose.pose.position.x;
  double y = curr_robot_pose.pose.position.y;
  double theta = yaw;
  double v = latest_recived_odom_.twist.twist.linear.x;

  auto coeffs = polyfit(xvals_base_link, yvals_base_link, 3);
  std::cout << "coeffs \n " << coeffs << std::endl;

  double target_speed = 1.0;

  // acado setting
  // because current pos is in local coordinate, x = y = psi = 0
  vector<double> cur_state = {0, 0, 0, 0};
  vector<double> predicted_states = motion_prediction(cur_state, control_output);
  vector<double> ref_states = calculate_ref_states(coeffs, target_speed, v);
  control_output = run_mpc_acado(predicted_states, ref_states, control_output);

  // Display the MPC predicted trajectory
  vector<double> mpc_x_vals;
  vector<double> mpc_y_vals;

  // .. add (x,y) points to list here, points are in reference to the vehicle's coordinate system
  // the points in the simulator are connected by a Green line
  for (int i = 0; i < predicted_states.size(); i++) {
    if (i % NY == 0) {
      mpc_x_vals.push_back(predicted_states[i]);
    } else if (i % NY == 1) {
      mpc_y_vals.push_back(predicted_states[i]);
    }
  }

  // Display the waypoints/reference line
  vector<double> next_x_vals;
  vector<double> next_y_vals;
  //.. add (x,y) points to list here, points are in reference to the vehicle's coordinate system
  // the points in the simulator are connected by a Yellow line
  for (int x = 0; x < 100; x = x + 5) {
    double y = polyeval(coeffs, x);
    next_x_vals.push_back(x);
    next_y_vals.push_back(y);
  }

  std::cout << "acceleration cmd " << control_output[0][0] << std::endl;
  std::cout << "steering angle " << control_output[1][0] << std::endl;
  std::cout << "Time step " << dt << std::endl;

  twist.linear.x += control_output[0][0] * (dt);

  double kMAX_SPEED = 1.0;

  if (twist.linear.x > kMAX_SPEED) {
    twist.linear.x = kMAX_SPEED;
  } else if (twist.linear.x < -kMAX_SPEED) {
    twist.linear.x = -kMAX_SPEED;
  }

  twist.angular.z = twist.linear.x * control_output[1][0] / Lf;
  cmd_vel_publisher_->publish(twist);

  previous_time_ = node_->now();
}

void MPCController::configure(
  const rclcpp_lifecycle::LifecycleNode::WeakPtr &,
  std::string name, const std::shared_ptr<tf2_ros::Buffer> &
  /*const std::shared_ptr<nav2_costmap_2d::Costmap2DROS> &*/)
{

}

void MPCController::solve()
{

}
geometry_msgs::msg::TwistStamped MPCController::computeVelocityCommands(
  const geometry_msgs::msg::PoseStamped & pose,
  const geometry_msgs::msg::Twist & velocity)
{
  return geometry_msgs::msg::TwistStamped();
}

void MPCController::setPlan(const nav_msgs::msg::Path & path)
{

}

Eigen::VectorXd MPCController::polyfit(Eigen::VectorXd xvals, Eigen::VectorXd yvals, int order)
{
  assert(xvals.size() == yvals.size());
  assert(order >= 1 && order <= xvals.size() - 1);
  Eigen::MatrixXd A(xvals.size(), order + 1);

  for (int i = 0; i < xvals.size(); i++) {
    A(i, 0) = 1.0;
  }

  for (int j = 0; j < xvals.size(); j++) {
    for (int i = 0; i < order; i++) {
      A(j, i + 1) = A(j, i) * xvals(j);
    }
  }

  auto Q = A.householderQr();
  auto result = Q.solve(yvals);
  return result;
}

// Evaluate a polynomial.
double MPCController::polyeval(Eigen::VectorXd coeffs, double x)
{
  double result = 0.0;
  for (int i = 0; i < coeffs.size(); i++) {
    result += coeffs[i] * pow(x, i);
  }
  return result;
}


void MPCController::setSpeedLimit(const double & speed_limit)
{

}


}  // namespace mpc_controller

}  // namespace botanbot_control


int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<rclcpp::Node>("mpc");
  botanbot_control::mpc_controller::MPCController mpc(node);
  rclcpp::spin(node->get_node_base_interface());
  rclcpp::shutdown();
  return 0;
}
