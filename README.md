# kuka_idf
Tools developed in the Automatizacion Industrial y Robotica (AIR) labotaroty of Instituto de Diseño y Fabricacion (IDF) at Universitat Politecnica de Valencia (UPV), in order to control and teleoperate the Kuka Agilus KR6 robot, either using the real system or Gazebo simulator.


## Simulation in Gazebo of the robotic cell with 3 depth cameras

    roslaunch kuka_control demo_cell.launch gazebo:=true


## Load robotic cell model with transforms to 3 depth cameras frames

    roslaunch kuka_control demo_cell.launch gazebo:=false
