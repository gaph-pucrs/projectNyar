#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/compressed_image.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <cstdio>
#include <ctime> // Magia antiga  pra converter o relógio do sistema pra tempo humano e legivel

class MultiCamReceptor : public rclcpp::Node {
public:
    MultiCamReceptor() : Node("multi_cam_receptor"), show_timestamp_(true) {

        // A MÁGICA DO BUFFER: QoS tamanho 1
        // Zero lag, zero filas. Ligou o programa, ele puxa só o frame atual.(Advento da IA AMEM IRMÃOS!!!)
        rclcpp::QoS qos_profile(1);
        qos_profile.best_effort();

        sub_ocam_ = this->create_subscription<sensor_msgs::msg::CompressedImage>(
            "/ocam/image_raw/compressed", qos_profile,
            std::bind(&MultiCamReceptor::ocam_callback, this, std::placeholders::_1));

        sub_zed_ = this->create_subscription<sensor_msgs::msg::CompressedImage>(
            "/zed/image_raw/compressed", qos_profile,
            std::bind(&MultiCamReceptor::zed_callback, this, std::placeholders::_1));

        sub_picam_ = this->create_subscription<sensor_msgs::msg::CompressedImage>(
            "/picam/image_raw/compressed", qos_profile,
            std::bind(&MultiCamReceptor::picam_callback, this, std::placeholders::_1));

        RCLCPP_INFO(this->get_logger(), "HUD Tático Online! Pressione 'T' nas janelas para ocultar/mostrar o tempo.");
    }

private:
    bool show_timestamp_; // Função booleana bala pra ligar e desligar o timestamp

    // Função que checa o teclado
    void checar_teclado() {
        char key = (char)cv::waitKey(1);
        if (key == 't' || key == 'T') {
            show_timestamp_ = !show_timestamp_; // Inverte o botão
        }
    }

    // Função que forja o tempo no formato Hora:Minuto:Segundo.Milissegundo
    void pintar_timestamp(cv::Mat& frame, const sensor_msgs::msg::CompressedImage::SharedPtr& msg, const cv::Scalar& cor) {
        if (!show_timestamp_) return; // Se o botão estiver desligado, não pinta nada

        // Converte os segundos brutos para a hora local
        time_t raw_time = msg->header.stamp.sec;
        struct tm * timeinfo = localtime(&raw_time);
        char clock_buffer[32];
        strftime(clock_buffer, sizeof(clock_buffer), "%H:%M:%S", timeinfo);

        // Pega os nanossegundos e transforma em milissegundos (3 dígitos)
        int millis = msg->header.stamp.nanosec / 1000000;

        char full_time[64];
        snprintf(full_time, sizeof(full_time), "Stamp: %s.%03d", clock_buffer, millis);

        cv::putText(frame, full_time, cv::Point(20, 75),
                     cv::FONT_HERSHEY_SIMPLEX, 0.7, cor, 2);
    }

	// Começa a settar os callbacks

    void ocam_callback(const sensor_msgs::msg::CompressedImage::SharedPtr msg) {
        try {
            cv::Mat frame = cv_bridge::toCvCopy(msg, "bgr8")->image;

            cv::putText(frame, "AZATHOTH Vision - oCam", cv::Point(20, 40),
                         cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

            pintar_timestamp(frame, msg, cv::Scalar(0, 255, 0));
            cv::imshow("oCam Image", frame);
            checar_teclado(); // Escuta o botão 'T'
        } catch (cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(), "Erro na ponte OpenCV oCam: %s", e.what());
        }
    }

    void zed_callback(const sensor_msgs::msg::CompressedImage::SharedPtr msg) {
        try {
            cv::Mat frame = cv_bridge::toCvCopy(msg, "bgr8")->image;

            cv::putText(frame, "AZATHOTH Vision - ZED", cv::Point(20, 40),
                         cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 140, 255), 2);

            pintar_timestamp(frame, msg, cv::Scalar(0, 140, 255));
            cv::imshow("ZED Image", frame);
            checar_teclado();
        } catch (cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(), "Erro na ponte OpenCV ZED: %s", e.what());
        }
    }

    void picam_callback(const sensor_msgs::msg::CompressedImage::SharedPtr msg) {
        try {
            cv::Mat frame = cv_bridge::toCvCopy(msg, "bgr8")->image;

            cv::putText(frame, "AZATHOTH Vision - PiCam CSI", cv::Point(20, 40),
                         cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 0, 255), 2);

            pintar_timestamp(frame, msg, cv::Scalar(255, 0, 255));
            cv::imshow("PiCam Image", frame);
            checar_teclado();
        } catch (cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(), "Erro na ponte OpenCV PiCam: %s", e.what());
        }
    }
	//Aqui ficam as inscrições dos tópicos ativos
    rclcpp::Subscription<sensor_msgs::msg::CompressedImage>::SharedPtr sub_ocam_;
    rclcpp::Subscription<sensor_msgs::msg::CompressedImage>::SharedPtr sub_zed_;
    rclcpp::Subscription<sensor_msgs::msg::CompressedImage>::SharedPtr sub_picam_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MultiCamReceptor>());
    rclcpp::shutdown();
    return 0;
}
