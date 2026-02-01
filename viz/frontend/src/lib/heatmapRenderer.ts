import { bidColor, askColor } from './colorScale';
import type { HeatmapColumn } from '../types';

export interface Viewport {
  priceMin: number;
  priceMax: number;
  timeOffset: number;
  autoFollow: boolean;
}

export interface HeatmapState {
  columns: HeatmapColumn[];
  maxColumns: number;
  dataPriceMin: number;
  dataPriceMax: number;
  maxQty: number;
}

export function createHeatmapState(maxColumns = 500): HeatmapState {
  return {
    columns: [],
    maxColumns,
    dataPriceMin: Infinity,
    dataPriceMax: -Infinity,
    maxQty: 1,
  };
}

export function createViewport(): Viewport {
  return {
    priceMin: 0,
    priceMax: 0,
    timeOffset: 0,
    autoFollow: true,
  };
}

// Module-scoped singletons so rc-dock remounts don't lose accumulated data
let sharedState: HeatmapState | null = null;
let sharedViewport: Viewport | null = null;

export function getSharedState(): HeatmapState {
  if (!sharedState) sharedState = createHeatmapState(500);
  return sharedState;
}

export function getSharedViewport(): Viewport {
  if (!sharedViewport) sharedViewport = createViewport();
  return sharedViewport;
}

export function pushColumn(state: HeatmapState, col: HeatmapColumn): void {
  state.columns.push(col);
  if (state.columns.length > state.maxColumns) {
    state.columns.shift();
  }

  let pMin = Infinity;
  let pMax = -Infinity;
  let mQty = 1;

  // Scan last ~30 columns for price range (smooth tracking)
  const lookback = Math.min(30, state.columns.length);
  for (let i = state.columns.length - lookback; i < state.columns.length; i++) {
    const c = state.columns[i];
    for (const [price] of c.levels) {
      if (price < pMin) pMin = price;
      if (price > pMax) pMax = price;
    }
  }

  // maxQty from all columns (for consistent color scale)
  for (const c of state.columns) {
    for (const [, entry] of c.levels) {
      if (entry.qty > mQty) mQty = entry.qty;
    }
  }

  const padding = (pMax - pMin) * 0.1;
  state.dataPriceMin = pMin - padding;
  state.dataPriceMax = pMax + padding;
  state.maxQty = mQty;
}

export function renderHeatmap(
  ctx: CanvasRenderingContext2D,
  state: HeatmapState,
  width: number,
  height: number,
  viewport: Viewport,
): void {
  const { columns, dataPriceMin, dataPriceMax, maxQty } = state;

  // Determine effective price bounds
  const priceMin = viewport.autoFollow ? dataPriceMin : viewport.priceMin;
  const priceMax = viewport.autoFollow ? dataPriceMax : viewport.priceMax;

  if (columns.length === 0 || priceMax <= priceMin) {
    ctx.fillStyle = '#0a0a0f';
    ctx.fillRect(0, 0, width, height);
    ctx.fillStyle = '#555';
    ctx.font = '14px monospace';
    ctx.textAlign = 'center';
    ctx.fillText('Waiting for data...', width / 2, height / 2);
    return;
  }

  const imageData = ctx.createImageData(width, height);
  const data = imageData.data;

  // Background fill
  for (let i = 0; i < data.length; i += 4) {
    data[i] = 10;
    data[i + 1] = 10;
    data[i + 2] = 15;
    data[i + 3] = 255;
  }

  const colWidth = Math.max(1, Math.floor(width / state.maxColumns));
  const priceRange = priceMax - priceMin;

  const priceToY = (p: number) => Math.floor((1 - (p - priceMin) / priceRange) * height);

  // Apply time offset: shift columns left/right
  const timeShift = Math.round(viewport.timeOffset);

  for (let ci = 0; ci < columns.length; ci++) {
    const col = columns[ci];
    const x0 = width - (columns.length - ci - timeShift) * colWidth;
    if (x0 + colWidth <= 0 || x0 >= width) continue;

    // Sort levels by price descending (high to low)
    const sorted: { price: number; qty: number; side: 'bid' | 'ask' }[] = [];
    for (const [price, entry] of col.levels) {
      sorted.push({ price, qty: entry.qty, side: entry.side });
    }
    sorted.sort((a, b) => b.price - a.price);

    for (let li = 0; li < sorted.length; li++) {
      const level = sorted[li];
      const intensity = Math.sqrt(level.qty / maxQty);
      const [r, g, b] = level.side === 'bid'
        ? bidColor(intensity)
        : askColor(intensity);

      let yTop: number;
      let yBot: number;

      if (sorted.length === 1) {
        yTop = priceToY(level.price + 0.5);
        yBot = priceToY(level.price - 0.5);
      } else if (li === 0) {
        const gap = sorted[0].price - sorted[1].price;
        yTop = priceToY(level.price + gap * 0.5);
        yBot = priceToY((level.price + sorted[1].price) / 2);
      } else if (li === sorted.length - 1) {
        const gap = sorted[li - 1].price - sorted[li].price;
        yTop = priceToY((sorted[li - 1].price + level.price) / 2);
        yBot = priceToY(level.price - gap * 0.5);
      } else {
        yTop = priceToY((sorted[li - 1].price + level.price) / 2);
        yBot = priceToY((level.price + sorted[li + 1].price) / 2);
      }

      const yStart = Math.max(0, Math.min(yTop, yBot));
      const yEnd = Math.min(height - 1, Math.max(yTop, yBot));

      for (let y = yStart; y <= yEnd; y++) {
        for (let dx = 0; dx < colWidth; dx++) {
          const px = x0 + dx;
          if (px < 0 || px >= width) continue;
          const idx = (y * width + px) * 4;
          data[idx] = r;
          data[idx + 1] = g;
          data[idx + 2] = b;
          data[idx + 3] = 255;
        }
      }
    }
  }

  ctx.putImageData(imageData, 0, 0);

  // Price axis labels
  ctx.fillStyle = '#888';
  ctx.font = '11px monospace';
  ctx.textAlign = 'right';
  const numLabels = 10;
  for (let i = 0; i <= numLabels; i++) {
    const price = priceMin + (priceRange * i) / numLabels;
    const y = height - (i / numLabels) * height;
    ctx.fillText(price.toFixed(2), width - 4, y + 4);
  }
}
