import { createContext, useContext, type ReactNode } from 'react';
import type { SnapshotMessage, OrderEntry } from '../types';
import { useWebSocket } from '../hooks/useWebSocket';

interface MarketData {
  connected: boolean;
  lastSnapshot: SnapshotMessage | null;
  orderBuffer: OrderEntry[];
}

const MarketDataContext = createContext<MarketData>({
  connected: false,
  lastSnapshot: null,
  orderBuffer: [],
});

export function MarketDataProvider({ url, children }: { url: string; children: ReactNode }) {
  const data = useWebSocket(url);
  return (
    <MarketDataContext.Provider value={data}>
      {children}
    </MarketDataContext.Provider>
  );
}

export function useMarketData() {
  return useContext(MarketDataContext);
}
