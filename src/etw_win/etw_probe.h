/*
Copyright (c) Microsoft Open Technologies, Inc.
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
disclaimer in the documentation and/or other materials provided with the distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once
#include <node.h>
#include <evntprov.h>
#include "provider_types.h"
#include "v8_compatibility.h"
#include <vector>
#include <memory>

using namespace node;
using namespace v8;

#define PROBE_CLASS_NAME "Probe"

class V8Probe: protected ObjectWrap {
public:
  static Handle<Value> New(V8Probe* probe);

  DEFINE_V8_CALLBACK(Fire)

  void FillArguments(const Local<Array>& input);

  class eEmptyArrayPassed {};
  class eArrayTooLarge {};
  class eArgumentTypesMismatch {
  public:
    eArgumentTypesMismatch(int argument_index): m_failed_argument_number(argument_index) {}
    int m_failed_argument_number;
  };

private:
  template <class T, class S>
  inline void SetValueHelper(IArgumentValue* argument, const S& value) {
    T val = (T) value;
    argument->SetValue(&value);
  }

  virtual void HandleArgumentValues(IArgumentValue* argument, unsigned int index) = 0;

protected:
  void AllocateArguments();

  ProbeArgumentsTypeInfo m_type_info;
  std::vector<std::unique_ptr<IArgumentValue>> m_argument_values;
};

class RealProvider;

/*
EtwEvent class represents a single event/probe that was added.
It knows about the arguments the probe has and contains a storage for their values.
The storage is allocated only once and remains in this state until the probe is destroyed.
*/
class RealProbe: public V8Probe {
public:
  RealProbe(const char* event_name, EVENT_DESCRIPTOR* descriptor, ProbeArgumentsTypeInfo& types);
  RealProbe(const char* event_name, int event_id, ProbeArgumentsTypeInfo& types);
  ~RealProbe();

  void Fire();
  bool IsBound() const;
  void Bind(RealProvider* provider);
  void Unbind();

private:
  void AllocateArguments() {
    int arg_number = m_type_info.GetArgsNumber();
    m_payload_descriptors.reset(new EVENT_DATA_DESCRIPTOR[arg_number]);
    V8Probe::AllocateArguments();
  }

  void HandleArgumentValues(IArgumentValue* argument, unsigned int index) {
    EVENT_DATA_DESCRIPTOR* descriptor = m_payload_descriptors.get() + index;
    EventDataDescCreate(descriptor, argument->GetValue(), argument->GetSize());
  }

  EVENT_DESCRIPTOR m_event_descriptor;
  std::unique_ptr<EVENT_DATA_DESCRIPTOR[]> m_payload_descriptors;
  std::string m_event_name;
  RealProvider* m_owner;
};

class JSONHandler {
  Persistent<Object> m_json_holder;
  Persistent<Function> m_stringify_holder;

public:
  JSONHandler() {
    HandleScope scope;
    Handle<Context> context = Context::GetCurrent();
    Handle<Object> global = context->Global();
    Handle<Object> l_JSON = global->Get(String::New("JSON"))->ToObject();
    Handle<Function> l_JSON_stringify = Handle<Function>::Cast(l_JSON->Get(String::New("stringify")));

    WrapInPersistent(m_json_holder, l_JSON);
    WrapInPersistent(m_stringify_holder, l_JSON_stringify);
  }

  std::string Stringify(Handle<Value> value) {
    HandleScope scope;

    Handle<Value> args[1] = {value};

#ifndef BUILD_PRE011_COMPATIBILITY
    Local<Function> js_stringify = Local<Function>::New(Isolate::GetCurrent(), m_stringify_holder); 
    Local<Object> js_object = Local<Object>::New(Isolate::GetCurrent(), m_json_holder); 
    Local<Value> j = js_stringify->Call(js_object, 1, args);
#else
    Local<Value> j = m_stringify_holder->Call(m_json_holder, 1, args);
#endif

    String::AsciiValue str(j->ToString());
    return std::string(*str);    
  }
};