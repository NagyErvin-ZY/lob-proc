#include "order_book_parser.h"
#include "src/wire_format.h"
#include <nats.h>
#include <cstdio>
#include <csignal>
#include <atomic>

using namespace cl::data_feed::data_feed_parser;

static std::atomic<bool> g_running(true);

static void signalHandler(int) {
    g_running.store(false);
}

static void onMessage(natsConnection* nc, natsSubscription*, natsMsg* msg, void* closure) {
    auto* parser = static_cast<SeekerNetBoonSnapshotParserToTBT*>(closure);

    const char* data = natsMsg_GetData(msg);
    int dataLen = natsMsg_GetDataLength(msg);

    PAIR_ID pairId;
    ORDER_TIME timestamp;
    std::vector<bookElement> buyBook, sellBook;

    if (!deserializeSnapshot(data, static_cast<size_t>(dataLen), pairId, timestamp, buyBook, sellBook)) {
        fprintf(stderr, "Failed to deserialize snapshot (%d bytes)\n", dataLen);
        natsMsg_Destroy(msg);
        return;
    }

    parser->EmitOrdersAndUpdateOldBuyBook(pairId, buyBook, timestamp);
    parser->EmitOrdersAndUpdateOldSellBook(pairId, sellBook, timestamp);

    const auto& orders = parser->getEmittedOrders();
    if (!orders.empty()) {
        std::vector<char> outBuf = serializeOrders(orders);
        natsStatus s = natsConnection_Publish(nc, "orderbook.tbt",
            outBuf.data(), static_cast<int>(outBuf.size()));
        if (s != NATS_OK) {
            fprintf(stderr, "Publish error: %s\n", natsStatus_GetText(s));
        }
    }

    parser->clearEmittedOrders();
    natsMsg_Destroy(msg);
}

int main() {
    signal(SIGINT, signalHandler);

    natsOptions* opts = NULL;
    natsConnection* conn = NULL;
    natsSubscription* sub = NULL;

    const char* nats_url = getenv("NATS_URL");
    if (!nats_url) nats_url = "nats://localhost:4222";

    natsStatus s = natsOptions_Create(&opts);
    if (s == NATS_OK) s = natsOptions_SetURL(opts, nats_url);
    if (s != NATS_OK) {
        fprintf(stderr, "Options error: %s\n", natsStatus_GetText(s));
        return 1;
    }

    s = natsConnection_Connect(&conn, opts);
    if (s != NATS_OK) {
        fprintf(stderr, "Connect error: %s\n", natsStatus_GetText(s));
        natsOptions_Destroy(opts);
        return 1;
    }
    printf("Connected to NATS\n");

    SeekerNetBoonSnapshotParserToTBT parser({1});

    s = natsConnection_Subscribe(&sub, conn, "orderbook.snapshots", onMessage, &parser);
    if (s != NATS_OK) {
        fprintf(stderr, "Subscribe error: %s\n", natsStatus_GetText(s));
        natsConnection_Destroy(conn);
        natsOptions_Destroy(opts);
        return 1;
    }
    printf("Subscribed to orderbook.snapshots, publishing to orderbook.tbt\n");

    while (g_running.load()) {
        nats_Sleep(100);
    }

    printf("\nShutting down...\n");
    natsSubscription_Destroy(sub);
    natsConnection_Destroy(conn);
    natsOptions_Destroy(opts);
    return 0;
}
