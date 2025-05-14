#include "native_gst_wrapper.h"
#include "gst_bridge.c" // Embed bridge logic here or separate as needed

Napi::Object InitModule(Napi::Env env, Napi::Object exports) {
  return Init(env, exports);
}