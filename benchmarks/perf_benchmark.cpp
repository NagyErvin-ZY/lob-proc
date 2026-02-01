#include "market_generator.h"
#include <benchmark/benchmark.h>

using namespace cl::data_feed::data_feed_parser;

// Full snapshot benchmark (parameterized by book depth)
static void BM_FullSnapshot(benchmark::State& state) {
    int depth = state.range(0);
    SinusoidalMarketGenerator gen(100.0, 5.0, 0.001, 0.5, depth);
    SeekerNetBoonSnapshotParserToTBT parser({1});
    std::vector<bookElement> buyBook, sellBook;

    // Warmup
    for (int i = 0; i < 50; i++) {
        gen.generateSnapshot(buyBook, sellBook);
        parser.EmitOrdersAndUpdateOldBuyBook(1, buyBook, gen.getTick());
        parser.EmitOrdersAndUpdateOldSellBook(1, sellBook, gen.getTick());
        parser.clearEmittedOrders();
    }

    for (auto _ : state) {
        gen.generateSnapshot(buyBook, sellBook);
        parser.EmitOrdersAndUpdateOldBuyBook(1, buyBook, gen.getTick());
        parser.EmitOrdersAndUpdateOldSellBook(1, sellBook, gen.getTick());
        benchmark::DoNotOptimize(parser.getEmittedOrders().size());
        parser.clearEmittedOrders();
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["depth"] = depth;
}

BENCHMARK(BM_FullSnapshot)
    ->Arg(5)->Arg(10)->Arg(20)->Arg(50)->Arg(100)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(2.0);

// Incremental update benchmark (parameterized by churn rate * 100)
static void BM_IncrementalUpdate(benchmark::State& state) {
    double changeRate = state.range(0) / 100.0;
    int depth = 20;
    SinusoidalMarketGenerator gen(100.0, 5.0, 0.001, 0.5, depth);
    SeekerNetBoonSnapshotParserToTBT parser({1});
    std::vector<bookElement> buyBook, sellBook;

    gen.generateSnapshot(buyBook, sellBook);

    // Warmup
    for (int i = 0; i < 50; i++) {
        gen.generateIncrementalUpdate(buyBook, sellBook, changeRate);
        auto buyCopy = buyBook, sellCopy = sellBook;
        parser.EmitOrdersAndUpdateOldBuyBook(1, buyCopy, gen.getTick());
        parser.EmitOrdersAndUpdateOldSellBook(1, sellCopy, gen.getTick());
        parser.clearEmittedOrders();
    }

    for (auto _ : state) {
        gen.generateIncrementalUpdate(buyBook, sellBook, changeRate);
        auto buyCopy = buyBook, sellCopy = sellBook;
        parser.EmitOrdersAndUpdateOldBuyBook(1, buyCopy, gen.getTick());
        parser.EmitOrdersAndUpdateOldSellBook(1, sellCopy, gen.getTick());
        benchmark::DoNotOptimize(parser.getEmittedOrders().size());
        parser.clearEmittedOrders();
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["churn%"] = state.range(0);
}

BENCHMARK(BM_IncrementalUpdate)
    ->Arg(10)->Arg(30)->Arg(50)->Arg(70)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(2.0);

// Market order benchmark
static void BM_MarketOrder(benchmark::State& state) {
    int depth = state.range(0);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> qtyDist(1, 100);
    std::uniform_int_distribution<int> sideDist(0, 1);

    SinusoidalMarketGenerator gen(100.0, 5.0, 0.001, 0.5, depth);
    SeekerNetBoonSnapshotParserToTBT parser({1});
    std::vector<bookElement> buyBook, sellBook;
    ORDER_TIME tick = 0;

    gen.generateSnapshot(buyBook, sellBook);
    parser.EmitOrdersAndUpdateOldBuyBook(1, buyBook, 1);
    parser.EmitOrdersAndUpdateOldSellBook(1, sellBook, 1);
    parser.clearEmittedOrders();

    for (auto _ : state) {
        tick++;
        if (parser.getBuySide(1).empty() || parser.getSellSide(1).empty()) {
            state.PauseTiming();
            gen.generateSnapshot(buyBook, sellBook);
            parser.EmitOrdersAndUpdateOldBuyBook(1, buyBook, tick);
            parser.EmitOrdersAndUpdateOldSellBook(1, sellBook, tick);
            parser.clearEmittedOrders();
            state.ResumeTiming();
        }

        int qty = qtyDist(rng);
        if (sideDist(rng) == 0 && !parser.getBuySide(1).empty()) {
            parser.EmitMarketOrderAndUpdateBuyBook(1, qty, parser.getBuySide(1)[0].price, tick);
        } else if (!parser.getSellSide(1).empty()) {
            parser.EmitMarketOrderAndUpdateSellBook(1, qty, parser.getSellSide(1)[0].price, tick);
        }
        benchmark::DoNotOptimize(parser.getEmittedOrders().size());
        parser.clearEmittedOrders();
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["depth"] = depth;
}

BENCHMARK(BM_MarketOrder)
    ->Arg(10)->Arg(50)->Arg(100)
    ->Unit(benchmark::kNanosecond)
    ->MinTime(2.0);

// Mixed workload: snapshots + market orders interleaved
static void BM_MixedWorkload(benchmark::State& state) {
    int depth = 20;
    SinusoidalMarketGenerator gen(100.0, 5.0, 0.001, 0.5, depth);
    SeekerNetBoonSnapshotParserToTBT parser({1});
    std::vector<bookElement> buyBook, sellBook;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> qtyDist(1, 100);
    ORDER_TIME tick = 0;

    gen.generateSnapshot(buyBook, sellBook);
    parser.EmitOrdersAndUpdateOldBuyBook(1, buyBook, 1);
    parser.EmitOrdersAndUpdateOldSellBook(1, sellBook, 1);
    parser.clearEmittedOrders();

    int cycle = 0;
    for (auto _ : state) {
        tick++;
        cycle++;

        // Every 10th operation is a snapshot update, rest are market orders
        if (cycle % 10 == 0) {
            gen.generateSnapshot(buyBook, sellBook);
            parser.EmitOrdersAndUpdateOldBuyBook(1, buyBook, tick);
            parser.EmitOrdersAndUpdateOldSellBook(1, sellBook, tick);
        } else {
            if (!parser.getBuySide(1).empty()) {
                parser.EmitMarketOrderAndUpdateBuyBook(1, qtyDist(rng),
                    parser.getBuySide(1)[0].price, tick);
            }
        }
        benchmark::DoNotOptimize(parser.getEmittedOrders().size());
        parser.clearEmittedOrders();
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_MixedWorkload)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(3.0);

BENCHMARK_MAIN();
