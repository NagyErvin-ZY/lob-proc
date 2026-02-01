import { useMarketData } from '../context/MarketDataContext';
import DepthChart from './DepthChart';

export default function DepthChartPanel() {
  const { lastSnapshot } = useMarketData();
  return <DepthChart snapshot={lastSnapshot} />;
}
