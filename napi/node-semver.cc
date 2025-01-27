#include <napi.h>
#include "version_weaver.h"

Napi::Value CoerceWrapped(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1 || !info[0].IsString()) {
    return env.Null();
  }

  std::string arg0 = info[0].As<Napi::String>().Utf8Value();
  std::optional<std::string> result = version_weaver::coerce(arg0);

  return result.has_value() ? Napi::String::New(env, result.value()) : env.Null();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "coerce"), Napi::Function::New(env, CoerceWrapped));
  return exports;
}

NODE_API_MODULE(myaddon, Init)
