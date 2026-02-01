ROOT_DIR := $(shell cd .. && pwd)
INFRA_DIR := $(shell pwd)
KIND_DIR := $(INFRA_DIR)/kind
CHARTS_DIR := $(INFRA_DIR)/charts
CLUSTER_NAME := buni

.PHONY: cluster-up cluster-down infra build-images load-images \
        deploy-processor deploy-feeder deploy-viz deploy-all all

cluster-up:
	bash $(KIND_DIR)/create-cluster.sh create

cluster-down:
	bash $(KIND_DIR)/create-cluster.sh delete

infra:
	cd $(INFRA_DIR)/ansible && ansible-playbook playbook.yaml

build-images:
	docker build -t buni-processor:latest $(ROOT_DIR)
	docker build -t buni-feeder:latest $(ROOT_DIR)/viz/feeder
	docker build -t buni-viz:latest -f $(ROOT_DIR)/viz/server/Dockerfile $(ROOT_DIR)/viz

load-images:
	kind load docker-image buni-processor:latest --name $(CLUSTER_NAME)
	kind load docker-image buni-feeder:latest --name $(CLUSTER_NAME)
	kind load docker-image buni-viz:latest --name $(CLUSTER_NAME)

deploy-processor:
	helm upgrade --install processor $(CHARTS_DIR)/processor

deploy-feeder:
	helm upgrade --install feeder $(CHARTS_DIR)/feeder

deploy-viz:
	helm upgrade --install viz $(CHARTS_DIR)/viz

deploy-all: deploy-processor deploy-feeder deploy-viz

all: cluster-up infra build-images load-images deploy-all
