#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLUSTER_NAME="buni"

case "${1:-}" in
  create)
    echo "Creating Kind cluster '$CLUSTER_NAME'..."
    kind create cluster --name "$CLUSTER_NAME" --config "$SCRIPT_DIR/kind-config.yaml" --wait 60s
    echo "Cluster '$CLUSTER_NAME' is ready."
    ;;
  delete)
    echo "Deleting Kind cluster '$CLUSTER_NAME'..."
    kind delete cluster --name "$CLUSTER_NAME"
    echo "Cluster '$CLUSTER_NAME' deleted."
    ;;
  *)
    echo "Usage: $0 {create|delete}"
    exit 1
    ;;
esac
