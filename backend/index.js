// src/index.ts
import { App } from "uWebSockets.js";
import { GStreamerPipeline } from "./gstreamer-pipeline";

//import { WASMGstBridge } from "./wasm-gst-bridge"; // Hypothetical bridge

const PORT = 8080;

const server = App()
  .ws("/*", {

    maxPayloadLength: 1024 * 1024 * 10, // 10MB
    compression: 0, // Disable compression for lower latency
    maxBackpressure: 64 * 1024 * 1024, // 64MB
    idleTimeout: 0,

    open: (ws) => {
      console.log("A WebSocket connected!");
      const pipeline = new GStreamerPipeline("d3d11upload ! d3d11convert ! d3d11h264enc ! h264parse", (dataBuf) =>
        ws.send(dataBuf, true, false)
      ); // "vaapipostproc ! vaapih264enc ! h264parse"
      // pipeline.on("data", (dataBuf) => {
      //   // send the processed data from the GStreamer pipeline back to websocket client
      //   ws.send(dataBuf, true, false); // Binary, no compression
      // });
      ws.pipeline = pipeline;
    },
    message: (ws, message, isBinary) => {
      const buffer = Buffer.from(message);
      // send the incoming data to GStreamer pipeline for processing
      ws.pipeline.write(buffer);
    },
    drain: (ws) => {
      console.log("WebSocket backpressure: " + ws.getBufferedAmount());
    },
    close: (ws, code, message) => {
      ws.pipeline?.end();
      console.log("WebSocket closed");
    },
  })
  .listen(PORT, (token) => {
    if (!token) {
      console.error(`Failed to listen on port ${PORT}`);
    } else {
      console.log(`Server running on ws://localhost:${PORT}`);
    }
  });
