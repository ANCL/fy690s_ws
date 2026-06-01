# FY690S Workspace

## Software Installation

### Clone the Repository
```
cd
git clone --recursive https://github.com/ANCL/fy690s_ws.git
```

### Install ROS2 Jazzy
Follow the instructions [here](https://docs.ros.org/en/jazzy/Installation/Ubuntu-Install-Debs.html).
Then add the source command to .bashrc:
```
echo "source /opt/ros/jazzy/setup.bash" >> ~/.bashrc
source ~/.bashrc
```

### Install QGC
Follow the instructions [here](https://docs.qgroundcontrol.com/Stable_V5.0/en/qgc-user-guide/getting_started/download_and_install.html#ubuntu).
After installation, move the QGC App image to home directory (~). Run QGC, and click the QGC icon at the top-left corner -> Application Settings -> Fly view -> toggle "enable virtual joystick".

### Setup PX4
```
bash ~/fy690s_ws/px4/PX4-Autopilot/Tools/setup/ubuntu.sh 
cd ~/fy690s_ws/px4/PX4-Autopilot/
make px4_sitl
```

### Setup Micro XRCE-DDS Agent & Client
```
chmod +x ~/fy690s_ws/scripts/install_src_uxrce_agent.sh
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
### Run the Examples
```
# new terminal
MicroXRCEAgent udp4 -p 8888
# new terminal
cd ~/fy690s_ws/px4/PX4-Autopilot/
make px4_sitl gz_x500
# new terminal 
cd
./QGroundControl-x86_64.AppImage 
```
#### Example 1. Sensor-Combined Listener
```
# new terminal
cd ~/fy690s_ws/ros2_ws
source install/local_setup.bash
ros2 launch px4_ros_com sensor_combined_listener.launch.py
```
#### Example 2. Offboard Control
```
# new terminal
cd ~/fy690s_ws/ros2_ws
source install/local_setup.bash
ros2 run px4_ros_com offboard_control
```

## Firmware Installation & Configuration
### Flashing PX4 v1.16.2 dev firmware to Pixhawk
Connect the Pixhawk to QGC on the dev computer using a USB-C cable. After QGC detects the Pixhawk, follow the instructions [here](https://docs.px4.io/main/en/dev_setup/building_px4#nuttx-pixhawk-based-boards) to flash the firmware. For example, to flash the Pixhawk 6C, do
```
cd ~/fy690s_ws/px4/PX4-Autopilot/
make px4_fmu-v6_default upload
```
Then, back up the current QGC parameters
```
QGroundControl → Vehicle Setup → Parameters → Tools → Save to file
```
Also， record outputs of the following commands by running them in Analyze Tools → MAVLink Console:
```
param show MAV_*_CONFIG
param show UXRCE_DDS_CFG
param show SER_TEL*
param show SENS_EN_LL40LS
param show EKF2_EV_CTRL
param show EKF2_HGT_REF
param show EKF2_GPS_CTRL
param show EKF2_BARO_CTRL
param show EKF2_RNG_CTRL
```
### Setup TELEM1 for QGC Telemetry connection
We use TELEM1 as the MAVLink channel for QGC telemetry radio.
```
param set MAV_0_CONFIG 101
param set SER_TEL1_BAUD 57600
param set MAV_0_MODE 0
param set MAV_0_RATE 1200
param set MAV_0_RADIO_CTL 1
```
### Setup uXRCE-DDS connection
Assuming we use the following physical wiring
```
Companion Computer ↔ USB-UART ↔ TELEM3 on Pixhawk
```
First, disable other MAVLink links on TELEM3
```
param set MAV_2_CONFIG 0
```
Enable uXRCE-DDS on TELEM3
```
param set UXRCE_DDS_CFG 103
param set SER_TEL3_BAUD 921600
param set UXRCE_DDS_DOM_ID 0
param set UXRCE_DDS_KEY 1
param set UXRCE_DDS_SYNCT 1
reboot
```
This means:
```
UXRCE_DDS_CFG = 103  → use TELEM3
101 = TELEM1
102 = TELEM2
103 = TELEM3
1000 = Ethernet
SER_TEL3_BAUD = 921600 → TELEM3 serial baud rate
UXRCE_DDS_DOM_ID = 0 → must match ROS_DOMAIN_ID on companion computer
```
Check the status in the QGC MAVLink Console
```
uxrce_dds_client status
```
At this stage, it says `disconnected` until the Micro XRCE-DDS Agent is running on the Companion Computer.

On the companion computer, identify the USB-serial device:
```
ls -l /dev/serial/by-id/
sudo dmesg | grep -i tty
```
You may see
```
/dev/ttyUSB0
/dev/ttyACM0
/dev/serial/by-id/usb-FTDI_...
```
Start the agent using `by-id`. If you see multiple IDs in the last step, try them one-by-one until the connection is successful:
```
MicroXRCEAgent serial --dev /dev/serial/by-id/<your_usb_uart_device> -b 921600
```
If the connection is successful, the terminal should print a long list; otherwise, there will be only two lines. 
Then verify 
```
ros2 topic list | grep /fmu
ros2 topic echo /fmu/out/vehicle_status
```
### Configure external vision fusion

### Configure offboard-loss behavior and RC overwrite

### Configure indoor flight 
TODO: disable GPS, disable magnetometer, allow arming without GPS, etc. 

### Configure Garmin LiDAR on I2C
TODO: connect distance sensor 

## Vehicle Power/Actuator/Sensor Calibration
