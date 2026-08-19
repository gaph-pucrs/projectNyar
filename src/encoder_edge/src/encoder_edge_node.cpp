#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int32.hpp>
#include <fstream>
#include <string>
#include <thread>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>

class EncoderEdgeNode : public rclcpp::Node {
public:
    EncoderEdgeNode() : Node("encoder_edge_node") {
        pub_ = this->create_publisher<std_msgs::msg::Int32>(
            "/sensor/encoder/raw_state", rclcpp::SensorDataQoS());

        setup_gpio(pin_clk_, "20");
        setup_gpio(pin_dt_, "21");

        active_ = true;
        poll_thread_ = std::thread(&EncoderEdgeNode::poll_gpios, this);

        RCLCPP_INFO(this->get_logger(), "Nó C++ de Coleta Ativo na Pi (GPIO 20 e 21)");
    }

    ~EncoderEdgeNode() {
        active_ = false;
        if (poll_thread_.joinable()) {
            poll_thread_.join();
        }
        if (fd_clk_ >= 0) close(fd_clk_);
        if (fd_dt_ >= 0) close(fd_dt_);
        unexport_gpio("20");
        unexport_gpio("21");
    }

private:
    const std::string pin_clk_ = "20";
    const std::string pin_dt_ = "21";
    int fd_clk_ = -1;
    int fd_dt_ = -1;
    std::thread poll_thread_;
    bool active_;
    rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr pub_;

    void setup_gpio(const std::string& /*pin*/, const std::string& pin_num) {
        std::ofstream export_file("/sys/class/gpio/export");
        if (export_file.is_open()) {
            export_file << pin_num;
            export_file.close();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::ofstream dir_file("/sys/class/gpio/gpio" + pin_num + "/direction");
        if (dir_file.is_open()) {
            dir_file << "in";
            dir_file.close();
        }

        std::ofstream edge_file("/sys/class/gpio/gpio" + pin_num + "/edge");
        if (edge_file.is_open()) {
            edge_file << "both";
            edge_file.close();
        }

        std::string val_path = "/sys/class/gpio/gpio" + pin_num + "/value";
        if (pin_num == "20") {
            fd_clk_ = open(val_path.c_str(), O_RDONLY | O_NONBLOCK);
        } else {
            fd_dt_ = open(val_path.c_str(), O_RDONLY | O_NONBLOCK);
        }
    }

    void unexport_gpio(const std::string& pin_num) {
        std::ofstream unexport_file("/sys/class/gpio/unexport");
        if (unexport_file.is_open()) {
            unexport_file << pin_num;
            unexport_file.close();
        }
    }

    int read_fd(int fd) {
        if (fd < 0) return 0;
        char buf[2];
        lseek(fd, 0, SEEK_SET);
        if (read(fd, buf, 1) > 0) {
            return buf[0] - '0'; // Correção: Lendo o valor de dentro do buffer
        }
        return 0;
    }

    void poll_gpios() {
        struct pollfd fds[2];
        fds[0].fd = fd_clk_;         // Correção: Índice 0
        fds[0].events = POLLPRI | POLLERR;
        fds[1].fd = fd_dt_;          // Correção: Índice 1 (antes era 3)
        fds[1].events = POLLPRI | POLLERR;

        read_fd(fd_clk_);
        read_fd(fd_dt_);

        while (active_ && rclcpp::ok()) {
            int ret = poll(fds, 2, 100);
            if (ret > 0) {
                // Se houver dados em qualquer pino (POLLPRI), lemos os dois
                if ((fds[0].revents & POLLPRI) || (fds[1].revents & POLLPRI)) {
                    int clk_val = read_fd(fd_clk_);
                    int dt_val = read_fd(fd_dt_);

                    auto msg = std_msgs::msg::Int32();
                    msg.data = (clk_val << 1) | dt_val;
                    pub_->publish(msg);
                }
            }
        }
    }
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<EncoderEdgeNode>());
    rclcpp::shutdown();
    return 0;
}
