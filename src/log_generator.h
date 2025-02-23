#ifndef LOG_GENERATOR_H
#define LOG_GENERATOR_H

#include <string>
#include <chrono>
#include <random>

class LogGenerator {
public:
    std::string generate_log(int node_id) {
        auto now = std::chrono::system_clock::now();
        std::time_t tt = std::chrono::system_clock::to_time_t(now);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 1000);
        return "Node" + std::to_string(node_id) + " [" + std::ctime(&tt) + "] EventID:" + std::to_string(dis(gen));
    }
};

#endif
