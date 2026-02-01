import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import 'rc-dock/dist/rc-dock-dark.css'
import './dockTheme.css'
import App from './App'

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <App />
  </StrictMode>,
)
