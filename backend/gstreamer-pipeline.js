import { join } from "path";
import { Transform } from "stream";

const native = require(join(__dirname, "build", "Release", "gstzerocopy.node"));

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
