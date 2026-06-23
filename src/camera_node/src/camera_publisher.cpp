#include <chrono>
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "image_transport/image_transport.hpp"
#include "cv_bridge/cv_bridge.h"
#include "opencv2/opencv.hpp"

using namespace std::chrono_literals;

class CameraPublisher : public rclcpp::Node {
    public:
    CameraPublisher() : Node("camera_publisher") {
        this->declare_parameter("camera_id", 0);
        this->declare_parameter("fps", 30);

        int camera_id = this->get_parameter("camera_id").as_int();
        int fps = this->get_parameter("fps").as_int();

        cap_.open(camera_id);
        if (!cap_.isOpened()) {
            RCLCPP_ERROR(this->get_logger(), "ERRO: Nenhuma câmera encontrada no ID %d", camera_id);
        } else {
            RCLCPP_INFO(this->get_logger(), "Câmera %d iniciada! Publicando a %d FPS.", camera_id, fps);
        }

        pub_ = image_transport::create_publisher(this, "camera/image", rclcpp::QoS(10).get_rmw_qos_profile());

        auto period = std::chrono::milliseconds(1000 / fps);
        timer_ = this->create_wall_timer(period, std::bind(&CameraPublisher::timer_callback, this));
        }

    private:
        void timer_callback() {
            if (!cap_.isOpened()) return;

            cv::Mat frame;
            cap_ >> frame; // pega o frame atual
            if (frame.empty()) return;

            cv::resize(frame, frame, cv::Size(320, 240));

            // converter openCV Mat para sensor_msgs com cv_bridge
            std_msgs::msg::Header header;
            header.stamp = this->now();
            header.frame_id = "camera_link";

            sensor_msgs::msg::Image::SharedPtr msg = cv_bridge::CvImage(header, "bgr8", frame).toImageMsg();

            pub_.publish(*msg);
        }

        cv::VideoCapture cap_;
        image_transport::Publisher pub_;
        rclcpp::TimerBase::SharedPtr timer_;
    };

    int main(int argc, char * argv[]) {
        rclcpp::init(argc, argv);
        rclcpp::spin(std::make_shared<CameraPublisher>());
        rclcpp::shutdown();
        return 0;
}