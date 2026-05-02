# Workspace Template

## Installation

### Clone the Repository
```
cd
git clone --recursive https://github.com/ANCL/fy690s_ws.git
```

### [Install ROS2 Jazzy](https://docs.ros.org/en/jazzy/Installation/Ubuntu-Install-Debs.html)

Optional: Add source command to .bashrc
```
echo "source /opt/ros/<distro>/setup.bash" >> ~/.bashrc
source ~/.bashrc
```

### [Install QGC](https://docs.qgroundcontrol.com/Stable_V5.0/en/qgc-user-guide/getting_started/download_and_install.html#ubuntu)


### [Setup PX4](https://docs.px4.io/main/en/ros2/user_guide#install-px4)
```
bash ~/fy690s_ws/px4/PX4-Autopilot/Tools/setup/ubuntu.sh 
cd px4/PX4-Autopilot/
make px4_sitl
```

### [Setup Micro XRCE-DDS Agent & Client](https://docs.px4.io/main/en/ros2/user_guide#setup-micro-xrce-dds-agent-client)

```
~/fy690s_ws/scripts/install_src_uxrce_agent.sh
```

### Compile ROS2 Workspace
```
cd ~/fy690s_ws/ros2_ws/
colcon build
```
Optional: Add source command to .bashrc
```
echo "source ~/fy690s_ws/ros2_ws/install/local_setup.bash" >> ~/.bashrc
source ~/.bashrc
```

## Run the Examples
```
# new terminal
MicroXRCEAgent udp4 -p 8888
# new terminal
cd fy690s_ws/px4/PX4-Autopilot/
make px4_sitl gz_x500
# new terminal 
cd
./QGroundControl-x86_64.AppImage 
```
### Example 1. Sensor-Combined Listener
```
# new terminal
cd ./ros2_ws
source install/local_setup.bash
ros2 launch px4_ros_com sensor_combined_listener.launch.py
```
### Example 2. Offboard Control
```
# new terminal
cd ~/fy690s_ws/ros2_ws
source install/local_setup.bash
ros2 run px4_ros_com offboard_control
```

