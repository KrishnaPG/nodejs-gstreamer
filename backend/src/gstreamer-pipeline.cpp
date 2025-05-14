#include <napi.h>
#include <gst/gst.h>
#include <gst/app/app.h>
#include <memory>
#include <vector>
#include <mutex>

#define ENABLE_D3D11   0

#if ENABLE_D3D11
#include <gst/d3d11/gstd3d11.h>
#endif

// RAII wrapper for GStreamer resources
struct GstResource
{
    GstElement* element = nullptr;
    explicit GstResource(GstElement* elem) : element(elem) { }
    ~GstResource() { if (element) gst_object_unref(element); }
};

// Main pipeline class
class GStreamerPipeline : public Napi::ObjectWrap<GStreamerPipeline>
{
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    GStreamerPipeline(const Napi::CallbackInfo& info);
    ~GStreamerPipeline();

private:
    static Napi::FunctionReference constructor;
    std::unique_ptr<GstResource> pipeline;
    std::unique_ptr<GstResource> appsrc;
    std::unique_ptr<GstResource> appsink;
    std::mutex buffer_mutex;
    Napi::ThreadSafeFunction tsfn;
    std::vector<GstBuffer*> buffer_pool;

    // Methods
    Napi::Value PushBuffer(const Napi::CallbackInfo& info);
    Napi::Value Destroy(const Napi::CallbackInfo& info);
    static GstFlowReturn OnNewSample(GstAppSink* sink, gpointer user_data);
    void AllocateBufferPool(size_t count, size_t size);
};

// Static initialization
static struct GstInitializer
{
    GstInitializer()
    {
        gst_init(nullptr, nullptr);
#ifdef _WIN32
        gst_plugin_load_by_name("d3d11");
#else
        gst_plugin_load_by_name("vaapi");
#endif
    }
} _initializer;

// Initialize N-API module
Napi::Object GStreamerPipeline::Init(Napi::Env env, Napi::Object exports)
{
    Napi::Function func = DefineClass(env, "GStreamerPipeline", {
        InstanceMethod("pushBuffer", &GStreamerPipeline::PushBuffer),
        InstanceMethod("destroy", &GStreamerPipeline::Destroy)
        });
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("GStreamerPipeline", func);
    return exports;
}

// Constructor
GStreamerPipeline::GStreamerPipeline(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<GStreamerPipeline>(info)
{
    Napi::Env env = info.Env();

    // Validate pipeline string
    if (!info[0].IsString())
    {
        Napi::TypeError::New(env, "Pipeline string required").ThrowAsJavaScriptException();
        return;
    }
    std::string pipeline_str = info[0].ToString();

    // Create pipeline
    GError* error = nullptr;
    GstElement* pipeline_elem = gst_parse_launch(("appsrc name=src ! " + pipeline_str + " ! appsink name=sink").c_str(), &error);
    if (!pipeline_elem || error)
    {
        std::string err_msg = error ? error->message : "Failed to create pipeline";
        g_clear_error(&error);
        Napi::Error::New(env, err_msg).ThrowAsJavaScriptException();
        return;
    }
    pipeline = std::make_unique<GstResource>(pipeline_elem);

    // Configure appsrc
    appsrc = std::make_unique<GstResource>(gst_bin_get_by_name(GST_BIN(pipeline_elem), "src"));
    g_object_set(appsrc->element, "stream-type", GST_APP_STREAM_TYPE_STREAM, "format", GST_FORMAT_TIME, nullptr);

    // Configure appsink
    appsink = std::make_unique<GstResource>(gst_bin_get_by_name(GST_BIN(pipeline_elem), "sink"));
    g_object_set(appsink->element, "emit-signals", TRUE, "sync", FALSE, nullptr);
    g_signal_connect(appsink->element, "new-sample", G_CALLBACK(OnNewSample), this);

    // Pre-allocate buffer pool (10x 4MB)
    AllocateBufferPool(10, 4 * 1024 * 1024);

    // Thread-safe callback for output
    tsfn = Napi::ThreadSafeFunction::New(
        env, info[1].As<Napi::Function>(), "GStreamerCallback", 0, 1, [](Napi::Env) { });

    // Start pipeline
    if (gst_element_set_state(pipeline_elem, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        Napi::Error::New(env, "Failed to start pipeline").ThrowAsJavaScriptException();
    }
}

// Destructor
GStreamerPipeline::~GStreamerPipeline()
{
    if (pipeline && pipeline->element)
    {
        gst_element_set_state(pipeline->element, GST_STATE_NULL);
    }
    tsfn.Release();
}

// Pre-allocate reusable GstBuffers
void GStreamerPipeline::AllocateBufferPool(size_t count, size_t size)
{
    std::lock_guard<std::mutex> lock(buffer_mutex);
    for (size_t i = 0; i < count; i++)
    {
#ifdef _WIN32
#if ENABLE_D3D11
        // Windows: Use D3D11 allocator for zero-copy
        GstMemory* mem = gst_d3d11_allocator_alloc(nullptr, size, nullptr);
#else
        GstMemory* mem = gst_allocator_alloc(nullptr, size, nullptr); // Fallback
#endif
#else
        // Linux: Prefer DMA-BUF if available
        GstMemory* mem = gst_fd_allocator_alloc(nullptr, size, nullptr);
        if (!mem) mem = gst_allocator_alloc(nullptr, size, nullptr); // Fallback
#endif
        GstBuffer* buf = gst_buffer_new();
        gst_buffer_append_memory(buf, mem);
        buffer_pool.push_back(buf);
    }
}

// Push buffer from Node.js to GStreamer
Napi::Value GStreamerPipeline::PushBuffer(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (!info[0].IsBuffer())
    {
        Napi::TypeError::New(env, "Buffer required").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Buffer<uint8_t> js_buffer = info[0].As<Napi::Buffer<uint8_t>>();
    GstBuffer* gst_buf = nullptr;
    {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        if (!buffer_pool.empty())
        {
            gst_buf = buffer_pool.back();
            buffer_pool.pop_back();
        }
    }

    if (!gst_buf)
    {
        // Allocate new if pool is empty
#ifdef _WIN32
#if ENABLE_D3D11
        GstMemory* mem = gst_d3d11_allocator_alloc(nullptr, js_buffer.Length(), nullptr);
#else
#endif
        GstMemory* mem = gst_allocator_alloc(nullptr, js_buffer.Length(), nullptr);
#else
        GstMemory* mem = gst_fd_allocator_alloc(nullptr, js_buffer.Length(), nullptr);
        if (!mem) mem = gst_allocator_alloc(nullptr, js_buffer.Length(), nullptr);
#endif
        gst_buf = gst_buffer_new();
        gst_buffer_append_memory(gst_buf, mem);
    }

    // Map and write directly (minimal copy)
    GstMapInfo map;
    if (!gst_buffer_map(gst_buf, &map, GST_MAP_WRITE))
    {
        gst_buffer_unref(gst_buf);
        Napi::Error::New(env, "Failed to map buffer").ThrowAsJavaScriptException();
        return env.Null();
    }
    memcpy(map.data, js_buffer.Data(), js_buffer.Length());
    gst_buffer_unmap(gst_buf, &map);

    // Push to pipeline
    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc->element), gst_buf);
    if (ret != GST_FLOW_OK)
    {
        Napi::Error::New(env, "Failed to push buffer").ThrowAsJavaScriptException();
        return env.Null();
    }
    return Napi::Boolean::New(env, true);
}

// Handle GStreamer output
GstFlowReturn GStreamerPipeline::OnNewSample(GstAppSink* sink, gpointer user_data)
{
    GStreamerPipeline* self = static_cast<GStreamerPipeline*>(user_data);
    GstSample* sample = gst_app_sink_pull_sample(sink);
    if (!sample) return GST_FLOW_ERROR;

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ))
    {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    // Pass to Node.js without copying
    self->tsfn.NonBlockingCall([buffer, &map](Napi::Env env, Napi::Function cb)
        {
            Napi::Buffer<uint8_t> js_buffer = Napi::Buffer<uint8_t>::New(
                env, map.data, map.size,
                [&map](Napi::Env, void*, GstBuffer* buf)
                {
                    gst_buffer_unmap(buf, &map);
                    gst_buffer_unref(buf);
                }, buffer);
            cb.Call({ js_buffer });
        });

    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

// Explicit destroy method
Napi::Value GStreamerPipeline::Destroy(const Napi::CallbackInfo& info)
{
    if (pipeline && pipeline->element)
    {
        gst_element_set_state(pipeline->element, GST_STATE_NULL);
    }
    std::lock_guard<std::mutex> lock(buffer_mutex);
    for (GstBuffer* buf : buffer_pool)
    {
        gst_buffer_unref(buf);
    }
    buffer_pool.clear();
    return info.Env().Undefined();
}

// N-API module entry
Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    return GStreamerPipeline::Init(env, exports);
}
NODE_API_MODULE(gstzerocopy, Init)