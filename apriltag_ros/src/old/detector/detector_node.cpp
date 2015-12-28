#include "apriltag_ros/detector_node.h"
#include "apriltag_ros/utils.h"

#include <thread>

#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <apriltag_ros/Apriltags.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <AprilTags/Tag36h11.h>

namespace apriltag_ros {

DetectorNode::DetectorNode(const ros::NodeHandle &nh,
                           const ros::NodeHandle &pnh)
    : nh_(nh),
      tag_family_("36h11"),
      cam_calibrated_(true),
      it_(nh),
      sub_camera_(
          it_.subscribeCamera("image_raw", 1, &DetectorNode::CameraCb, this)),
      pub_tags_(nh_.advertise<apriltag_ros::Apriltags>("apriltags", 1)),
      tag_detector_(AprilTags::tag_codes_36h11),
      tag_viz_(nh, "apriltags_marker") {
  // Do nothing if nobody subscribes
  /*
  ros::SubscriberStatusCallback connect_cb =
      boost::bind(&DetectorNode::ConnectCb, this);
  pub_apriltags_ = nh_.advertise<apriltag_ros::Apriltags>(
      "apriltags", 1, connect_cb, connect_cb);
  */
  if (!pnh.getParam("size", tag_size_)) {
    throw std::runtime_error("No tag size specified");
  }
  tag_viz_.set_color(kr::viz::colors::RED);
  tag_viz_.set_alpha(0.75);
}

void DetectorNode::ConnectCb() {
  std::lock_guard<std::mutex> lock(connect_mutex_);
  if (!pub_tags_.getNumSubscribers())
    sub_camera_.shutdown();
  else if (!sub_camera_) {
    image_transport::TransportHints hints("raw", ros::TransportHints(), nh_);
    sub_camera_ = it_.subscribeCamera("image_raw", 2, &DetectorNode::CameraCb,
                                      this, hints);
  }
}

void DetectorNode::CameraCb(const sensor_msgs::ImageConstPtr &image_msg,
                            const sensor_msgs::CameraInfoConstPtr &cinfo_msg) {
  // Only show detection if camera is not calibrated
  if (cinfo_msg->K[0] == 0.0 || cinfo_msg->height == 0) {
    ROS_ERROR_THROTTLE(1, "%s: %s", nh_.getNamespace().c_str(),
                       "camera not calibrate");
    cam_calibrated_ = false;
  }

  // Retrieve camera info and image
  model_.fromCameraInfo(cinfo_msg);
  cv::Mat image = cv_bridge::toCvCopy(
                      image_msg, sensor_msgs::image_encodings::MONO8)->image;

  // Disable drawing later
  cv::Mat color;
  cv::cvtColor(image, color, CV_GRAY2BGR);

  // Detect tags
  std::vector<AprilTags::TagDetection> detections =
      tag_detector_.extractTags(image);

  // Process detection
  if (!detections.empty()) {
    // Maybe use Ptr?
    Apriltags tags_c_msg;
    tags_c_msg.header = image_msg->header;
    // Actual processing
    std::for_each(begin(detections), end(detections),
                  [&](const AprilTags::TagDetection &detection) {
      tags_c_msg.apriltags.push_back(DetectionToApriltagMsg(detection));
      detection.draw(color);  // Disable drawing later
    });
    tag_viz_.PublishApriltagsMarker(tags_c_msg);
    pub_tags_.publish(tags_c_msg);
  }

  // Display
  cv::imshow("image", color);
  cv::waitKey(1);
}

Apriltag DetectorNode::DetectionToApriltagMsg(
    const AprilTags::TagDetection &detection) {
  Apriltag tag;
  // Gather basic information
  tag.id = detection.id;
  tag.family = tag_family_;
  tag.hamming_distance = detection.hammingDistance;
  tag.center.x = detection.cxy.first;
  tag.center.y = detection.cxy.second;
  tag.size = tag_size_;
  std::for_each(begin(detection.p), end(detection.p),
                [&](const AprilTags::Pointf &corner) {
    geometry_msgs::Point point;
    point.x = corner.first;
    point.y = corner.second;
    tag.corners.push_back(point);
  });
  if (!cam_calibrated_) return tag;
  // Get rotation and translation of tag in camera frame, only if we have cinfo!
  Eigen::Quaterniond c_Q_b;
  Eigen::Vector3d c_T_b;
  /// @todo: Need to decide whether to undistort points here!
  detection.getRelativeQT(tag_size_, model_.fullIntrinsicMatrix(),
                          model_.distortionCoeffs(), c_Q_b, c_T_b);
  SetPose(&tag.pose, c_Q_b, c_T_b);
  return tag;
}

}  // namespace apriltag_ros
