import { useRef } from 'react';
import DockLayout, { type LayoutData } from 'rc-dock';
import { MarketDataProvider } from './context/MarketDataContext';
import HeatmapPanel from './components/HeatmapPanel';
import DepthChartPanel from './components/DepthChartPanel';
import OrderFlowPanel from './components/OrderFlowPanel';

const wsUrl = `ws://${window.location.host}/ws`;

function DockApp() {
  const dockRef = useRef<DockLayout>(null);

  const defaultLayout: LayoutData = {
    dockbox: {
      mode: 'horizontal',
      children: [
        {
          size: 200,
          minWidth: 150,
          tabs: [
            {
              id: 'orderflow',
              title: 'Order Flow',
              content: <OrderFlowPanel />,
              closable: false,
            },
          ],
        },
        {
          size: 960,
          tabs: [
            {
              id: 'heatmap',
              title: 'Heatmap',
              content: <HeatmapPanel />,
              closable: false,
            },
          ],
        },
        {
          size: 280,
          minWidth: 200,
          tabs: [
            {
              id: 'depth',
              title: 'Depth',
              content: <DepthChartPanel />,
              closable: false,
            },
          ],
        },
      ],
    },
  };

  const loadTab = (tab: { id?: string }) => {
    switch (tab.id) {
      case 'orderflow':
        return {
          id: 'orderflow',
          title: 'Order Flow',
          content: <OrderFlowPanel />,
          closable: false,
        };
      case 'heatmap':
        return {
          id: 'heatmap',
          title: 'Heatmap',
          content: <HeatmapPanel />,
          closable: false,
        };
      case 'depth':
        return {
          id: 'depth',
          title: 'Depth',
          content: <DepthChartPanel />,
          closable: false,
        };
      default:
        return { id: tab.id ?? '', title: '', content: <div /> };
    }
  };

  return (
    <DockLayout
      ref={dockRef}
      defaultLayout={defaultLayout}
      loadTab={loadTab}
      style={{ width: '100vw', height: '100vh', background: '#0d0d14' }}
    />
  );
}

export default function App() {
  return (
    <MarketDataProvider url={wsUrl}>
      <DockApp />
    </MarketDataProvider>
  );
}
