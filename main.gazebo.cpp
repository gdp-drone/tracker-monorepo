#include <ros/ros.h>
#include <opencv2/core.hpp>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "tracker-arb/TrackerARB.h"
#include "tracker-hsv/TrackerHSV.h"
#include "agent-xb/AgentXB.h"
#include <opencv2/core.hpp>
///< ROS headers
#include <geometry_msgs/TwistStamped.h>
#include <geometry_msgs/Vector3.h>
#include <std_msgs/Bool.h>


#define DEFAULT_PORT 0
#define VID_CAPTURE_WIDTH 640


cv::Mat frame;
cv::Vec3d tVec, rVec;

CVCalibration cvl("CalibParams.txt");

// Instantiate tracker of small markers
float markerLength = 3.62;
float markerSeparation = 2.63;
int markersX = 6;
int markersY = 8;
int markerDict = 0;
unique_ptr<Tracker> tracker_s(
  new TrackerARB(cvl, markerLength, markerSeparation, markersX, markersY, markerDict, true));

// Instantiate tracker of larger markers
markerLength = 9.89;
markerSeparation = 15.15;
markersX = 2;
markersY = 2;
markerDict = 4;

unique_ptr<Tracker> tracker_l(
  new TrackerARB(cvl, markerLength, markerSeparation, markersX, markersY, markerDict, true));

// Instantiate HSV tracker
unique_ptr<Tracker> tracker_hsv(
  new TrackerHSV(cvl, true));

// Instantiate Tracking Agent with trackers
AgentXB trackingAgent(AgentXB::MODE_GREEDY, true);
trackingAgent.addTracker(move(tracker_s));
trackingAgent.addTracker(move(tracker_l));
trackingAgent.addTracker(move(tracker_hsv));

ros::Publisher vishnu_cam_data_pub;
ros::Publisher vishnu_cam_detection_pub;

std_msgs::Bool bool_msg;
geometry_msgs::Twist data_msg;

class ImageConverter
{
  ros::NodeHandle nh_;
  image_transport::ImageTransport it_;
  image_transport::Subscriber image_sub_;


public:
  ImageConverter()
  : it_(nh_)
  {
      // Subscribe to input video feed and publish output video feed
    image_sub_ = it_.subscribe("/iris/usb_cam/image_raw", 1,
     &ImageConverter::imageCb, this);


    cv::namedWindow("oprgb");
  }

  ~ImageConverter()
  {
    cv::destroyWindow("oprgb");
  }

  void imageCb(const sensor_msgs::ImageConstPtr &msg)
  {
    cv_bridge::CvImagePtr cv_ptr;
    try
    {
      cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::RGB8);
    }
    catch (cv_bridge::Exception &e)
    {
      ROS_ERROR("cv_bridge exception: %s", e.what());
      return;
    }


    if (trackingAgent.getPose(cv_ptr->image, tVec, rVec))
    {
      trackingAgent.smaPose(tVec, tVec);
      ROS_INFO("X: %f, Y: %f, Z: %f, ID: %d", tVec[0], tVec[1], tVec[2], trackingAgent.activeTracker);
      data_msg.linear.x   = (float) (tVec[0] /  100);
      data_msg.linear.y   = (float) (tVec[1] / 100);
      data_msg.linear.z   = (float) (tVec[2] / 100);
      data_msg.angular.x  = (float) rVec[0];
      data_msg.angular.y  = (float) rVec[1];
      data_msg.angular.z  = (float) rVec[2];
      vishnu_cam_data_pub.publish(data_msg);
      bool_msg.data = 1;

    }
    else
    {
      bool_msg.data = 0;
    }

    vishnu_cam_detection_pub.publish(bool_msg);

      // Update GUI Window
    cv::imshow("oprgb", cv_ptr->image);
    cv::waitKey(3);
  }

};

int main(int argc, char **argv)
{
  ros::init(argc, argv, "vishnu_cam_node");
  ros::NodeHandle n;
  ImageConverter ic;

  // Publish to bodyframe pos of tag to topic /vishnu_cam_data
  vishnu_cam_data_pub = n.advertise<geometry_msgs::Twist>("vishnu_cam_data", 1);
  // Publish whether cam is detecting the ARtag
  vishnu_cam_detection_pub = n.advertise<std_msgs::Bool>("vishnu_cam_detection", 1);


  ros::spin();
  return 0;
}