#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "node_pointer.h"
#include "out123_int.h"

using namespace v8;
using namespace node;

extern mpg123_module_t mpg123_output_module_info;

namespace {

struct write_req {
  uv_work_t req;
  out123_handle *ao;
  unsigned char *buffer;
  int len;
  int written;
  Nan::Callback *callback;
};

NAN_METHOD(Create) {
  Nan::EscapableHandleScope scope;

  out123_handle *ao = out123_new();

  if (!ao) {
    out123_del(ao);
  } else if (out123_open(ao, NULL, NULL) != OUT123_OK) {
    out123_del(ao);
  }

  info.GetReturnValue().Set(scope.Escape(WrapPointer(ao, static_cast<uint32_t>(sizeof ao))));
}

NAN_METHOD(Open) {
  Nan::EscapableHandleScope scope;
  int r;
  out123_handle *ao = UnwrapPointer<out123_handle *>(info[0]);

  int channels = info[1]->Int32Value(); /* channels */
  long rate = (long) info[2]->Int32Value(); /* sample rate */
  int encoding = info[3]->Int32Value(); /* MPG123_ENC_* format */

  r = out123_start(ao, rate, channels, encoding);
  if (r || !ao) {
    out123_del(ao);
  }

  info.GetReturnValue().Set(scope.Escape(Nan::New<v8::Integer>(r)));
}

void write_async (uv_work_t *);
void write_after (uv_work_t *);

NAN_METHOD(Write) {
  Nan::HandleScope scope;
  out123_handle *ao = UnwrapPointer<out123_handle *>(info[0]);
  unsigned char *buffer = UnwrapPointer<unsigned char *>(info[1]);
  int len = info[2]->Int32Value();

  write_req *req = new write_req;
  req->ao = ao;
  req->buffer = buffer;
  req->len = len;
  req->written = 0;
  req->callback = new Nan::Callback(info[3].As<Function>());

  req->req.data = req;

  uv_queue_work(Nan::GetCurrentEventLoop(), &req->req, write_async, (uv_after_work_cb)write_after);

  info.GetReturnValue().SetUndefined();
}

void write_async (uv_work_t *req) {
  write_req *wreq = reinterpret_cast<write_req *>(req->data);
  wreq->written = out123_play(wreq->ao, wreq->buffer, wreq->len);
}

void write_after (uv_work_t *req) {
  Nan::HandleScope scope;
  write_req *wreq = reinterpret_cast<write_req *>(req->data);

  Local<Value> argv[] = {
    Nan::New(wreq->written)
  };

  Nan::Call(*wreq->callback, 1, argv);

  delete wreq->callback;
}

NAN_METHOD(Flush) {
  Nan::HandleScope scope;
  out123_handle *ao = UnwrapPointer<out123_handle *>(info[0]);
  out123_drain(ao);
  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(Close) {
  Nan::EscapableHandleScope scope;
  out123_handle *ao = UnwrapPointer<out123_handle *>(info[0]);
  int r = 0;
  if (ao) {
    out123_close(ao);
    r = 1;
  }
  info.GetReturnValue().Set(scope.Escape(Nan::New<v8::Integer>(r)));
}

void Initialize(Handle<Object> target) {
  Nan::HandleScope scope;
  Nan::DefineOwnProperty(target,
                Nan::New("api_version").ToLocalChecked(),
                Nan::New(mpg123_output_module_info.api_version));
  Nan::DefineOwnProperty(target,
                Nan::New("name").ToLocalChecked(),
                Nan::New(mpg123_output_module_info.name).ToLocalChecked());
  Nan::DefineOwnProperty(target,
                Nan::New("description").ToLocalChecked(),
                Nan::New(mpg123_output_module_info.description).ToLocalChecked());
  Nan::DefineOwnProperty(target,
                Nan::New("revision").ToLocalChecked(),
                Nan::New(mpg123_output_module_info.revision).ToLocalChecked());

  out123_handle *ao = out123_new();
  out123_open(ao, NULL, NULL);
  Nan::DefineOwnProperty(target, Nan::New("formats").ToLocalChecked(), Nan::New(out123_encodings(ao, -1, -1)));
  out123_del(ao);

#define CONST_INT(value) \
  Nan::DefineOwnProperty(target, Nan::New(#value).ToLocalChecked(), Nan::New(value), \
      static_cast<PropertyAttribute>(ReadOnly|DontDelete));

  CONST_INT(MPG123_ENC_FLOAT_32);
  CONST_INT(MPG123_ENC_FLOAT_64);
  CONST_INT(MPG123_ENC_SIGNED_8);
  CONST_INT(MPG123_ENC_UNSIGNED_8);
  CONST_INT(MPG123_ENC_SIGNED_16);
  CONST_INT(MPG123_ENC_UNSIGNED_16);
  CONST_INT(MPG123_ENC_SIGNED_24);
  CONST_INT(MPG123_ENC_UNSIGNED_24);
  CONST_INT(MPG123_ENC_SIGNED_32);
  CONST_INT(MPG123_ENC_UNSIGNED_32);

  Nan::SetMethod(target, "create", Create);
  Nan::SetMethod(target, "open", Open);
  Nan::SetMethod(target, "write", Write);
  Nan::SetMethod(target, "flush", Flush);
  Nan::SetMethod(target, "close", Close);
}

} // anonymous namespace

NODE_MODULE(binding, Initialize)
