#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
CLUSTER_NAME="buni"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

log()  { echo -e "${GREEN}[+]${NC} $*"; }
warn() { echo -e "${YELLOW}[!]${NC} $*"; }
fail() { echo -e "${RED}[x]${NC} $*"; exit 1; }

# --- preflight ---
check_dep() {
  command -v "$1" &>/dev/null || fail "'$1' is not installed"
}

log "Checking dependencies..."
for dep in docker kind kubectl helm ansible-playbook; do
  check_dep "$dep"
done

# --- cluster ---
if kind get clusters 2>/dev/null | grep -q "^${CLUSTER_NAME}$"; then
  warn "Kind cluster '$CLUSTER_NAME' already exists, skipping creation"
else
  log "Creating Kind cluster '$CLUSTER_NAME'..."
  kind create cluster --name "$CLUSTER_NAME" \
    --config "$SCRIPT_DIR/kind/kind-config.yaml" \
    --wait 60s
fi

kubectl config use-context "kind-${CLUSTER_NAME}" >/dev/null 2>&1
log "Cluster ready"

# --- nats ---
log "Deploying NATS..."
helm repo add nats https://nats-io.github.io/k8s/helm/charts/ --force-update >/dev/null 2>&1
helm upgrade --install nats nats/nats \
  --namespace nats --create-namespace \
  --set config.cluster.enabled=false \
  --set config.jetstream.enabled=false \
  --wait --timeout 120s

log "NATS deployed"

# --- images ---
log "Building Docker images..."
docker build -q -t buni-processor:latest "$ROOT_DIR"
docker build -q -t buni-feeder:latest "$ROOT_DIR/viz/feeder"
docker build -q -t buni-viz:latest -f "$ROOT_DIR/viz/server/Dockerfile" "$ROOT_DIR/viz"
log "Images built"

log "Loading images into Kind..."
kind load docker-image buni-processor:latest --name "$CLUSTER_NAME"
kind load docker-image buni-feeder:latest --name "$CLUSTER_NAME"
kind load docker-image buni-viz:latest --name "$CLUSTER_NAME"
log "Images loaded"

# --- deploy ---
log "Deploying services..."
helm upgrade --install processor "$SCRIPT_DIR/charts/processor" --wait --timeout 60s
helm upgrade --install feeder "$SCRIPT_DIR/charts/feeder" --wait --timeout 60s
helm upgrade --install viz "$SCRIPT_DIR/charts/viz" --wait --timeout 60s
log "Services deployed"

# --- verify ---
log "Verifying pods..."
kubectl wait --for=condition=Ready pods --all --timeout=60s 2>/dev/null || true
echo ""
kubectl get pods -A --field-selector=metadata.namespace!=kube-system,metadata.namespace!=local-path-storage
echo ""
log "Done. Viz UI: http://localhost:30080"
