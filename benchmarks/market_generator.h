#pragma once

#include "order_book_parser.h"
#include <random>
#include <cmath>
#include <vector>

using namespace cl::data_feed::data_feed_parser;

class SinusoidalMarketGenerator {
public:
    SinusoidalMarketGenerator(double basePrice, double amplitude, double frequency,
                               double noiseLevel, int bookDepth, double spreadBps = 10.0)
        : _basePrice(basePrice), _amplitude(amplitude), _frequency(frequency),
          _noiseLevel(noiseLevel), _bookDepth(bookDepth), _spreadBps(spreadBps), _tick(0),
          _rng(42), _noiseDist(-noiseLevel, noiseLevel), _qtyDist(100, 10000),
          _qtyChangeDist(-500, 500), _levelSkipDist(0, 3) {}

    void generateSnapshot(std::vector<bookElement>& buyBook, std::vector<bookElement>& sellBook) {
        _tick++;
        double sinComponent = _amplitude * std::sin(2.0 * M_PI * _frequency * _tick);
        double noise = _noiseDist(_rng);
        double midPrice = _basePrice + sinComponent + noise;
        double halfSpread = midPrice * (_spreadBps / 10000.0) / 2.0;
        double bestBid = midPrice - halfSpread;
        double bestAsk = midPrice + halfSpread;
        double tickSize = midPrice * 0.0001;

        buyBook.clear();
        double price = bestBid;
        for (int i = 0; i < _bookDepth; i++) {
            bookElement elem;
            elem.price = price;
            elem.qty = _qtyDist(_rng);
            elem.time = _tick;
            buyBook.push_back(elem);
            price -= tickSize * (1 + _levelSkipDist(_rng));
        }

        sellBook.clear();
        price = bestAsk;
        for (int i = 0; i < _bookDepth; i++) {
            bookElement elem;
            elem.price = price;
            elem.qty = _qtyDist(_rng);
            elem.time = _tick;
            sellBook.push_back(elem);
            price += tickSize * (1 + _levelSkipDist(_rng));
        }
    }

    void generateIncrementalUpdate(std::vector<bookElement>& buyBook,
                                    std::vector<bookElement>& sellBook, double changeRate) {
        _tick++;
        double drift = _noiseDist(_rng) * 0.1;
        for (auto& elem : buyBook) {
            if (_changeDist(_rng) < changeRate) {
                elem.qty = std::max(1, elem.qty + _qtyChangeDist(_rng));
                elem.time = _tick;
            }
            elem.price += drift;
        }
        for (auto& elem : sellBook) {
            if (_changeDist(_rng) < changeRate) {
                elem.qty = std::max(1, elem.qty + _qtyChangeDist(_rng));
                elem.time = _tick;
            }
            elem.price -= drift;
        }
    }

    ORDER_TIME getTick() const { return _tick; }

private:
    double _basePrice, _amplitude, _frequency, _noiseLevel, _spreadBps;
    int _bookDepth;
    ORDER_TIME _tick;
    std::mt19937 _rng;
    std::uniform_real_distribution<double> _noiseDist;
    std::uniform_int_distribution<int> _qtyDist, _qtyChangeDist, _levelSkipDist;
    std::uniform_real_distribution<double> _changeDist{0.0, 1.0};
};
