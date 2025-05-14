#include <napi.h>
#include <gst/gst.h>
#include "object_pool.h"

static BufferPool input_pool, output_pool;

typedef struct {
  GstElement* pipeline;
  GstElement* appsrc;
  GstElement* appsink;
} GstPipeline;

Napi::Value Process(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  GstPipeline* pipe = (GstPipeline*)info.Data();

  Buffer* input = pool_acquire(&input_pool);
  memcpy(input->ptr, info[0].As<Napi::Buffer<uint8_t>>().Data(), input->size);

  GstBuffer* gst_buf = gst_buffer_new_wrapped(input->ptr, input->size);
  GST_BUFFER_DTS(gst_buf) = GST_CLOCK_TIME_NONE;
  GST_BUFFER_PTS(gst_buf) = GST_CLOCK_TIME_NONE;

  GstFlowReturn ret;
  g_signal_emit_by_name(pipe->appsrc, "push-buffer", gst_buf, &ret);
  gst_buffer_unref(gst_buf);

  Buffer* output = pool_acquire(&output_pool);
  GstSample* sample = NULL;
  g_signal_emit_by_name(pipe->appsink, "pull-sample", &sample);

  if (sample) {
    GstBuffer* outbuf = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(outbuf, &map, GST_MAP_READ);
    memcpy(output->ptr, map.data, map.size);
    gst_buffer_unmap(outbuf, &map);
    gst_sample_unref(sample);
  }

  pool_release(&input_pool, input);
  return Napi::Buffer<uint8_t>::Copy(env, output->ptr, output->size);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  GstPipeline* pipe = (GstPipeline*)malloc(sizeof(GstPipeline));
  gst_init(NULL, NULL);

  pool_init(&input_pool, 10, 640 * 480 * 4);  // RGBA
  pool_init(&output_pool, 10, 640 * 480 * 3); // JPEG

  pipe->pipeline = gst_parse_launch(
    "appsrc name=source is-live=true format=time ! "
    "video/x-raw,format=RGBA,width=640,height=480,framerate=30/1 ! "
    "videocrop top=0 bottom=0 left=0 right=0 ! videoconvert ! videoscale ! "
    "video/x-raw,width=640,height=480 ! videoconvert ! jpegenc ! appsink name=sink",
    NULL
  );

  pipe->appsrc = gst_bin_get_by_name(GST_BIN(pipe->pipeline), "source");
  pipe->appsink = gst_bin_get_by_name(GST_BIN(pipe->pipeline), "sink");

  gst_element_set_state(pipe->pipeline, GST_STATE_PLAYING);

  Napi::FunctionReference* constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(Napi::Function::New(env, Process, "Process", (void*)pipe));

  exports.Set("process", *constructor);
  return exports;
}

Napi::Object InitModule(Napi::Env env, Napi::Object exports) {
  return Init(env, exports);
}

NODE_API_MODULE(gst_bridge, InitModule)