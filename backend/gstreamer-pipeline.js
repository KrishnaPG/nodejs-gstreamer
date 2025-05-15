import path from "node:path";
import { Transform } from "stream";
import { fileURLToPath } from "url";

import { createRequire } from "node:module";
const require = createRequire(import.meta.url);
const __filename = fileURLToPath(import.meta.url); // get the resolved path to the file
const __dirname = path.dirname(__filename); // get the name of the directory

const native = require(process.env.NODE_ENV === "development"
  ? path.resolve(__dirname, "build", "Debug", "gstzerocopy.node")
  : path.resolve(__dirname, "build", "Release", "gstzerocopy.node"));

export class GStreamerPipeline extends Transform {
  constructor(pipelineStr, callback) {
    super({ objectMode: true });
    this.nativePipeline = new native.GStreamerPipeline(pipelineStr, callback);
  }

  _transform(chunk, encoding, callback) {
    const success = this.nativePipeline.pushBuffer(chunk);
    if (!success) {
      this.emit("error", new Error("GStreamer push failed"));
    }
    callback();
  }

  _flush(callback) {
    this.nativePipeline.destroy();
    callback();
  }
}
