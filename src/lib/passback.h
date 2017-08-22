#ifndef K_QP_H_
#define K_QP_H_

namespace K {
static json qpRepo;

void passback(const FunctionCallbackInfo<Value>& args) {
  Isolate * isolate = args.GetIsolate();

  v8::String::Utf8Value s(args[0]);
  std::string str(*s);
  std::reverse(str.begin(), str.end());

  Local<String> retval = String::NewFromUtf8(isolate, str.c_str());
  args.GetReturnValue().Set(retval);
}

}

void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "passback", passback);
}

#endif
