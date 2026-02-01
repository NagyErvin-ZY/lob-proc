import { useEffect, useRef, useCallback, useState } from 'react';
import type { WSMessage, SnapshotMessage, OrderEntry } from '../types';

interface UseWebSocketReturn {
  connected: boolean;
  lastSnapshot: SnapshotMessage | null;
  orderBuffer: OrderEntry[];
}

export function useWebSocket(url: string): UseWebSocketReturn {
  const [connected, setConnected] = useState(false);
  const [lastSnapshot, setLastSnapshot] = useState<SnapshotMessage | null>(null);
  const [orderBuffer, setOrderBuffer] = useState<OrderEntry[]>([]);
  const wsRef = useRef<WebSocket | null>(null);
  const reconnectTimer = useRef<number>(0);

  const connect = useCallback(() => {
    if (wsRef.current?.readyState === WebSocket.OPEN) return;

    const ws = new WebSocket(url);
    wsRef.current = ws;

    ws.onopen = () => setConnected(true);
    ws.onclose = () => {
      setConnected(false);
      reconnectTimer.current = window.setTimeout(connect, 1000);
    };
    ws.onerror = () => ws.close();

    ws.onmessage = (ev) => {
      try {
        const msg: WSMessage = JSON.parse(ev.data);
        if (msg.type === 'snapshot') {
          setLastSnapshot(msg);
        } else if (msg.type === 'orders') {
          setOrderBuffer((prev) => {
            const next = [...prev, ...msg.orders];
            return next.length > 200 ? next.slice(-200) : next;
          });
        }
      } catch {}
    };
  }, [url]);

  useEffect(() => {
    connect();
    return () => {
      clearTimeout(reconnectTimer.current);
      wsRef.current?.close();
    };
  }, [connect]);

  return { connected, lastSnapshot, orderBuffer };
}
