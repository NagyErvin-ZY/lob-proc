import { useRef, useEffect } from 'react';
import type { SnapshotMessage } from '../types';
import { bidColorCSS, askColorCSS } from '../lib/colorScale';

interface Props {
  snapshot: SnapshotMessage | null;
}

export default function DepthChart({ snapshot }: Props) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || !snapshot) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const rect = canvas.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    const w = Math.floor(rect.width * dpr);
    const h = Math.floor(rect.height * dpr);
    if (canvas.width !== w || canvas.height !== h) {
      canvas.width = w;
      canvas.height = h;
    }
    ctx.scale(dpr, dpr);
    const cw = rect.width;
    const ch = rect.height;

    ctx.fillStyle = '#0d0d14';
    ctx.fillRect(0, 0, cw, ch);

    const bids = [...snapshot.bids].sort((a, b) => b.price - a.price);
    const asks = [...snapshot.asks].sort((a, b) => a.price - b.price);

    if (bids.length === 0 && asks.length === 0) return;

    const allPrices = [...bids, ...asks];
    const maxQty = Math.max(...allPrices.map(l => l.qty), 1);
    const minPrice = Math.min(...allPrices.map(l => l.price));
    const maxPrice = Math.max(...allPrices.map(l => l.price));
    const priceRange = maxPrice - minPrice || 1;

    const margin = { top: 20, bottom: 20, left: 4, right: 4 };
    const plotW = cw - margin.left - margin.right;
    const plotH = ch - margin.top - margin.bottom;
    const barMaxW = plotW / 2 - 30; // leave room for price labels

    const priceToY = (p: number) =>
      margin.top + (1 - (p - minPrice) / priceRange) * plotH;

    const midX = cw / 2;

    // Bid bars — extend left from center
    ctx.font = '10px monospace';
    ctx.textBaseline = 'middle';
    for (let i = 0; i < bids.length; i++) {
      const y = priceToY(bids[i].price);
      const barW = (bids[i].qty / maxQty) * barMaxW;
      const barH = Math.max(2, (plotH / (bids.length + asks.length)) - 1);
      const intensity = bids[i].qty / maxQty;
      ctx.fillStyle = bidColorCSS(intensity);
      ctx.fillRect(midX - barW, y - barH / 2, barW, barH);

      if (barH >= 8) {
        ctx.fillStyle = '#aaa';
        ctx.textAlign = 'left';
        ctx.fillText(bids[i].price.toFixed(2), midX + 2, y);
      }
    }

    // Ask bars — extend right from center
    for (let i = 0; i < asks.length; i++) {
      const y = priceToY(asks[i].price);
      const barW = (asks[i].qty / maxQty) * barMaxW;
      const barH = Math.max(2, (plotH / (bids.length + asks.length)) - 1);
      const intensity = asks[i].qty / maxQty;
      ctx.fillStyle = askColorCSS(intensity);
      ctx.fillRect(midX, y - barH / 2, barW, barH);

      if (barH >= 8) {
        ctx.fillStyle = '#aaa';
        ctx.textAlign = 'right';
        ctx.fillText(asks[i].price.toFixed(2), midX - 2, y);
      }
    }

    // Spread label
    if (bids.length > 0 && asks.length > 0) {
      const spread = asks[0].price - bids[0].price;
      const spreadY = (priceToY(bids[0].price) + priceToY(asks[0].price)) / 2;
      ctx.fillStyle = '#555';
      ctx.font = '10px monospace';
      ctx.textAlign = 'center';
      ctx.fillText(`${spread.toFixed(3)}`, midX, spreadY);
    }

    // Reset transform
    ctx.setTransform(1, 0, 0, 1, 0, 0);
  }, [snapshot]);

  return (
    <canvas
      ref={canvasRef}
      style={{ width: '100%', height: '100%', display: 'block' }}
    />
  );
}
