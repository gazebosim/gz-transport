# Instead of sourcing ros setup script, set only required env variables.
# This is needed for finding zenoh packages vendored by ROS 2.
export ROS_DISTRO="jazzy"
LD_LIBRARY_PATH="/opt/ros/${ROS_DISTRO}/opt/zenoh_cpp_vendor/lib:$LD_LIBRARY_PATH"
