// MIT License

// Copyright (c) 2018 Zhuhao Wang

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

#pragma once

#include "../utils/http_message_stream_rewriter.h"
#include "local_data_flow_interface.h"

namespace nekit {
namespace data_flow {
class HttpServerDataFlow : public LocalDataFlowInterface {
 public:
  enum class ErrorCode {
    NoError = 0,
    DataBeforeConnectRequestFinish,
    InvalidRequest,
  };

  HttpServerDataFlow(std::unique_ptr<LocalDataFlowInterface>&& data_flow,
                     std::shared_ptr<utils::Session> session);
  ~HttpServerDataFlow();

  utils::Cancelable Read(std::unique_ptr<utils::Buffer>&&,
                         DataEventHandler) override
      __attribute__((warn_unused_result));
  utils::Cancelable Write(std::unique_ptr<utils::Buffer>&&,
                          EventHandler) override
      __attribute__((warn_unused_result));

  utils::Cancelable CloseWrite(EventHandler) override
      __attribute__((warn_unused_result));

  bool IsReadClosed() const override;
  bool IsWriteClosed() const override;
  bool IsWriteClosing() const override;

  bool IsReading() const override;
  bool IsWriting() const override;

  data_flow::State State() const override;

  data_flow::DataFlowInterface* NextHop() const override;

  data_flow::DataType FlowDataType() const override;

  std::shared_ptr<utils::Session> Session() const override;

  boost::asio::io_context* io() override;

  utils::Cancelable Open(EventHandler) override
      __attribute__((warn_unused_result));

  utils::Cancelable Continue(EventHandler) override
      __attribute__((warn_unused_result));

  utils::Cancelable ReportError(std::error_code, EventHandler) override
      __attribute__((warn_unused_result));

  LocalDataFlowInterface* NextLocalHop() const override;

  bool OnMethod();

  bool OnUrl();

  bool OnVersion();

  bool OnStatus();

  bool OnHeaderPair();

  bool OnHeaderComplete();

  bool OnMessageComplete(size_t buffer_offset, bool upgrade);

 private:
  void EnsurePendingBuffer();
  void NegotiateRead(EventHandler handler);

  std::unique_ptr<LocalDataFlowInterface> data_flow_;
  std::shared_ptr<utils::Session> session_;

  bool reporting_{false},  // Reporting error
      reportable_{false},  // Can report error
      reading_{false},     // Processing reading request
      writing_{false},     // Processing writing request
      opening_{false},     // Negotiating
      read_closed_{false}, write_closed_{false};

  bool is_connect_{false}, has_read_method_{false}, reading_first_header_{true};
  size_t first_header_offset_{0};

  data_flow::State state_{data_flow::State::Closed};

  utils::Cancelable open_cancelable_, read_cancelable_, write_cancelable_;

  std::unique_ptr<utils::Buffer> first_header_;

  utils::HttpMessageStreamRewriter rewriter_;
  http_parser_url url_parser_;
};

std::error_code make_error_code(HttpServerDataFlow::ErrorCode ec);
}  // namespace data_flow
}  // namespace nekit

namespace std {
template <>
struct is_error_code_enum<nekit::data_flow::HttpServerDataFlow::ErrorCode>
    : true_type {};
}  // namespace std
