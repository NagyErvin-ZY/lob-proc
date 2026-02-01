package main

import (
	"encoding/binary"
	"log"
	"math"
	"math/rand"
	"os"
	"strings"
	"sync/atomic"
	"time"

	"github.com/nats-io/nats.go"
)

type MarketGenerator struct {
	basePrice float64
	amplitude float64
	frequency float64
	noise     float64
	depth     int
	spreadBps float64
	tickSize  float64
	tick      uint64
	rng       *rand.Rand

	// Persistent book state for smooth evolution
	prevBids []BookLevel
	prevAsks []BookLevel
}

var orderSeq uint64

func NewMarketGenerator(basePrice, amplitude, frequency, noise float64, depth int, spreadBps float64) *MarketGenerator {
	return &MarketGenerator{
		basePrice: basePrice,
		amplitude: amplitude,
		frequency: frequency,
		noise:     noise,
		depth:     depth,
		spreadBps: spreadBps,
		tickSize:  math.Round(basePrice*0.0001*100) / 100, // round to cent
		rng:       rand.New(rand.NewSource(42)),
	}
}

type BookLevel struct {
	Price float64
	Qty   int32
}

func roundToTick(price, tickSize float64) float64 {
	return math.Round(price/tickSize) * tickSize
}

func (g *MarketGenerator) GenerateSnapshot() (bids []BookLevel, asks []BookLevel) {
	g.tick++
	sinComponent := g.amplitude * math.Sin(2.0*math.Pi*g.frequency*float64(g.tick))
	// Smoothed noise: small drift each tick instead of random jumps
	noise := (g.rng.Float64()*2 - 1) * g.noise * 0.1
	midPrice := g.basePrice + sinComponent + noise
	halfSpread := midPrice * (g.spreadBps / 10000.0) / 2.0
	bestBid := roundToTick(midPrice-halfSpread, g.tickSize)
	bestAsk := roundToTick(midPrice+halfSpread, g.tickSize)
	if bestAsk <= bestBid {
		bestAsk = bestBid + g.tickSize
	}

	bids = make([]BookLevel, g.depth)
	for i := 0; i < g.depth; i++ {
		price := bestBid - float64(i)*g.tickSize
		qty := int32(500 + g.rng.Intn(4500))
		// If we have previous state, evolve smoothly
		if len(g.prevBids) > 0 {
			// Find matching price in prev
			for _, pb := range g.prevBids {
				if math.Abs(pb.Price-price) < g.tickSize*0.5 {
					// Evolve qty: keep 80% of prev + 20% new random
					qty = int32(float64(pb.Qty)*0.8 + float64(qty)*0.2)
					break
				}
			}
		}
		if qty < 100 {
			qty = 100
		}
		bids[i] = BookLevel{Price: math.Round(price*100) / 100, Qty: qty}
	}

	asks = make([]BookLevel, g.depth)
	for i := 0; i < g.depth; i++ {
		price := bestAsk + float64(i)*g.tickSize
		qty := int32(500 + g.rng.Intn(4500))
		if len(g.prevAsks) > 0 {
			for _, pa := range g.prevAsks {
				if math.Abs(pa.Price-price) < g.tickSize*0.5 {
					qty = int32(float64(pa.Qty)*0.8 + float64(qty)*0.2)
					break
				}
			}
		}
		if qty < 100 {
			qty = 100
		}
		asks[i] = BookLevel{Price: math.Round(price*100) / 100, Qty: qty}
	}

	g.prevBids = bids
	g.prevAsks = asks
	return bids, asks
}

// Generate synthetic orders by diffing current vs previous snapshot
func (g *MarketGenerator) GenerateOrders(bids, asks []BookLevel) []Order {
	var orders []Order

	// Generate some random order flow each tick
	numOrders := 2 + g.rng.Intn(5) // 2-6 orders per tick
	for i := 0; i < numOrders; i++ {
		isBuy := g.rng.Intn(2) == 0
		var side int32 = 2 // SELL
		var book []BookLevel
		if isBuy {
			side = 1 // BUY
			book = bids
		} else {
			book = asks
		}
		if len(book) == 0 {
			continue
		}

		levelIdx := g.rng.Intn(len(book))
		actionRoll := g.rng.Float64()
		var action int32
		switch {
		case actionRoll < 0.5:
			action = 1 // ADD
		case actionRoll < 0.8:
			action = 3 // MODIFY
		default:
			action = 2 // REMOVE
		}

		orderType := int32(1) // LIMIT
		if g.rng.Float64() < 0.1 {
			orderType = 2 // MARKET
		}

		orders = append(orders, Order{
			PairID: 1,
			Price:  book[levelIdx].Price,
			Time:   g.tick,
			Qty:    int32(50 + g.rng.Intn(2000)),
			Side:   side,
			Type:   orderType,
			Action: action,
		})
	}

	return orders
}

type Order struct {
	PairID int64
	Price  float64
	Time   uint64
	Qty    int32
	Side   int32
	Type   int32
	Action int32
}

func serializeSnapshot(pairID int64, timestamp uint64, bids, asks []BookLevel) []byte {
	numBids := uint16(len(bids))
	numAsks := uint16(len(asks))
	totalSize := 20 + int(numBids+numAsks)*12
	buf := make([]byte, totalSize)

	binary.LittleEndian.PutUint64(buf[0:8], uint64(pairID))
	binary.LittleEndian.PutUint64(buf[8:16], timestamp)
	binary.LittleEndian.PutUint16(buf[16:18], numBids)
	binary.LittleEndian.PutUint16(buf[18:20], numAsks)

	offset := 20
	for _, b := range bids {
		binary.LittleEndian.PutUint64(buf[offset:offset+8], math.Float64bits(b.Price))
		binary.LittleEndian.PutUint32(buf[offset+8:offset+12], uint32(b.Qty))
		offset += 12
	}
	for _, a := range asks {
		binary.LittleEndian.PutUint64(buf[offset:offset+8], math.Float64bits(a.Price))
		binary.LittleEndian.PutUint32(buf[offset+8:offset+12], uint32(a.Qty))
		offset += 12
	}

	return buf
}

func serializeOrders(orders []Order) []byte {
	count := uint32(len(orders))
	// Header: type(1) + pairId(4) + seq(8) + count(4) + pad(3) = 20
	// Order struct: pairId(8) + price(8) + time(8) + qty(4) + side(4) + type(4) + action(4) = 40
	buf := make([]byte, 20+int(count)*40)
	buf[0] = 1
	if count > 0 {
		binary.LittleEndian.PutUint32(buf[1:5], uint32(orders[0].PairID))
	}
	seq := atomic.AddUint64(&orderSeq, 1)
	binary.LittleEndian.PutUint64(buf[5:13], seq)
	binary.LittleEndian.PutUint32(buf[13:17], count)

	offset := 20
	for _, o := range orders {
		binary.LittleEndian.PutUint64(buf[offset:offset+8], uint64(o.PairID))
		binary.LittleEndian.PutUint64(buf[offset+8:offset+16], math.Float64bits(o.Price))
		binary.LittleEndian.PutUint64(buf[offset+16:offset+24], o.Time)
		binary.LittleEndian.PutUint32(buf[offset+24:offset+28], uint32(o.Qty))
		binary.LittleEndian.PutUint32(buf[offset+28:offset+32], uint32(o.Side))
		binary.LittleEndian.PutUint32(buf[offset+32:offset+36], uint32(o.Type))
		binary.LittleEndian.PutUint32(buf[offset+36:offset+40], uint32(o.Action))
		offset += 40
	}

	return buf
}

func envBool(key string, def bool) bool {
	val := strings.ToLower(strings.TrimSpace(os.Getenv(key)))
	if val == "" {
		return def
	}
	switch val {
	case "1", "true", "t", "yes", "y", "on":
		return true
	case "0", "false", "f", "no", "n", "off":
		return false
	default:
		return def
	}
}

func main() {
	natsURL := os.Getenv("NATS_URL")
	if natsURL == "" {
		natsURL = nats.DefaultURL
	}

	nc, err := nats.Connect(natsURL)
	if err != nil {
		log.Fatalf("NATS connect: %v", err)
	}
	defer nc.Close()
	log.Printf("Connected to NATS at %s", natsURL)

	gen := NewMarketGenerator(
		100.0,    // basePrice
		0.15,     // amplitude — keep drift within ~1 book width over 500 cols
		0.0003,   // frequency
		0.02,     // noise
		20,       // bookDepth
		5.0,      // spreadBps
	)

	rate := 10
	ticker := time.NewTicker(time.Second / time.Duration(rate))
	defer ticker.Stop()

	publishOrders := envBool("PUBLISH_ORDERS", false)
	if publishOrders {
		log.Printf("Publishing snapshots at %d/sec to orderbook.snapshots + orderbook.tbt", rate)
	} else {
		log.Printf("Publishing snapshots at %d/sec to orderbook.snapshots", rate)
	}

	for range ticker.C {
		bids, asks := gen.GenerateSnapshot()

		snapData := serializeSnapshot(1, gen.tick, bids, asks)
		if err := nc.Publish("orderbook.snapshots", snapData); err != nil {
			log.Printf("publish snapshot error: %v", err)
			continue
		}

		if publishOrders {
			orders := gen.GenerateOrders(bids, asks)
			if len(orders) > 0 {
				orderData := serializeOrders(orders)
				if err := nc.Publish("orderbook.tbt", orderData); err != nil {
					log.Printf("publish orders error: %v", err)
				}
			}
		}

		nc.Flush()
	}
}
