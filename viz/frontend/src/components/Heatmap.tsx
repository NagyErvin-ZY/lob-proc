import { useRef, useEffect, useCallback } from 'react';
import type { SnapshotMessage, HeatmapColumn } from '../types';
import {
  pushColumn,
  renderHeatmap,
  getSharedState,
  getSharedViewport,
  type Viewport,
} from '../lib/heatmapRenderer';

interface Props {
  snapshot: SnapshotMessage | null;
  onViewportChange?: (viewport: Viewport) => void;
}

export default function Heatmap({ snapshot, onViewportChange }: Props) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const stateRef = useRef(getSharedState());
  const viewportRef = useRef(getSharedViewport());
  const rafRef = useRef<number>(0);
  const dirtyRef = useRef(false);
  const dragRef = useRef<{ startX: number; startY: number; startTimeOffset: number; startPriceMin: number; startPriceMax: number } | null>(null);

  const notifyViewport = useCallback(() => {
    onViewportChange?.(viewportRef.current);
  }, [onViewportChange]);

  useEffect(() => {
    if (!snapshot) return;

    const levels = new Map<number, { qty: number; side: 'bid' | 'ask' }>();

    for (const b of snapshot.bids) {
      const key = Math.round(b.price * 100) / 100;
      levels.set(key, { qty: b.qty, side: 'bid' });
    }
    for (const a of snapshot.asks) {
      const key = Math.round(a.price * 100) / 100;
      levels.set(key, { qty: a.qty, side: 'ask' });
    }

    const col: HeatmapColumn = { timestamp: snapshot.timestamp, levels };
    pushColumn(stateRef.current, col);
    dirtyRef.current = true;
  }, [snapshot]);

  // Render loop
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d', { alpha: false });
    if (!ctx) return;

    const render = () => {
      if (dirtyRef.current) {
        const rect = canvas.getBoundingClientRect();
        const dpr = window.devicePixelRatio || 1;
        const w = Math.floor(rect.width * dpr);
        const h = Math.floor(rect.height * dpr);
        if (canvas.width !== w || canvas.height !== h) {
          canvas.width = w;
          canvas.height = h;
        }
        renderHeatmap(ctx, stateRef.current, w, h, viewportRef.current);
        dirtyRef.current = false;
      }
      rafRef.current = requestAnimationFrame(render);
    };

    rafRef.current = requestAnimationFrame(render);
    return () => cancelAnimationFrame(rafRef.current);
  }, []);

  // Wheel zoom (price axis) — attached via addEventListener for passive:false
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const onWheel = (e: WheelEvent) => {
      e.preventDefault();
      const vp = viewportRef.current;
      const state = stateRef.current;
      const rect = canvas.getBoundingClientRect();
      const mouseY = (e.clientY - rect.top) / rect.height; // 0=top, 1=bottom

      // Get current effective bounds
      const curMin = vp.autoFollow ? state.dataPriceMin : vp.priceMin;
      const curMax = vp.autoFollow ? state.dataPriceMax : vp.priceMax;
      const range = curMax - curMin;

      // Price at mouse position (top=max, bottom=min)
      const priceAtMouse = curMax - mouseY * range;

      const factor = e.deltaY > 0 ? 1.1 : 0.9;
      const newRange = range * factor;

      // Keep price at mouse position fixed
      const newMax = priceAtMouse + mouseY * newRange;
      const newMin = priceAtMouse - (1 - mouseY) * newRange;

      vp.priceMin = newMin;
      vp.priceMax = newMax;
      vp.autoFollow = false;
      dirtyRef.current = true;
      notifyViewport();
    };

    canvas.addEventListener('wheel', onWheel, { passive: false });
    return () => canvas.removeEventListener('wheel', onWheel);
  }, [notifyViewport]);

  // Drag pan
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const onMouseDown = (e: MouseEvent) => {
      const vp = viewportRef.current;
      const state = stateRef.current;
      const curMin = vp.autoFollow ? state.dataPriceMin : vp.priceMin;
      const curMax = vp.autoFollow ? state.dataPriceMax : vp.priceMax;

      dragRef.current = {
        startX: e.clientX,
        startY: e.clientY,
        startTimeOffset: vp.timeOffset,
        startPriceMin: curMin,
        startPriceMax: curMax,
      };
    };

    const onMouseMove = (e: MouseEvent) => {
      if (!dragRef.current) return;
      const vp = viewportRef.current;
      const rect = canvas.getBoundingClientRect();
      const drag = dragRef.current;

      const dx = e.clientX - drag.startX;
      const dy = e.clientY - drag.startY;

      // X drag: shift time offset (pixels -> columns)
      const colWidth = Math.max(1, Math.floor(rect.width / stateRef.current.maxColumns));
      vp.timeOffset = drag.startTimeOffset + dx / colWidth;

      // Y drag: shift price range
      const priceRange = drag.startPriceMax - drag.startPriceMin;
      const priceDelta = (dy / rect.height) * priceRange;
      vp.priceMin = drag.startPriceMin + priceDelta;
      vp.priceMax = drag.startPriceMax + priceDelta;
      vp.autoFollow = false;

      dirtyRef.current = true;
      notifyViewport();
    };

    const onMouseUp = () => {
      dragRef.current = null;
    };

    canvas.addEventListener('mousedown', onMouseDown);
    window.addEventListener('mousemove', onMouseMove);
    window.addEventListener('mouseup', onMouseUp);
    return () => {
      canvas.removeEventListener('mousedown', onMouseDown);
      window.removeEventListener('mousemove', onMouseMove);
      window.removeEventListener('mouseup', onMouseUp);
    };
  }, [notifyViewport]);

  // Double-click to reset
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const onDblClick = () => {
      const vp = viewportRef.current;
      vp.autoFollow = true;
      vp.timeOffset = 0;
      dirtyRef.current = true;
      notifyViewport();
    };

    canvas.addEventListener('dblclick', onDblClick);
    return () => canvas.removeEventListener('dblclick', onDblClick);
  }, [notifyViewport]);

  return (
    <canvas
      ref={canvasRef}
      style={{ width: '100%', height: '100%', display: 'block', cursor: 'grab' }}
    />
  );
}
