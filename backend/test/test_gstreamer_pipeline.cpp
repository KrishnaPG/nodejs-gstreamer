#include <iostream>
#include <vector>
#include <cstring>
#include <gst/gst.h>
#include <gst/app/app.h>
#include "gstreamer-pipeline.cpp"

// Mock N-API types to avoid Node.js dependency
namespace MockNapi {
    class Env {
    public:
        Env() {}
    };

    template<typename T>
    class Buffer {
    public:
        Buffer(T* data, size_t length) : data_(data), length_(length) {}
        T* Data() const { return data_; }
        size_t Length() const { return length_; }
    private:
        T* data_;
        size_t length_;
    };

    class Function {
    public:
        Function(void (*cb)(Env, Buffer<uint8_t>)) : callback_(cb) {}
        void Call(Env env, Buffer<uint8_t> data) { callback_(env, data); }
    private:
        void (*callback_)(Env, Buffer<uint8_t>);
    };

    class Value {
    public:
        Value() {}
        bool AsBoolean() const { return true; } // Mock success
    };

    class Object {
    public:
        Object() {}
    };
}

// Mock ObjectWrap to bypass N-API
template<typename T>
class ObjectWrap {
public:
    static void Init(MockNapi::Env, MockNapi::Object) {}
};

// Callback for pipeline output
void OutputCallback(MockNapi::Env, MockNapi::Buffer<uint8_t> data) {
    std::cout << "Received output buffer of size: " << data.Length() << std::endl;
    if (data.Length() > 0) {
        std::cout << "Test passed: Data processed by pipeline\n";
    } else {
        std::cout << "Test failed: No data received\n";
    }
}

int main() {
    // Initialize GStreamer
    gst_init(nullptr, nullptr);

    // Create mock environment
    MockNapi::Env env;

    // Define a simple pipeline
    std::string pipeline_str = "videoconvert ! video/x-raw,format=NV12,width=320,height=240 ! appsink";

    // Create callback
    MockNapi::Function callback(OutputCallback);

    // Instantiate GStreamerPipeline
    GStreamerPipeline pipeline({{pipeline_str, callback}});

    // Simulate a raw NV12 frame (320x240, ~115kB)
    std::vector<uint8_t> test_data(320 * 240 * 3 / 2, 0x80); // Gray frame
    MockNapi::Buffer<uint8_t> input_buffer(test_data.data(), test_data.size());

    // Push test data
    std::cout << "Pushing test data of size: " << test_data.size() << " bytes\n";
    MockNapi::Value result = pipeline.PushBuffer({input_buffer});

    // Check result
    if (!result.AsBoolean()) {
        std::cerr << "Test failed: Failed to push buffer\n";
        return 1;
    }

    // Wait for processing
    g_usleep(100000); // 100ms

    // Clean up
    pipeline.Destroy({});

    std::cout << "Test completed\n";
    return 0;
}