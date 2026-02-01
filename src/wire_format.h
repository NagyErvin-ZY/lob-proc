#pragma once

#include "data_structures.h"
#include <vector>
#include <cstring>
#include <cstdint>
#include <atomic>

namespace cl {
namespace data_feed {
namespace data_feed_parser {

namespace wire_detail {
inline void write_u16_le(char* dst, uint16_t v) {
    dst[0] = static_cast<char>(v & 0xFF);
    dst[1] = static_cast<char>((v >> 8) & 0xFF);
}

inline void write_u32_le(char* dst, uint32_t v) {
    dst[0] = static_cast<char>(v & 0xFF);
    dst[1] = static_cast<char>((v >> 8) & 0xFF);
    dst[2] = static_cast<char>((v >> 16) & 0xFF);
    dst[3] = static_cast<char>((v >> 24) & 0xFF);
}

inline void write_u64_le(char* dst, uint64_t v) {
    for (int i = 0; i < 8; i++) {
        dst[i] = static_cast<char>((v >> (i * 8)) & 0xFF);
    }
}

inline uint16_t read_u16_le(const char* src) {
    return static_cast<uint16_t>(
        (static_cast<uint16_t>(static_cast<unsigned char>(src[0])) << 0) |
        (static_cast<uint16_t>(static_cast<unsigned char>(src[1])) << 8));
}

inline uint32_t read_u32_le(const char* src) {
    return static_cast<uint32_t>(
        (static_cast<uint32_t>(static_cast<unsigned char>(src[0])) << 0) |
        (static_cast<uint32_t>(static_cast<unsigned char>(src[1])) << 8) |
        (static_cast<uint32_t>(static_cast<unsigned char>(src[2])) << 16) |
        (static_cast<uint32_t>(static_cast<unsigned char>(src[3])) << 24));
}

inline uint64_t read_u64_le(const char* src) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) {
        v |= static_cast<uint64_t>(static_cast<unsigned char>(src[i])) << (i * 8);
    }
    return v;
}

inline void write_i32_le(char* dst, int32_t v) {
    write_u32_le(dst, static_cast<uint32_t>(v));
}

inline int32_t read_i32_le(const char* src) {
    return static_cast<int32_t>(read_u32_le(src));
}

inline void write_i64_le(char* dst, int64_t v) {
    write_u64_le(dst, static_cast<uint64_t>(v));
}

inline int64_t read_i64_le(const char* src) {
    return static_cast<int64_t>(read_u64_le(src));
}

inline void write_f64_le(char* dst, double v) {
    uint64_t bits = 0;
    std::memcpy(&bits, &v, sizeof(bits));
    write_u64_le(dst, bits);
}

inline double read_f64_le(const char* src) {
    uint64_t bits = read_u64_le(src);
    double v = 0.0;
    std::memcpy(&v, &bits, sizeof(v));
    return v;
}
} // namespace wire_detail

constexpr uint8_t WIRE_MSG_ORDERS = 1;
constexpr size_t WIRE_SNAPSHOT_HEADER_SIZE = 20; // pairId(8) + timestamp(8) + numBids(2) + numAsks(2)
constexpr size_t WIRE_BOOK_LEVEL_SIZE = 12;      // price(8) + qty(4)
constexpr size_t WIRE_ORDERS_HEADER_SIZE = 20;   // type(1) + pairId(4) + seq(8) + count(4) + pad(3)
constexpr size_t WIRE_ORDER_SIZE = 40;           // pairId(8) + price(8) + time(8) + qty(4) + side(4) + type(4) + action(4)

inline std::vector<char> serializeSnapshot(
    PAIR_ID pairId,
    ORDER_TIME timestamp,
    const std::vector<bookElement>& buyBook,
    const std::vector<bookElement>& sellBook)
{
    size_t numLevels = buyBook.size() + sellBook.size();
    size_t totalSize = WIRE_SNAPSHOT_HEADER_SIZE + numLevels * WIRE_BOOK_LEVEL_SIZE;
    std::vector<char> buf(totalSize);

    wire_detail::write_i64_le(buf.data() + 0, static_cast<int64_t>(pairId));
    wire_detail::write_u64_le(buf.data() + 8, static_cast<uint64_t>(timestamp));
    wire_detail::write_u16_le(buf.data() + 16, static_cast<uint16_t>(buyBook.size()));
    wire_detail::write_u16_le(buf.data() + 18, static_cast<uint16_t>(sellBook.size()));

    size_t offset = WIRE_SNAPSHOT_HEADER_SIZE;
    for (size_t i = 0; i < buyBook.size(); i++) {
        wire_detail::write_f64_le(buf.data() + offset, buyBook[i].price);
        wire_detail::write_i32_le(buf.data() + offset + 8, static_cast<int32_t>(buyBook[i].qty));
        offset += WIRE_BOOK_LEVEL_SIZE;
    }
    for (size_t i = 0; i < sellBook.size(); i++) {
        wire_detail::write_f64_le(buf.data() + offset, sellBook[i].price);
        wire_detail::write_i32_le(buf.data() + offset + 8, static_cast<int32_t>(sellBook[i].qty));
        offset += WIRE_BOOK_LEVEL_SIZE;
    }

    return buf;
}

inline bool deserializeSnapshot(
    const char* data,
    size_t len,
    PAIR_ID& pairId,
    ORDER_TIME& timestamp,
    std::vector<bookElement>& buyBook,
    std::vector<bookElement>& sellBook)
{
    if (len < WIRE_SNAPSHOT_HEADER_SIZE) return false;

    pairId = static_cast<PAIR_ID>(wire_detail::read_i64_le(data + 0));
    timestamp = static_cast<ORDER_TIME>(wire_detail::read_u64_le(data + 8));
    uint16_t numBids = wire_detail::read_u16_le(data + 16);
    uint16_t numAsks = wire_detail::read_u16_le(data + 18);

    size_t expectedSize = WIRE_SNAPSHOT_HEADER_SIZE +
        static_cast<size_t>(numBids + numAsks) * WIRE_BOOK_LEVEL_SIZE;
    if (len < expectedSize) return false;

    buyBook.resize(numBids);
    size_t offset = WIRE_SNAPSHOT_HEADER_SIZE;
    for (uint16_t i = 0; i < numBids; i++) {
        buyBook[i].price = wire_detail::read_f64_le(data + offset);
        buyBook[i].qty = wire_detail::read_i32_le(data + offset + 8);
        buyBook[i].time = timestamp;
        offset += WIRE_BOOK_LEVEL_SIZE;
    }

    sellBook.resize(numAsks);
    for (uint16_t i = 0; i < numAsks; i++) {
        sellBook[i].price = wire_detail::read_f64_le(data + offset);
        sellBook[i].qty = wire_detail::read_i32_le(data + offset + 8);
        sellBook[i].time = timestamp;
        offset += WIRE_BOOK_LEVEL_SIZE;
    }

    return true;
}

inline std::vector<char> serializeOrders(const std::vector<Order>& orders) {
    uint32_t count = static_cast<uint32_t>(orders.size());
    size_t totalSize = WIRE_ORDERS_HEADER_SIZE + static_cast<size_t>(count) * WIRE_ORDER_SIZE;
    std::vector<char> buf(totalSize, 0);

    uint32_t wirePairId = 0;
    if (count > 0) {
        wirePairId = static_cast<uint32_t>(orders[0].pairId);
    }

    static std::atomic<uint64_t> sequence{1};
    uint64_t seq = sequence.fetch_add(1, std::memory_order_relaxed);

    buf[0] = static_cast<char>(WIRE_MSG_ORDERS);
    wire_detail::write_u32_le(buf.data() + 1, wirePairId);
    wire_detail::write_u64_le(buf.data() + 5, seq);
    wire_detail::write_u32_le(buf.data() + 13, count);

    size_t offset = WIRE_ORDERS_HEADER_SIZE;
    for (const auto& order : orders) {
        wire_detail::write_i64_le(buf.data() + offset + 0, static_cast<int64_t>(order.pairId));
        wire_detail::write_f64_le(buf.data() + offset + 8, order.price);
        wire_detail::write_u64_le(buf.data() + offset + 16, static_cast<uint64_t>(order.time));
        wire_detail::write_i32_le(buf.data() + offset + 24, static_cast<int32_t>(order.qty));
        wire_detail::write_i32_le(buf.data() + offset + 28, static_cast<int32_t>(order.side));
        wire_detail::write_i32_le(buf.data() + offset + 32, static_cast<int32_t>(order.type));
        wire_detail::write_i32_le(buf.data() + offset + 36, static_cast<int32_t>(order.action));
        offset += WIRE_ORDER_SIZE;
    }

    return buf;
}

} // namespace data_feed_parser
} // namespace data_feed
} // namespace cl
