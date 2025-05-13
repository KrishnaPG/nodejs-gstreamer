// src/index.ts
import { App } from "uWebSockets.js";
//import { WASMGstBridge } from "./wasm-gst-bridge"; // Hypothetical bridge

const PORT = 8080;

const server = App()
  .ws("/*", {
    maxPayloadLength: 1024 * 1024 * 10,
    open: (ws) => {
      console.log("A WebSocket connected!");
    },
    message: (ws, message, isBinary) => {
      const buffer = Buffer.from(message);

      // // Pipe to GStreamer
      // gstPipeline.write(buffer);

      // // Or use WASM bridge
      // const result = WASMGstBridge.process(buffer);

           console.log("Received data size:", buffer.size || buffer.byteLength);
           // For testing, just display as base64
           console.log(
             "First 50 chars:",
             btoa(String.fromCharCode.apply(null, new Uint8Array(buffer.data))).substring(0, 50)
           );      

      const result = Buffer.copyBytesFrom(buffer);
      ws.send(result, isBinary);
    },
    drain: (ws) => {
      console.log("WebSocket backpressure: " + ws.getBufferedAmount());
    },
    close: (ws, code, message) => {
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
