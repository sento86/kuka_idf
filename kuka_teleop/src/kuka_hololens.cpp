/*
 * kuka_teleop
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <ORGANIZATION> nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

///\author Vicent Girbés Juan
///\brief Converts joystick commands on /joy to twist commands on /cmd_vel.
//***********************************************
#define DEFAULT_MAX_LEVEL           100.0
#define NUM_AXIS                    6
#define NUM_BUTTONS                 13
#define JOYSTICK_HZ                 100.0   //Desired frequency
#define JOYSTICK_TIMER              -1.0    //Time (secs) max. without reading (Timeout for joystick reset (<0 no timeout))

//***********************************************

#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include "ros/ros.h"

//***********************************************

#include "sensor_msgs/Joy.h"
#include "geometry_msgs/TwistStamped.h"

#include "std_msgs/String.h"
#include "std_msgs/Float64.h"
#include "std_msgs/Int32.h"

//***********************************************

using namespace std;

//***********************************************

class TeleopJoy
{
public:
  geometry_msgs::TwistStamped cmd_vel;
  double req_vx, req_vy, req_vz, req_wx, req_wy, req_wz;
  double sign_vx, sign_vy, sign_vz, sign_wx, sign_wy, sign_wz;
  double max_vx, max_vy, max_vz, max_wx, max_wy, max_wz;
  int axis_vx, axis_vy, axis_vz, axis_wx, axis_wy, axis_wz;
  double percentage_level;
  int deadman_button, percent_button;
  bool deadman_no_publish_, deadman_, last_deadman_;
  std::string last_selected_topic_;
  std::string cmd_vel_topic_, cmd_joint_topic_;

  sensor_msgs::Joy last_processed_joy_message_;
  ros::Time last_recieved_joy_message_time_;
  ros::Duration joy_msg_timeout_;

  ros::NodeHandle n_, n_private_;
  ros::Publisher vel_pub_;
  ros::Subscriber joy_sub_;

  TeleopJoy(bool deadman_no_publish = false) :
      sign_vx(1.0), sign_vy(1.0), sign_vz(1.0), sign_wx(1.0), sign_wy(1.0), sign_wz(1.0), max_vx(0.1), max_vy(0.1), max_vz(
          0.1), max_wx(0.5), max_wy(0.5), max_wz(0.5), deadman_no_publish_(deadman_no_publish), deadman_(false), last_deadman_(
          false), n_private_("~")
  {
  }

  void init()
  {
    cmd_vel.twist.linear.x = cmd_vel.twist.linear.y = cmd_vel.twist.linear.z = 0.0;
    cmd_vel.twist.angular.x = cmd_vel.twist.angular.y = cmd_vel.twist.angular.z = 0;

    n_private_.param("cmd_vel_topic", cmd_vel_topic_, std::string("cmd_vel"));
    n_private_.param("deadman_no_publish", deadman_no_publish_, deadman_no_publish_);

    // Set max speed (cartesian)
    n_private_.param("max_vx", max_vx, max_vx);
    n_private_.param("max_vy", max_vy, max_vy);
    n_private_.param("max_vz", max_vz, max_vz);
    n_private_.param("max_wx", max_wx, max_wx);
    n_private_.param("max_wy", max_wy, max_wy);
    n_private_.param("max_wz", max_wz, max_wz);

    // Set max speed (cartesian)
    n_private_.param("sign_vx", sign_vx, 1.0);
    n_private_.param("sign_vy", sign_vy, 1.0);
    n_private_.param("sign_vz", sign_vz, 1.0);
    n_private_.param("sign_wx", sign_wx, 1.0);
    n_private_.param("sign_wy", sign_wy, 1.0);
    n_private_.param("sign_wz", sign_wz, 1.0);

    // Set axis
    n_private_.param("axis_vx", axis_vx, 3);
    n_private_.param("axis_vy", axis_vy, 0);
    n_private_.param("axis_vz", axis_vz, 4);
    n_private_.param("axis_wx", axis_wx, 0);
    n_private_.param("axis_wy", axis_wy, 3);
    n_private_.param("axis_wz", axis_wz, 1);

    // Set buttons
    n_private_.param("deadman_button", deadman_button, 0);
    n_private_.param("percent_button", percent_button, 1);

    double joy_msg_timeout;
    n_private_.param("joy_msg_timeout", joy_msg_timeout, JOYSTICK_TIMER); //default to JOYSTICK_TIMER=1.0 seconds timeout
    if (joy_msg_timeout <= 0)
    {
      joy_msg_timeout_ = ros::Duration().fromSec(9999999); //DURATION_MAX;
      ROS_DEBUG("joy_msg_timeout <= 0 -> no timeout");
    }
    else
    {
      joy_msg_timeout_.fromSec(joy_msg_timeout);
      ROS_DEBUG("joy_msg_timeout: %.3f", joy_msg_timeout_.toSec());
    }
    ROS_DEBUG("sign_vx: %f", sign_vx);
    ROS_DEBUG("sign_vy: %f", sign_vy);
    ROS_DEBUG("sign_vz: %f", sign_vz);
    ROS_DEBUG("sign_wx: %f", sign_wx);
    ROS_DEBUG("sign_wy: %f", sign_wy);
    ROS_DEBUG("sign_wz: %f", sign_wz);

    ROS_DEBUG("max_vx: %.3f m/s", max_vx);
    ROS_DEBUG("max_vy: %.3f m/s", max_vy);
    ROS_DEBUG("max_vz: %.3f m/s", max_vz);
    ROS_DEBUG("max_wx: %.3f deg/s", max_wx*180.0/M_PI);
    ROS_DEBUG("max_wy: %.3f deg/s", max_wy*180.0/M_PI);
    ROS_DEBUG("max_wz: %.3f deg/s", max_wz*180.0/M_PI);

    ROS_DEBUG("axis_vx: %d", axis_vx);
    ROS_DEBUG("axis_vy: %d", axis_vy);
    ROS_DEBUG("axis_vz: %d", axis_vz);
    ROS_DEBUG("axis_wx: %d", axis_wx);
    ROS_DEBUG("axis_wy: %d", axis_wy);
    ROS_DEBUG("axis_wz: %d", axis_wz);

    ROS_DEBUG("deadman_button: %d", deadman_button);
    ROS_DEBUG("percent_button: %d", percent_button);
    ROS_DEBUG("joy_msg_timeout: %f", joy_msg_timeout);

    vel_pub_ = n_.advertise<geometry_msgs::TwistStamped>(cmd_vel_topic_, 1);

    joy_sub_ = n_.subscribe("joy", 10, &TeleopJoy::joy_cb, this);
  }

  ~TeleopJoy()
  {
  }

  /** Callback for joy topic **/
  void joy_cb(const sensor_msgs::Joy::ConstPtr& joy_msg)
  {
    // Do not process the same message twice.
    if (joy_msg->header.stamp == last_processed_joy_message_.header.stamp)
    {
      // notify the user only if the problem persists
      if (ros::Time::now() - joy_msg->header.stamp > ros::Duration(5.0 / JOYSTICK_HZ))
        ROS_WARN_THROTTLE(1.0,
                          "Received Joy message with same timestamp multiple times. Ignoring subsequent messages.");
      deadman_ = false;
      return;
    }
    last_processed_joy_message_ = *joy_msg;

    //Record this message reciept
    last_recieved_joy_message_time_ = ros::Time::now();

    deadman_ = (((unsigned int)deadman_button < joy_msg->buttons.size()) && joy_msg->buttons[deadman_button]);
    percentage_level = ((double)joy_msg->buttons[percent_button]) / DEFAULT_MAX_LEVEL;

    if (!deadman_)
      return;

    // Update axis values
    if ((axis_vx >= 0) && (((unsigned int)axis_vx) < joy_msg->axes.size()))
      req_vx = joy_msg->axes[axis_vx] * max_vx * percentage_level;
    else
      req_vx = 0.0;
    if ((axis_vy >= 0) && (((unsigned int)axis_vy) < joy_msg->axes.size()))
      req_vy = joy_msg->axes[axis_vy] * max_vy * percentage_level;
    else
      req_vy = 0.0;
    if ((axis_vz >= 0) && (((unsigned int)axis_vz) < joy_msg->axes.size()))
      req_vz = joy_msg->axes[axis_vz] * max_vz * percentage_level;
    else
      req_vz = 0.0;
    if ((axis_wx >= 0) && (((unsigned int)axis_wx) < joy_msg->axes.size()))
      req_wx = joy_msg->axes[axis_wx] * max_wx * percentage_level;
    else
      req_wx = 0.0;
    if ((axis_wy >= 0) && (((unsigned int)axis_wy) < joy_msg->axes.size()))
      req_wy = joy_msg->axes[axis_wy] * max_wy * percentage_level;
    else
      req_wy = 0.0;
    if ((axis_wz >= 0) && (((unsigned int)axis_wz) < joy_msg->axes.size()))
      req_wz = joy_msg->axes[axis_wz] * max_wz * percentage_level;
    else
      req_wz = 0.0;

    // Set a deadzone to get rid of noise
    double deadzone = 0.1;
    if (fabs(req_vx) < deadzone * max_vx * percentage_level)
      req_vx = 0.0;
    if (fabs(req_vy) < deadzone * max_vy * percentage_level)
      req_vy = 0.0;
    if (fabs(req_vz) < deadzone * max_vz * percentage_level)
      req_vz = 0.0;
    if (fabs(req_wx) < deadzone * max_wx * percentage_level)
      req_wx = 0.0;
    if (fabs(req_wy) < deadzone * max_wy * percentage_level)
      req_wy = 0.0;
    if (fabs(req_wz) < deadzone * max_wz * percentage_level)
      req_wz = 0.0;

    // Enforce max/mins for velocity
    // Joystick should be [-1, 1], but it might not be
    req_vx = max(min(req_vx, max_vx), -max_vx);
    req_vy = max(min(req_vy, max_vy), -max_vy);
    req_vz = max(min(req_vz, max_vz), -max_vz);
    req_wx = max(min(req_wx, max_wx), -max_wx);
    req_wy = max(min(req_wy, max_wy), -max_wy);
    req_wz = max(min(req_wz, max_wz), -max_wz);
  }

  void send_cmd()
  {
    if (deadman_ && last_recieved_joy_message_time_ + joy_msg_timeout_ > ros::Time::now())
    {
      // Copy linear and angular velocities to twist message
      cmd_vel.twist.linear.x = req_vx;
      cmd_vel.twist.linear.y = req_vy;
      cmd_vel.twist.linear.z = req_vz;
      cmd_vel.twist.angular.x = req_wx;
      cmd_vel.twist.angular.y = req_wy;
      cmd_vel.twist.angular.z = req_wz;
      // Publish cmd_vel
      cmd_vel.header.stamp = ros::Time::now();
      vel_pub_.publish(cmd_vel);
      ROS_DEBUG("kuka_teleop::hololens vx %f, vy %f, vz %f, wx %f, wy %f, wz %f", cmd_vel.twist.linear.x,
                cmd_vel.twist.linear.y, cmd_vel.twist.linear.z, cmd_vel.twist.angular.x, cmd_vel.twist.angular.y,
                cmd_vel.twist.angular.z);
    }
    else
    {
      // Publish zero commands if deadman_no_publish is false
      cmd_vel.twist.linear.x = cmd_vel.twist.linear.y = cmd_vel.twist.linear.z = 0;
      cmd_vel.twist.angular.x = cmd_vel.twist.angular.y = cmd_vel.twist.angular.z = 0;
      if (!deadman_no_publish_)
      {
        // Publish cmd_vel
        cmd_vel.header.stamp = ros::Time::now();
        vel_pub_.publish(cmd_vel);
      }
    }

    //make sure we store the state of our last deadman
    last_deadman_ = deadman_;
  }

};

int main(int argc, char **argv)
{
  // Initializing the roscpp Node
  ros::init(argc, argv, "kuka_gamepad");

  // Starting the roscpp Node
  ros::NodeHandle n;
//  ros::NodeHandle n("~");

  //***********************************************

  const char* opt_no_publish = "--deadman_no_publish";

  bool no_publish = false;
  for (int i = 1; i < argc; i++)
  {
    if (!strncmp(argv[i], opt_no_publish, strlen(opt_no_publish)))
      no_publish = true;
  }

  TeleopJoy teleop_joy(no_publish);
  teleop_joy.init();

  //***********************************************

  ros::Rate pub_rate(JOYSTICK_HZ);

  while (teleop_joy.n_.ok())
  {
    teleop_joy.send_cmd();
    ros::spinOnce();
    pub_rate.sleep();
  }

  exit(0);
  return 0;
}

