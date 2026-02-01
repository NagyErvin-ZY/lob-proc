import { useMarketData } from '../context/MarketDataContext';
import OrderFlow from './OrderFlow';

export default function OrderFlowPanel() {
  const { orderBuffer } = useMarketData();
  return <OrderFlow orders={orderBuffer} />;
}
