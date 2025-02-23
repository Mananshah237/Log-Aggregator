all: log_aggregator

log_aggregator: src/log_aggregator.cpp src/log_generator.h
	g++ -std=c++17 src/log_aggregator.cpp -o log_aggregator -laws_s3 -lrdkafka++ -lz -pthread

run: log_aggregator
	./log_aggregator

clean:
	rm -f log_aggregator
