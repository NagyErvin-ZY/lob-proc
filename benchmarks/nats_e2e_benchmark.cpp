#include "market_generator.h"
#include "src/wire_format.h"
#include <nats.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>

using namespace cl::data_feed::data_feed_parser;
using Clock = std::chrono::high_resolution_clock;

struct LatencyStats {
    std::mutex mu;
    std::condition_variable cv;
    std::vector<double> latenciesUs;
    uint64_t received = 0;

    void record(double latencyUs) {
        std::lock_guard<std::mutex> lock(mu);
        latenciesUs.push_back(latencyUs);
        received++;
        cv.notify_one();
    }

    void waitFor(uint64_t count) {
        std::unique_lock<std::mutex> lock(mu);
        cv.wait(lock, [&]{ return received >= count; });
    }
};

struct ThroughputStats {
    std::atomic<uint64_t> received{0};
    std::atomic<uint64_t> bytesReceived{0};
};

static void onLatencyMsg(natsConnection*, natsSubscription*, natsMsg* msg, void* closure) {
    auto now = Clock::now();
    auto* stats = static_cast<LatencyStats*>(closure);
    const char* data = natsMsg_GetData(msg);
    int dataLen = natsMsg_GetDataLength(msg);

    if (dataLen >= static_cast<int>(sizeof(uint64_t))) {
        uint64_t sendTimeNs;
        std::memcpy(&sendTimeNs, data, sizeof(uint64_t));
        double latencyUs = std::chrono::duration<double, std::micro>(
            now.time_since_epoch() - std::chrono::nanoseconds(sendTimeNs)).count();
        stats->record(latencyUs);
    }
    natsMsg_Destroy(msg);
}

static void onThroughputMsg(natsConnection*, natsSubscription*, natsMsg* msg, void* closure) {
    auto* stats = static_cast<ThroughputStats*>(closure);
    stats->received.fetch_add(1, std::memory_order_relaxed);
    stats->bytesReceived.fetch_add(natsMsg_GetDataLength(msg), std::memory_order_relaxed);
    natsMsg_Destroy(msg);
}

static double percentile(std::vector<double>& sorted, double p) {
    if (sorted.empty()) return 0.0;
    double idx = p / 100.0 * (sorted.size() - 1);
    size_t lo = static_cast<size_t>(idx);
    size_t hi = lo + 1;
    if (hi >= sorted.size()) return sorted.back();
    double frac = idx - lo;
    return sorted[lo] * (1.0 - frac) + sorted[hi] * frac;
}

int main(int argc, char* argv[]) {
    int durationSec = 5;
    int depth = 20;
    int latencyRounds = 10000;
    if (argc > 1) durationSec = atoi(argv[1]);
    if (argc > 2) depth = atoi(argv[2]);
    if (argc > 3) latencyRounds = atoi(argv[3]);

    printf("NATS E2E Benchmark\n");
    printf("  duration=%ds  depth=%d  latency_rounds=%d\n\n", durationSec, depth, latencyRounds);

    // Connect
    natsOptions* opts = NULL;
    natsConnection* conn = NULL;
    natsStatus s = natsOptions_Create(&opts);
    if (s == NATS_OK) s = natsOptions_SetURL(opts, "nats://localhost:4222");
    if (s != NATS_OK) {
        fprintf(stderr, "Options error: %s\n", natsStatus_GetText(s));
        return 1;
    }

    s = natsConnection_Connect(&conn, opts);
    if (s != NATS_OK) {
        fprintf(stderr, "Connect error: %s\n", natsStatus_GetText(s));
        fprintf(stderr, "Start NATS: nats-server or docker compose up -d\n");
        natsOptions_Destroy(opts);
        return 1;
    }
    printf("Connected to NATS at localhost:4222\n\n");

    SinusoidalMarketGenerator gen(100.0, 5.0, 0.001, 0.5, depth);
    SeekerNetBoonSnapshotParserToTBT parser({1});
    std::vector<bookElement> buyBook, sellBook;

    // Warmup parser so first snapshot isn't add-all
    for (int i = 0; i < 10; i++) {
        gen.generateSnapshot(buyBook, sellBook);
        parser.EmitOrdersAndUpdateOldBuyBook(1, buyBook, gen.getTick());
        parser.EmitOrdersAndUpdateOldSellBook(1, sellBook, gen.getTick());
        parser.clearEmittedOrders();
    }

    // ============================================================
    // PHASE 1: Latency (one message at a time, flush each, no queuing)
    // ============================================================
    printf("--- Phase 1: Latency (%d rounds, flush per message) ---\n", latencyRounds);

    LatencyStats latStats;
    natsSubscription* latSub = NULL;
    s = natsConnection_Subscribe(&latSub, conn, "bench.lat.resp", onLatencyMsg, &latStats);
    if (s != NATS_OK) {
        fprintf(stderr, "Subscribe error: %s\n", natsStatus_GetText(s));
        natsConnection_Destroy(conn);
        natsOptions_Destroy(opts);
        return 1;
    }

    for (int i = 0; i < latencyRounds; i++) {
        gen.generateSnapshot(buyBook, sellBook);
        parser.EmitOrdersAndUpdateOldBuyBook(1, buyBook, gen.getTick());
        parser.EmitOrdersAndUpdateOldSellBook(1, sellBook, gen.getTick());

        const auto& orders = parser.getEmittedOrders();
        std::vector<char> orderBuf = serializeOrders(orders);

        // Stamp send time
        auto now = Clock::now();
        uint64_t sendTimeNs = static_cast<uint64_t>(now.time_since_epoch().count());

        std::vector<char> msg(sizeof(uint64_t) + orderBuf.size());
        std::memcpy(msg.data(), &sendTimeNs, sizeof(uint64_t));
        std::memcpy(msg.data() + sizeof(uint64_t), orderBuf.data(), orderBuf.size());

        natsConnection_Publish(conn, "bench.lat.resp",
            msg.data(), static_cast<int>(msg.size()));
        natsConnection_Flush(conn);

        parser.clearEmittedOrders();
    }

    // Wait for all latency messages
    latStats.waitFor(latencyRounds);
    natsSubscription_Destroy(latSub);

    {
        std::lock_guard<std::mutex> lock(latStats.mu);
        std::sort(latStats.latenciesUs.begin(), latStats.latenciesUs.end());
        printf("  samples:  %zu\n", latStats.latenciesUs.size());
        printf("  min:      %.1f us\n", latStats.latenciesUs.front());
        printf("  p50:      %.1f us\n", percentile(latStats.latenciesUs, 50));
        printf("  p95:      %.1f us\n", percentile(latStats.latenciesUs, 95));
        printf("  p99:      %.1f us\n", percentile(latStats.latenciesUs, 99));
        printf("  max:      %.1f us\n", latStats.latenciesUs.back());
    }

    // ============================================================
    // PHASE 2: Throughput (flood for durationSec, no flush)
    // ============================================================
    printf("\n--- Phase 2: Throughput (flood for %ds) ---\n", durationSec);

    ThroughputStats tpStats;
    natsSubscription* tpSub = NULL;
    s = natsConnection_Subscribe(&tpSub, conn, "bench.tp", onThroughputMsg, &tpStats);
    if (s != NATS_OK) {
        fprintf(stderr, "Subscribe error: %s\n", natsStatus_GetText(s));
        natsConnection_Destroy(conn);
        natsOptions_Destroy(opts);
        return 1;
    }

    // Pre-generate a batch of messages to remove generation overhead from measurement
    const int batchSize = 1000;
    std::vector<std::vector<char>> pregenMsgs;
    pregenMsgs.reserve(batchSize);
    for (int i = 0; i < batchSize; i++) {
        gen.generateSnapshot(buyBook, sellBook);
        parser.EmitOrdersAndUpdateOldBuyBook(1, buyBook, gen.getTick());
        parser.EmitOrdersAndUpdateOldSellBook(1, sellBook, gen.getTick());
        std::vector<char> buf = serializeOrders(parser.getEmittedOrders());
        pregenMsgs.push_back(std::move(buf));
        parser.clearEmittedOrders();
    }

    size_t msgIdx = 0;
    uint64_t published = 0;
    uint64_t publishedBytes = 0;

    auto tpStart = Clock::now();
    auto deadline = tpStart + std::chrono::seconds(durationSec);

    while (Clock::now() < deadline) {
        const auto& buf = pregenMsgs[msgIdx % batchSize];
        natsStatus ps = natsConnection_Publish(conn, "bench.tp",
            buf.data(), static_cast<int>(buf.size()));
        if (ps == NATS_OK) {
            published++;
            publishedBytes += buf.size();
        }
        msgIdx++;
    }

    natsConnection_Flush(conn);
    auto tpElapsed = Clock::now() - tpStart;
    double tpSec = std::chrono::duration<double>(tpElapsed).count();

    // Give subscriber a moment to drain
    nats_Sleep(500);
    natsSubscription_Destroy(tpSub);

    uint64_t recvd = tpStats.received.load();
    uint64_t recvdBytes = tpStats.bytesReceived.load();

    printf("  published:   %llu msgs\n", (unsigned long long)published);
    printf("  received:    %llu msgs\n", (unsigned long long)recvd);
    printf("  duration:    %.2f s\n", tpSec);
    printf("  pub ops/s:   %.0f\n", published / tpSec);
    printf("  recv ops/s:  %.0f\n", recvd / tpSec);
    printf("  pub MB/s:    %.2f\n", (publishedBytes / (1024.0 * 1024.0)) / tpSec);
    printf("  recv MB/s:   %.2f\n", (recvdBytes / (1024.0 * 1024.0)) / tpSec);

    // ============================================================
    // PHASE 3: Processing throughput (generate + parse + serialize + publish)
    // ============================================================
    printf("\n--- Phase 3: Full pipeline throughput (gen+parse+serialize+pub, %ds) ---\n", durationSec);

    ThroughputStats pipeStats;
    natsSubscription* pipeSub = NULL;
    s = natsConnection_Subscribe(&pipeSub, conn, "bench.pipe", onThroughputMsg, &pipeStats);
    if (s != NATS_OK) {
        fprintf(stderr, "Subscribe error: %s\n", natsStatus_GetText(s));
        natsConnection_Destroy(conn);
        natsOptions_Destroy(opts);
        return 1;
    }

    uint64_t pipePublished = 0;
    uint64_t pipeBytes = 0;

    auto pipeStart = Clock::now();
    auto pipeDeadline = pipeStart + std::chrono::seconds(durationSec);

    while (Clock::now() < pipeDeadline) {
        gen.generateSnapshot(buyBook, sellBook);
        parser.EmitOrdersAndUpdateOldBuyBook(1, buyBook, gen.getTick());
        parser.EmitOrdersAndUpdateOldSellBook(1, sellBook, gen.getTick());

        const auto& orders = parser.getEmittedOrders();
        std::vector<char> buf = serializeOrders(orders);

        natsStatus ps = natsConnection_Publish(conn, "bench.pipe",
            buf.data(), static_cast<int>(buf.size()));
        if (ps == NATS_OK) {
            pipePublished++;
            pipeBytes += buf.size();
        }
        parser.clearEmittedOrders();
    }

    natsConnection_Flush(conn);
    auto pipeElapsed = Clock::now() - pipeStart;
    double pipeSec = std::chrono::duration<double>(pipeElapsed).count();

    nats_Sleep(500);
    natsSubscription_Destroy(pipeSub);

    uint64_t pipeRecvd = pipeStats.received.load();
    uint64_t pipeRecvdBytes = pipeStats.bytesReceived.load();

    printf("  published:   %llu msgs\n", (unsigned long long)pipePublished);
    printf("  received:    %llu msgs\n", (unsigned long long)pipeRecvd);
    printf("  duration:    %.2f s\n", pipeSec);
    printf("  pub ops/s:   %.0f\n", pipePublished / pipeSec);
    printf("  recv ops/s:  %.0f\n", pipeRecvd / pipeSec);
    printf("  pub MB/s:    %.2f\n", (pipeBytes / (1024.0 * 1024.0)) / pipeSec);
    printf("  recv MB/s:   %.2f\n", (pipeRecvdBytes / (1024.0 * 1024.0)) / pipeSec);

    printf("\n================================================\n");

    natsConnection_Destroy(conn);
    natsOptions_Destroy(opts);
    return 0;
}
