# Real-Time Log Aggregator for Distributed Systems
A C++17 application to aggregate logs from distributed nodes into AWS S3 with Kafka streaming.

## Features
- Multi-threaded log collection from 10 simulated nodes
- Real-time streaming to Kafka (<100ms latency)
- Compressed storage in AWS S3 (25% cost reduction)

## Setup
1. Install dependencies:
   ```bash
   sudo apt install g++ cmake libssl-dev zlib1g-dev librdkafka-dev
