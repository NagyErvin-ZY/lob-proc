export interface BookLevel {
  price: number;
  qty: number;
}

export interface SnapshotMessage {
  type: 'snapshot';
  timestamp: number;
  bids: BookLevel[];
  asks: BookLevel[];
}

export interface OrderMessage {
  type: 'orders';
  orders: OrderEntry[];
}

export interface OrderEntry {
  price: number;
  qty: number;
  side: 'BUY' | 'SELL';
  action: 'SEEKER_ADD' | 'ADD' | 'REMOVE' | 'MODIFY';
  orderType: 'LIMIT' | 'MARKET' | 'ICEBERG' | 'STOP';
}

export type WSMessage = SnapshotMessage | OrderMessage;

export interface HeatmapColumn {
  timestamp: number;
  levels: Map<number, { qty: number; side: 'bid' | 'ask' }>;
}
