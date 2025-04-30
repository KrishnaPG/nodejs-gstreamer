# GStreamer Worker Service

A high-performance, asynchronous GStreamer-based media worker that receives JSON-RPC commands over ZeroMQ to launch dynamic pipelines.

Supports:
- Shared Memory (`shmsink`) for zero-copy communication
- ZeroMQ PUB/SUB logging and control
- Hardware acceleration detection
- Multiple concurrent pipelines
- Manual stop & auto cleanup

Built using modern C++23 and GLib integration.

---

## 🔧 Features

- Accepts JSON-RPC 2.0 commands over ZeroMQ PUB/SUB
- Supports `shmsink` and `zmqsink` transports
- Logs published over ZMQ topic `log.pipeline`
- Pipelines launched asynchronously via thread pool
- UUID-based stream IDs
- Metrics collection per pipeline
- Auto-cleanup on completion or error

---

## 🛠️ Build

```bash
make
```

## 🚀 Run
```sh
./worker
```

## 🐳 Docker Build & Run
```sh
docker build -t gstreamer-worker .
docker run -d 
  --name=gstreamer-worker \
  -v /tmp:/tmp \
  -p 5555:5555 \
  gstreamer-worker
```

## 🧪 Example JSON-RPC Command
```json
{
  "jsonrpc": "2.0",
  "method": "start_pipeline",
  "params": {
    "pipeline": "rtspsrc location=rtsp://camera ! rtph264depay ! h264parse",
    "transport": "shm",
    "streamId": "abc123"
  },
  "id": "req_789"
}
```

## 📦 License
MIT