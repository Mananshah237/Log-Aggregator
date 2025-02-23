#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <fstream>
#include <sstream>
#include <zlib.h>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <rdkafka4cpp.h>
#include "log_generator.h"

class LogAggregator {
private:
    Aws::S3::S3Client s3_client_;
    RdKafka::Producer* kafka_producer_;
    std::mutex mtx_;
    std::vector<std::string> logs_;
    const std::string bucket_ = "manan-log-bucket"; // Replace with your S3 bucket
    const std::string topic_ = "logs";

    std::string compress(const std::string& data) {
        z_stream zs{};
        zs.zalloc = Z_NULL;
        zs.zfree = Z_NULL;
        zs.opaque = Z_NULL;
        deflateInit(&zs, Z_DEFAULT_COMPRESSION);
        zs.avail_in = data.size();
        zs.next_in = (Bytef*)data.data();
        
        std::string out;
        char buffer[1024];
        do {
            zs.avail_out = sizeof(buffer);
            zs.next_out = (Bytef*)buffer;
            deflate(&zs, Z_FINISH);
            out.append(buffer, sizeof(buffer) - zs.avail_out);
        } while (zs.avail_out == 0);
        deflateEnd(&zs);
        return out;
    }

public:
    LogAggregator() {
        Aws::Client::ClientConfiguration config;
        config.region = "us-east-1"; // Adjust your region
        s3_client_ = Aws::S3::S3Client(config);

        RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
        conf->set("bootstrap.servers", "localhost:9092", errstr_);
        kafka_producer_ = RdKafka::Producer::create(conf, errstr_);
        if (!kafka_producer_) {
            std::cerr << "Kafka producer init failed: " << errstr_ << std::endl;
            exit(1);
        }
        delete conf;
    }

    ~LogAggregator() { delete kafka_producer_; }

    void process_node(int node_id) {
        LogGenerator gen;
        for (int i = 0; i < 100000; ++i) { // Simulate 100K logs per node
            std::string log = gen.generate_log(node_id);
            {
                std::lock_guard<std::mutex> lock(mtx_);
                logs_.push_back(log);
            }
            kafka_producer_->produce(topic_, RdKafka::Partitioner::RD_KAFKA_PARTITION_UA,
                                     RdKafka::Producer::RK_MSG_COPY, log.data(), log.size(),
                                     nullptr, nullptr);
            std::this_thread::sleep_for(std::chrono::microseconds(100)); // Simulate real-time
        }
    }

    void upload_to_s3() {
        std::stringstream ss;
        for (const auto& log : logs_) ss << log << "\n";
        std::string compressed = compress(ss.str());

        Aws::S3::Model::PutObjectRequest request;
        request.SetBucket(bucket_);
        request.SetKey("logs/aggregated_logs.gz");
        auto input_data = Aws::MakeShared<Aws::StringStream>("LogData", compressed);
        request.SetBody(input_data);

        auto outcome = s3_client_.PutObject(request);
        if (!outcome.IsSuccess()) {
            std::cerr << "S3 upload failed: " << outcome.GetError().GetMessage() << std::endl;
        } else {
            std::cout << "Uploaded " << logs_.size() << " logs to S3" << std::endl;
        }
    }

private:
    std::string errstr_;
};

int main() {
    Aws::SDKOptions options;
    Aws::InitAPI(options);

    LogAggregator aggregator;
    std::vector<std::thread> nodes;
    for (int i = 1; i <= 10; ++i) {
        nodes.emplace_back(&LogAggregator::process_node, &aggregator, i);
    }
    for (auto& node : nodes) node.join();

    aggregator.upload_to_s3();

    Aws::ShutdownAPI(options);
    return 0;
}
