#ifndef APRILTAG_ROS_MAPPER_NODE_H_
#define APRILTAG_ROS_MAPPER_NODE_H_

#include <ros/ros.h>
#include <apriltag_ros/Apriltags.h>
#include <sensor_msgs/CameraInfo.h>
#include <image_geometry/pinhole_camera_model.h>
#include <rviz_helper/marker_visualizer.hpp>
#include "apriltag_ros/visualizer.h"

#include "apriltag_ros/mapper.h"
#include "apriltag_ros/tag_map.h"

namespace apriltag_ros {

class MapperNode {
 public:
  MapperNode(const ros::NodeHandle& nh, const std::string& frame_id)
      : nh_(nh),
        sub_tags_(nh_.subscribe("apriltags", 1, &MapperNode::TagsCb, this)),
        sub_cinfo_(nh_.subscribe("camera_info", 1, &MapperNode::CinfoCb, this)),
        frame_id_(frame_id),
        mapper_(0.04, 1),
        pose_viz_(nh, "traj"),
        tag_viz_(nh, "apriltags_map") {
    pose_viz_.SetColor(kr::viz::colors::MAGENTA);
    pose_viz_.SetAlpha(1);
    pose_viz_.SetScale(0.01);
    tag_viz_.set_color(kr::viz::colors::GREEN);
    tag_viz_.set_alpha(0.75);
  }

  bool GetGoodTags(const std::vector<apriltag_ros::Apriltag> tags_c,
                   std::vector<apriltag_ros::Apriltag>* tags_c_good);

 private:
  void TagsCb(const apriltag_ros::ApriltagsConstPtr& tags_c_msg);
  void CinfoCb(const sensor_msgs::CameraInfoConstPtr& cinfo_msg);

  ros::NodeHandle nh_;
  ros::Subscriber sub_tags_;
  ros::Subscriber sub_cinfo_;
  std::string frame_id_;
  apriltag_ros::TagMap map_;
  apriltag_ros::Mapper mapper_;
  kr::viz::PoseVisualizer pose_viz_;
  apriltag_ros::ApriltagVisualizer tag_viz_;
  image_geometry::PinholeCameraModel model_;
};

}  // namespace apriltag_ros

#endif  // APRILTAG_ROS_MAPPER_NODE_H_
