#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string>

class GpsEdgeNode : public rclcpp::Node {
public:
    GpsEdgeNode() : Node("gps_edge_node"), serial_fd_(-1) {
        // Publica sentenças brutas em formato string com QoS Best Effort para latência zero
        pub_ = this->create_publisher<std_msgs::msg::String>(
            "/sensor/gps/raw_nmea", rclcpp::SensorDataQoS());

        // Abre a porta serial física mapeada pela Rasp (/dev/serial0 mapeia a UART dos pinos 8 e 10)
        const char* port = "/dev/serial0";
        serial_fd_ = open(port, O_RDONLY | O_NOCTTY | O_NDELAY);
        if (serial_fd_ < 0) {
            RCLCPP_ERROR(this->get_logger(), "Falha crítica: não foi possível abrir a porta serial %s", port);
            return;
        }

        // Configuração física de baixo nível do terminal POSIX para leitura UART assíncrona
        struct termios options;
        tcgetattr(serial_fd_, &options);

        // Define a taxa de transmissão padrão de 9600 baud do chipset MTK3339
        cfsetispeed(&options, B9600);
        cfsetospeed(&options, B9600);

        options.c_cflag |= (CLOCAL | CREAD); // Habilita recepção local de dados
        options.c_cflag &= ~PARENB;          // Sem bit de paridade
        options.c_cflag &= ~CSTOPB;          // 1 bit de parada (stop bit)
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;              // 8 bits de dados por caractere
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Entrada não-canônica (modo bruto)

        tcsetattr(serial_fd_, TCSANOW, &options);

        // Timer de alta frequência (100 Hz / 10 ms) para esvaziar o buffer serial sem lags
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(10),
            std::bind(&GpsEdgeNode::read_serial, this)
        );

        RCLCPP_INFO(this->get_logger(), "Nó de Borda C++ do GPS ativo! Lendo %s a 9600 baud.", port);
    }

    ~GpsEdgeNode() {
        if (serial_fd_ >= 0) {
            close(serial_fd_);
        }
    }

private:
    int serial_fd_;
    std::string line_buffer_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_;
    rclcpp::TimerBase::SharedPtr timer_;

    void read_serial() {
        if (serial_fd_ < 0) return;

        // 1. Array real para atuar como buffer de leitura
        char buf[256];

        // Lê até 255 bytes de uma vez, garantindo espaço para o terminador null
        int n = read(serial_fd_, buf, sizeof(buf) - 1);
        if (n > 0) {
            for (int i = 0; i < n; ++i) {
                char c = buf[i];
                if (c == '\n' || c == '\r') {
                    if (!line_buffer_.empty()) {
                        // 2. Checa se o PRIMEIRO caractere da linha recebida é o '$'
                        if (line_buffer_[0] == '$') {
                            auto msg = std_msgs::msg::String();
                            msg.data = line_buffer_;
                            pub_->publish(msg);
                        }
                        line_buffer_.clear(); // Limpa o buffer para a próxima sentença
                    }
                } else {
                    // Evita overflow de buffer caso receba bytes corrompidos
                    if (line_buffer_.size() < 120) {
                        line_buffer_ += c;
                    } else {
                        line_buffer_.clear();
                    }
                }
            }
        }
    }
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<GpsEdgeNode>());
    rclcpp::shutdown();
    return 0;
}
