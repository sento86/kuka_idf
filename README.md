# kuka_idf
Tools developed in the Automatizacion Industrial y Robotica (AIR) labotaroty of Instituto de Diseño y Fabricacion (IDF) at Universitat Politecnica de Valencia (UPV), in order to control and teleoperate the Kuka Agilus KR6 robot, either using the real system or Gazebo simulator.


## To build the package in your ROS workspace

catkin_make
catkin_make --force-cmake -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug


## Kuka KR6 simulation (in Gazebo)

Run the following scripts to load the controllers in the simulated robot:

roscore

roslaunch kuka_control demo_kr6_cell.launch gazebo:=true

rosrun rqt_joint_trajectory_controller rqt_joint_trajectory_controller ns:=kr6 (optional)


## Kuka KR6 real (hanged in cell)

Run the following scripts to load the controllers in the real robot:

roscore

roslaunch kuka_control demo_kr6_cell.launch gazebo:=false

rosrun rqt_joint_trajectory_controller rqt_joint_trajectory_controller ns:=kr6 (optional)


## Initialize Kinect drivers and run ICP algorithm for car door matching

roslaunch pcl_icp_matching3d init_cam.launch

rosrun pcl_icp_matching3d multiPC_ICP_multicam_new


## Compute surface normal from PointCloud using RealSense D435

roslaunch polish_sr300 demo_surface_normal.launch


## Teleoperation using generic gamepad

rosrun kuka_control kr6_hololens.py

roslaunch kuka_teleop teleop_gamepad.launch


## Teleoperation using Xbox gamepad and Microsoft HoloLens

roslaunch rosbridge_server rosbridge_websocket.launch

rosrun kuka_control kr6_hololens.py

roslaunch kuka_teleop teleop_hololens.launch


## ATI Axia80 driver

rosrun netft_rdt_driver netft_node 172.1.10.1

