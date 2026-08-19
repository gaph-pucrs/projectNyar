#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <sensor_msgs/msg/nav_sat_fix.hpp>
#include <sensor_msgs/msg/nav_sat_status.hpp>
#include <string>
#include <vector>
#include <sstream>
#include <cmath>

class GpsLogicClient : public rclcpp::Node {
public:
    GpsLogicClient() : Node("gps_logic_client") {
        // Subscreve ao tópico de sentenças brutas vindas da Pi usando QoS Best Effort (Sensor Data)
        sub_ = this->create_subscription<std_msgs::msg::String>(
            "/sensor/gps/raw_nmea", rclcpp::SensorDataQoS(),
            std::bind(&GpsLogicClient::nmea_callback, this, std::placeholders::_1));

        // Publica os dados de posicionamento convertidos e estruturados no padrão ROS 2
        pub_ = this->create_publisher<sensor_msgs::msg::NavSatFix>("/sensor/gps/fix", 10);

        RCLCPP_INFO(this->get_logger(), "Processador Lógico C++ do GPS Inicializado na Workstation!");
    }

private:
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_;
    rclcpp::Publisher<sensor_msgs::msg::NavSatFix>::SharedPtr pub_;

    // Método auxiliar para dividir as strings NMEA pelas vírgulas
    std::vector<std::string> split(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    // Converte o formato do GPS (DDMM.MMMM ou DDDMM.MMMM) para Graus Decimais convencionais
    double convert_to_decimal_degrees(const std::string& nmea_val, const std::string& direction) {
        if (nmea_val.empty() || direction.empty()) {
            return 0.0;
        }

        try {
            size_t dot_pos = nmea_val.find('.');
            if (dot_pos == std::string::npos || dot_pos < 2) {
                return 0.0;
            }

            // Em NMEA, os minutos ocupam sempre as duas casas imediatamente anteriores ao ponto decimal (MM.MMMM)
            size_t deg_length = dot_pos - 2;
            double degrees = std::stod(nmea_val.substr(0, deg_length));
            double minutes = std::stod(nmea_val.substr(deg_length));

            // Fórmula padrão: Graus Decimais = Graus + (Minutos / 60)
            double decimal_degrees = degrees + (minutes / 60.0);

            // Norte (N) e Leste (E) são positivos, Sul (S) e Oeste (W) são negativos
            if (direction == "S" || direction == "W") {
                decimal_degrees = -decimal_degrees;
            }

            return decimal_degrees;
        } catch (const std::exception& e) {
            RCLCPP_ERROR(this->get_logger(), "Erro ao converter coordenada NMEA (%s, %s): %s",
                         nmea_val.c_str(), direction.c_str(), e.what());
            return 0.0;
        }
    }

    void nmea_callback(const std_msgs::msg::String::SharedPtr msg) {
        std::string nmea = msg->data;

        // Nós focamos na sentença RMC (Recommended Minimum Navigation Information)
        if (nmea.rfind("$GPRMC", 0) == 0 || nmea.rfind("$GNRMC", 0) == 0) {
            std::vector<std::string> parts = split(nmea, ',');

            // Uma sentença RMC válida deve conter pelo menos 12 campos
            if (parts.size() < 12) {
                return;
            }

            auto fix_msg = sensor_msgs::msg::NavSatFix();
            fix_msg.header.stamp = this->now();
            fix_msg.header.frame_id = "gps_link";

            // Status: 'A' = Ativo (com fix de satélite), 'V' = Void (sem fix)
            std::string status = parts[2];

            if (status == "A") {
                fix_msg.status.status = sensor_msgs::msg::NavSatStatus::STATUS_FIX;

                std::string lat_val = parts[3];  // Ex: 4042.6142
                std::string lat_dir = parts[4];  // Ex: N
                std::string lon_val = parts[5];  // Ex: 07400.4168
                std::string lon_dir = parts[6];  // Ex: W

                fix_msg.latitude  = convert_to_decimal_degrees(lat_val, lat_dir);
                fix_msg.longitude = convert_to_decimal_degrees(lon_val, lon_dir);
                fix_msg.altitude  = 0.0; // RMC não fornece altitude

                RCLCPP_INFO(this->get_logger(), "GPS FIX ATIVO! Lat: %.6f | Lon: %.6f",
                            fix_msg.latitude, fix_msg.longitude);
            } else {
                fix_msg.status.status = sensor_msgs::msg::NavSatStatus::STATUS_NO_FIX;
                fix_msg.latitude  = 0.0;
                fix_msg.longitude = 0.0;
                fix_msg.altitude  = 0.0;

                // Evita estourar o terminal com mensagens repetitivas usando logs limitados por tempo
                RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 5000,
                                     "GPS sem sinal estável! Coloque o robô próximo a uma janela para obter o 'Fix' (LED vermelho piscando a cada 15s).");
            }

            pub_->publish(fix_msg);
        }
    }
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<GpsLogicClient>());
    rclcpp::shutdown();
    return 0;
}
