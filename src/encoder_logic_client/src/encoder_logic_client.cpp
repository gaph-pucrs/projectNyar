#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int32.hpp>
#include <vector>

class EncoderLogicClient : public rclcpp::Node {
public:
    EncoderLogicClient() : Node("encoder_logic_client"), state_index_(0), tick_counter_(0), is_synced_(false) {
        sub_ = this->create_subscription<std_msgs::msg::Int32>(
            "/sensor/encoder/raw_state", rclcpp::SensorDataQoS(),
            std::bind(&EncoderLogicClient::raw_callback, this, std::placeholders::_1));

        pub_ = this->create_publisher<std_msgs::msg::Int32>("/sensor/encoder/ticks", 10);

        RCLCPP_INFO(this->get_logger(), "Processador Lógico C++ do Encoder Inicializado na Workstation!");
    }

private:
    const std::vector<int> states_ = {0b00, 0b10, 0b11, 0b01};
    size_t state_index_;
    int tick_counter_;
    bool is_synced_; // Flag para a primeira leitura

    rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr sub_;
    rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr pub_;

    void raw_callback(const std_msgs::msg::Int32::SharedPtr msg) {
        int pair = msg->data;

        // 1. Sincroniza a posição inicial na primeira vez que recebe um dado
        if (!is_synced_) {
            for (size_t i = 0; i < states_.size(); ++i) {
                if (states_[i] == pair) {
                    state_index_ = i;
                    is_synced_ = true;
                    RCLCPP_INFO(this->get_logger(), "Sincronizado no estado inicial: 0b%02b", pair);
                    return;
                }
            }
        }

        size_t cw_idx = (state_index_ + 1) % 4;
        size_t ccw_idx = (state_index_ + 3) % 4;

        if (pair != states_[state_index_]) {
            if (pair == states_[cw_idx]) {
                state_index_ = cw_idx;
                if (pair == 0b11 || pair == 0b00) {
                    tick_counter_++;
                    publish_ticks();
                }
            }
            else if (pair == states_[ccw_idx]) {
                state_index_ = ccw_idx;
                if (pair == 0b11 || pair == 0b00) {
                    tick_counter_--;
                    publish_ticks();
                }
            }
            else {
                // 2. Proteção contra perda de pacotes da rede Best Effort
                RCLCPP_WARN(this->get_logger(),
                    "PULO DE ESTADO DETECTADO! (Pacote Perdido). Esperado: 0b%02b ou 0b%02b, Recebido: 0b%02b",
                    states_[cw_idx], states_[ccw_idx], pair);

                // Força a ressincronização para não travar o nó para sempre
                for (size_t i = 0; i < states_.size(); ++i) {
                    if (states_[i] == pair) {
                        state_index_ = i;
                        break;
                    }
                }
            }
        }
    }

    void publish_ticks() {
        auto msg = std_msgs::msg::Int32();
        msg.data = tick_counter_;
        pub_->publish(msg);
        RCLCPP_INFO(this->get_logger(), "Ticks Consolidados: %d", tick_counter_);
    }
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<EncoderLogicClient>());
    rclcpp::shutdown();
    return 0;
}
