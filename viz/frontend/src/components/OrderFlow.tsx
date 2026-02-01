import type { OrderEntry } from '../types';

interface Props {
  orders: OrderEntry[];
}

const actionColors: Record<string, string> = {
  ADD_BUY: '#00cc88',
  ADD_SELL: '#ee4444',
  SEEKER_ADD_BUY: '#00cc88',
  SEEKER_ADD_SELL: '#ee4444',
  REMOVE_BUY: '#666',
  REMOVE_SELL: '#666',
  MODIFY_BUY: '#ccaa00',
  MODIFY_SELL: '#ccaa00',
};

export default function OrderFlow({ orders }: Props) {
  const visible = orders.slice(-50).reverse();

  return (
    <div style={{
      width: '100%',
      height: '100%',
      overflowY: 'auto',
      fontFamily: 'monospace',
      fontSize: '11px',
      lineHeight: '16px',
      padding: '4px 8px',
      background: '#0d0d14',
    }}>
      {visible.length === 0 && (
        <span style={{ color: '#555' }}>Waiting for orders...</span>
      )}
      {visible.map((o, i) => {
        const key = `${o.action}_${o.side}`;
        const color = actionColors[key] || '#888';
        return (
          <div key={i} style={{ color, whiteSpace: 'nowrap' }}>
            {o.action.padEnd(7)} {o.side.padEnd(4)} {o.price.toFixed(2).padStart(10)} x{String(o.qty).padStart(6)} {o.orderType}
          </div>
        );
      })}
    </div>
  );
}
