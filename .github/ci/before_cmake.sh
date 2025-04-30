
set -x
ROS_DISTRO="jazzy"
export ROS_DISTRO=${ROS_DISTRO}
#export LD_LIBRARY_PATH="/opt/ros/${ROS_DISTRO}/opt/zenoh_cpp_vendor/lib:$LD_LIBRARY_PATH"
#export CMAKE_PREFIX_PATH="/opt/ros/${ROS_DISTRO}/opt/zenoh_cpp_vendor/lib/cmake:$CMAKE_PREFIX_PATH"
ROS_SETUP_SCRIPT="/opt/ros/${ROS_DISTRO}/setup.bash"
 if [ -f ${ROS_SETUP_SCRIPT} ]; then
  set -a
  . ${ROS_SETUP_SCRIPT}
  set +a
fi
