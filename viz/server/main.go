package main

import (
	"encoding/binary"
	"encoding/json"
	"fmt"
	"log"
	"math"
	"net/http"
	"os"
	"sync"

	"github.com/gorilla/websocket"
	"github.com/nats-io/nats.go"
)

// Wire format structs matching C++ packed structs

type WireSnapshot struct {
	PairID    int64
	Timestamp uint64
	NumBids   uint16
	NumAsks   uint16
}

const wireSnapshotSize = 8 + 8 + 2 + 2 // 20 bytes

type WireBookLevel struct {
	Price float64
	Qty   int32
}

const wireBookLevelSize = 8 + 4 // 12 bytes

const wireOrdersHeaderSize = 20 // type(1) + pairId(4) + seq(8) + count(4) + pad(3)
// Order struct: 40 bytes (8+8+8+4+4+4+4)
const wireOrderSize = 40

// JSON output types

type BookLevel struct {
	Price float64 `json:"price"`
	Qty   int32   `json:"qty"`
}

type SnapshotMsg struct {
	Type      string      `json:"type"`
	Timestamp uint64      `json:"timestamp"`
	Bids      []BookLevel `json:"bids"`
	Asks      []BookLevel `json:"asks"`
}

type OrderEntry struct {
	Price     float64 `json:"price"`
	Qty       int32   `json:"qty"`
	Side      string  `json:"side"`
	Action    string  `json:"action"`
	OrderType string  `json:"orderType"`
}

type OrdersMsg struct {
	Type   string       `json:"type"`
	Orders []OrderEntry `json:"orders"`
}

var sideNames = map[int32]string{1: "BUY", 2: "SELL"}
var actionNames = map[int32]string{0: "SEEKER_ADD", 1: "ADD", 2: "REMOVE", 3: "MODIFY"}
var typeNames = map[int32]string{1: "LIMIT", 2: "MARKET", 3: "ICEBERG", 4: "STOP"}

func decodeSnapshot(data []byte) (*SnapshotMsg, error) {
	if len(data) < wireSnapshotSize {
		return nil, fmt.Errorf("snapshot too short: %d", len(data))
	}

	pairID := int64(binary.LittleEndian.Uint64(data[0:8]))
	_ = pairID
	timestamp := binary.LittleEndian.Uint64(data[8:16])
	numBids := binary.LittleEndian.Uint16(data[16:18])
	numAsks := binary.LittleEndian.Uint16(data[18:20])

	expected := wireSnapshotSize + int(numBids+numAsks)*wireBookLevelSize
	if len(data) < expected {
		return nil, fmt.Errorf("snapshot data too short: got %d, need %d", len(data), expected)
	}

	msg := &SnapshotMsg{
		Type:      "snapshot",
		Timestamp: timestamp,
		Bids:      make([]BookLevel, numBids),
		Asks:      make([]BookLevel, numAsks),
	}

	offset := wireSnapshotSize
	for i := 0; i < int(numBids); i++ {
		price := math.Float64frombits(binary.LittleEndian.Uint64(data[offset : offset+8]))
		qty := int32(binary.LittleEndian.Uint32(data[offset+8 : offset+12]))
		msg.Bids[i] = BookLevel{Price: price, Qty: qty}
		offset += wireBookLevelSize
	}
	for i := 0; i < int(numAsks); i++ {
		price := math.Float64frombits(binary.LittleEndian.Uint64(data[offset : offset+8]))
		qty := int32(binary.LittleEndian.Uint32(data[offset+8 : offset+12]))
		msg.Asks[i] = BookLevel{Price: price, Qty: qty}
		offset += wireBookLevelSize
	}

	return msg, nil
}

func decodeOrders(data []byte) (*OrdersMsg, error) {
	if len(data) < wireOrdersHeaderSize {
		return nil, fmt.Errorf("orders too short: %d", len(data))
	}

	msgType := data[0]
	_ = binary.LittleEndian.Uint32(data[1:5])
	_ = binary.LittleEndian.Uint64(data[5:13])
	count := binary.LittleEndian.Uint32(data[13:17])
	if msgType != 1 {
		return nil, fmt.Errorf("unexpected orders msgType: %d", msgType)
	}

	expected := wireOrdersHeaderSize + int(count)*wireOrderSize
	if len(data) < expected {
		return nil, fmt.Errorf("orders data too short: got %d, need %d", len(data), expected)
	}

	msg := &OrdersMsg{
		Type:   "orders",
		Orders: make([]OrderEntry, count),
	}

	offset := wireOrdersHeaderSize
	for i := 0; i < int(count); i++ {
		// pairId (8) + price (8) + time (8) + qty (4) + side (4) + type (4) + action (4)
		_ = binary.LittleEndian.Uint64(data[offset : offset+8]) // pairId
		price := math.Float64frombits(binary.LittleEndian.Uint64(data[offset+8 : offset+16]))
		_ = binary.LittleEndian.Uint64(data[offset+16 : offset+24]) // time
		qty := int32(binary.LittleEndian.Uint32(data[offset+24 : offset+28]))
		side := int32(binary.LittleEndian.Uint32(data[offset+28 : offset+32]))
		orderType := int32(binary.LittleEndian.Uint32(data[offset+32 : offset+36]))
		action := int32(binary.LittleEndian.Uint32(data[offset+36 : offset+40]))

		msg.Orders[i] = OrderEntry{
			Price:     price,
			Qty:       qty,
			Side:      sideNames[side],
			Action:    actionNames[action],
			OrderType: typeNames[orderType],
		}
		offset += wireOrderSize
	}

	return msg, nil
}

// WebSocket hub

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool { return true },
}

type client struct {
	conn *websocket.Conn
	mu   sync.Mutex
}

type Hub struct {
	mu      sync.RWMutex
	clients map[*client]struct{}
}

func (h *Hub) Add(conn *websocket.Conn) *client {
	c := &client{conn: conn}
	h.mu.Lock()
	h.clients[c] = struct{}{}
	h.mu.Unlock()
	return c
}

func (h *Hub) Remove(c *client) {
	h.mu.Lock()
	delete(h.clients, c)
	h.mu.Unlock()
}

func (h *Hub) Broadcast(data []byte) {
	h.mu.RLock()
	defer h.mu.RUnlock()
	for c := range h.clients {
		c.mu.Lock()
		err := c.conn.WriteMessage(websocket.TextMessage, data)
		c.mu.Unlock()
		if err != nil {
			c.conn.Close()
			go h.Remove(c)
		}
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

	hub := &Hub{clients: make(map[*client]struct{})}

	// Subscribe to snapshots
	_, err = nc.Subscribe("orderbook.snapshots", func(msg *nats.Msg) {
		snap, err := decodeSnapshot(msg.Data)
		if err != nil {
			log.Printf("decode snapshot: %v", err)
			return
		}
		data, _ := json.Marshal(snap)
		hub.Broadcast(data)
	})
	if err != nil {
		log.Fatalf("subscribe snapshots: %v", err)
	}

	// Subscribe to trade-by-trade orders
	_, err = nc.Subscribe("orderbook.tbt", func(msg *nats.Msg) {
		orders, err := decodeOrders(msg.Data)
		if err != nil {
			log.Printf("decode orders: %v", err)
			return
		}
		data, _ := json.Marshal(orders)
		hub.Broadcast(data)
	})
	if err != nil {
		log.Fatalf("subscribe tbt: %v", err)
	}

	// WebSocket endpoint
	http.HandleFunc("/ws", func(w http.ResponseWriter, r *http.Request) {
		conn, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			log.Printf("ws upgrade: %v", err)
			return
		}
		c := hub.Add(conn)
		log.Printf("client connected (%d total)", len(hub.clients))

		// Read loop to detect disconnects
		for {
			if _, _, err := conn.ReadMessage(); err != nil {
				hub.Remove(c)
				conn.Close()
				break
			}
		}
	})

	// Serve frontend static files
	distDir := os.Getenv("DIST_DIR")
	if distDir == "" {
		distDir = "../frontend/dist"
	}
	http.Handle("/", http.FileServer(http.Dir(distDir)))

	port := os.Getenv("PORT")
	if port == "" {
		port = "8080"
	}

	log.Printf("Listening on :%s", port)
	log.Fatal(http.ListenAndServe(":"+port, nil))
}
