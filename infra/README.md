# buni infra

Local Kubernetes deployment using Kind.

## Prerequisites

- docker
- kind
- kubectl
- helm
- ansible-playbook (with `kubernetes.core` collection)

Install the Ansible collection:

```
ansible-galaxy collection install kubernetes.core
```

## Deploy

```
./deploy.sh
```

Creates a Kind cluster, deploys NATS, builds and loads all images, and deploys processor, feeder, and viz.

Viz UI available at http://localhost:30080 after deploy completes.

## Teardown

```
./teardown.sh
```

## Individual targets

The Makefile has granular targets if needed:

```
make cluster-up       # create Kind cluster
make cluster-down     # delete Kind cluster
make infra            # deploy NATS via Ansible
make build-images     # build all Docker images
make load-images      # load images into Kind
make deploy-all       # helm install all services
make all              # everything
```
