// Bid colors: dark blue → bright cyan
// Ask colors: dark red → bright orange/yellow

function clamp(v: number, min: number, max: number): number {
  return v < min ? min : v > max ? max : v;
}

export function bidColor(intensity: number): [number, number, number] {
  const t = clamp(intensity, 0, 1);
  const r = Math.floor(0 + t * 20);
  const g = Math.floor(30 + t * 225);
  const b = Math.floor(80 + t * 175);
  return [r, g, b];
}

export function askColor(intensity: number): [number, number, number] {
  const t = clamp(intensity, 0, 1);
  const r = Math.floor(80 + t * 175);
  const g = Math.floor(20 + t * 180);
  const b = Math.floor(0 + t * 20);
  return [r, g, b];
}

export function bidColorCSS(intensity: number): string {
  const [r, g, b] = bidColor(intensity);
  return `rgb(${r},${g},${b})`;
}

export function askColorCSS(intensity: number): string {
  const [r, g, b] = askColor(intensity);
  return `rgb(${r},${g},${b})`;
}
