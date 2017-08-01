// MIT License
// Copyright (c) 2017 Zhuhao Wang

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "nekit/instance.h"

#include "nekit/utils/log.h"
#include "nekit/utils/runtime.h"
#include "nekit/utils/system_resolver.h"

#undef NECHANNEL
#define NECHANNEL "Instance"

namespace nekit {
using nekit::utils::LogLevel;

Instance::Instance(std::string name) : name_{name}, io_{} {
  resolver_ = std::make_unique<utils::SystemResolver>(io_);
}

void Instance::SetRuleSet(std::unique_ptr<rule::RuleSet> &&rule_set) {
  rule_set_ = std::move(rule_set);
  NEDEBUG << "Set new rules for instance " << name_ << ".";
}

void Instance::AddListener(
    std::unique_ptr<transport::ServerListenerInterface> &&listener) {
  listeners_.push_back(std::move(listener));
  NEDEBUG << "Add new listener to instance " << name_ << ".";
}

void Instance::Run() {
  assert(rule_set_);

  BOOST_LOG_SCOPED_THREAD_ATTR(
      "Instance", boost::log::attributes::constant<std::string>(name_));

  for (auto &listener : listeners_) {
    listener->Accept(
        [this](std::unique_ptr<transport::ConnectionInterface> &&conn,
               std::unique_ptr<stream_coder::ServerStreamCoderInterface>
                   &&stream_coder,
               std::error_code ec) {
          if (ec) {
            NEERROR << "Error happened when accepting new socket " << ec;
            exit(1);
          }

          auto tunnel = std::make_unique<transport::Tunnel>(
              std::move(conn), std::move(stream_coder));
          tunnel->Open();
          tunnels_[tunnel.get()] = std::move(tunnel);
        });
  }

  if (!resolver_) {
    NEINFO << "No resolver is specified, using system default.";
    resolver_ = std::make_unique<utils::SystemResolver>(io_);
  }

  NEDEBUG << "Setting up runtime information.";
  SetUpRuntime();

  NEINFO << "Start running instance.";
  io_.run();

  NEINFO << "Instance stopped.";
}

void Instance::Stop() { io_.stop(); }

void Instance::Reset() {
  // TODO: Reset the instance so it can run again.
}

boost::asio::io_service &Instance::io() { return io_; }

void Instance::SetUpRuntime() {
  utils::Runtime::CurrentRuntime().SetRuleSet(rule_set_.get());
  utils::Runtime::CurrentRuntime().SetResolver(resolver_.get());
  utils::Runtime::CurrentRuntime().SetIoService(&io_);
}
}  // namespace nekit