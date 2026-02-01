# lob-proc

Limit order book processor. Parses exchange snapshots into tick-by-tick order events, streams them over NATS, and renders a live heatmap in the browser.

This started as a personal project I had sitting around. I decided to publish it rather than let it collect dust — it's lightly polished but functional, and maybe useful to someone working on similar problems.

## What's in here

- **C++ core** — order book snapshot parser that diffs consecutive snapshots and emits individual order events (add/modify/cancel). Designed to be fast.
- **feeder** (Go) — generates synthetic snapshots (swap in an exchange websocket if needed), publishes to NATS.
- **processor** (C++) — subscribes to snapshots on NATS, runs them through the parser, publishes tick-by-tick events.
- **viz** (Go + TypeScript) — server subscribes to NATS events, pushes to browser over websocket. Frontend renders a live order book heatmap.
- **infra** — Kind cluster setup, Helm charts, single-script deployment. See `infra/README.md`.

## Quick start

Build and run locally with NATS:

```
# start nats
docker run -p 4222:4222 nats:latest

# build C++ (needs cmake, g++)
cmake -B build && cmake --build build

# run processor
NATS_URL=nats://localhost:4222 ./build/nats_processor

# run feeder (synthetic snapshots; set PUBLISH_ORDERS=true to emit synthetic orders too)
cd viz/feeder && go run .

# run viz
cd viz/server && go run .
# open http://localhost:8080
```

Or deploy everything to a local Kind cluster:

```
cd infra
./deploy.sh
# open http://localhost:30080
```

---

<sup>**On the seeker mechanism:** The parser includes a "seeker" that tags whether a price level is beyond previously seen frontiers. The current implementation is a naive bounds-based approach — it remembers the highest bid and lowest ask seen so far and tags only levels beyond those frontiers as `SEEKER_ADD`. Everything inside the band is treated as a revisit. This is the dumbest possible solution: it discards granular history entirely. A better approach is probabilistic — for example, comparing the current quantity at a revisited price level against historical quantities and only emitting orders for the differential. If the quantities haven't diverged much, treat it as a revisit; if they have, treat the difference as new orders to feed into downstream pipelines. This is just one of many possible heuristics. The seeker core is intended to be swappable so different strategies can be iterated on without touching the rest of the parser.</sup>

## License

MIT — see [LICENSE](LICENSE). Attribution required.
