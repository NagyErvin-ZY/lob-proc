FROM node:20-alpine AS frontend
WORKDIR /app
COPY frontend/package.json frontend/package-lock.json* ./
RUN npm install
COPY frontend/ .
RUN npm run build

FROM golang:1.22-alpine AS builder
WORKDIR /app
COPY server/go.mod server/go.sum ./
RUN go mod download
COPY server/ .
RUN CGO_ENABLED=0 go build -o server .

FROM alpine:3.20
COPY --from=builder /app/server /usr/local/bin/server
COPY --from=frontend /app/dist /app/dist
CMD ["server"]
