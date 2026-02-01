import { useState } from 'react';
import type { Viewport } from '../lib/heatmapRenderer';
import { getSharedViewport } from '../lib/heatmapRenderer';
import { useMarketData } from '../context/MarketDataContext';
import Heatmap from './Heatmap';

export default function HeatmapPanel() {
  const { lastSnapshot, connected } = useMarketData();
  const [autoFollow, setAutoFollow] = useState(true);

  const handleViewportChange = (vp: Viewport) => {
    setAutoFollow(vp.autoFollow);
  };

  const handleReset = () => {
    const vp = getSharedViewport();
    vp.autoFollow = true;
    vp.timeOffset = 0;
    setAutoFollow(true);
  };

  return (
    <div style={{ width: '100%', height: '100%', position: 'relative', overflow: 'hidden' }}>
      <Heatmap snapshot={lastSnapshot} onViewportChange={handleViewportChange} />
      <div style={{
        position: 'absolute',
        top: 6,
        left: 8,
        color: connected ? '#0a0' : '#a00',
        fontFamily: 'monospace',
        fontSize: '11px',
        pointerEvents: 'none',
      }}>
        {connected ? 'LIVE' : 'DISCONNECTED'}
      </div>
      {!autoFollow && (
        <button
          onClick={handleReset}
          style={{
            position: 'absolute',
            top: 6,
            right: 8,
            background: '#1a1a24',
            color: '#888',
            border: '1px solid #333',
            borderRadius: '4px',
            padding: '2px 8px',
            fontFamily: 'monospace',
            fontSize: '10px',
            cursor: 'pointer',
          }}
        >
          Reset
        </button>
      )}
    </div>
  );
}
