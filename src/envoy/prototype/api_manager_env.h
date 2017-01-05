#pragma once

#include "precompiled/precompiled.h"

#include "common/common/logger.h"
#include "common/http/header_map_impl.h"
#include "contrib/endpoints/include/api_manager/env_interface.h"
#include "envoy/upstream/cluster_manager.h"
#include "server/server.h"

namespace Http {
namespace ApiManager {

class Env;

class GrpcStreamingRequestCallbacks : public AsyncClient::StreamingCallbacks {
 private:
  Env* env_;
  std::unique_ptr<google::api_manager::GRPCRequest> request_;
  AsyncClient::StreamingRequest* underlying_request_;
  HeaderMapImpl headers_;

 public:
  GrpcStreamingRequestCallbacks(Env *env, HeaderMap& header) : env_(env), headers_(header) {}
  void onHeaders(HeaderMapPtr &&headers, bool end_stream) override;
  void onData(Buffer::Instance &data, bool end_stream) override;
  void onTrailers(HeaderMapPtr &&trailers) override;
  void onResetStream() override;
  AsyncClient::StreamingRequest* request() { return underlying_request_; }
  void set_request(AsyncClient::StreamingRequest *request) { underlying_request_ = request; }
  HeaderMap& headers() { return headers_; }
  void sendGRPCRequest(std::unique_ptr<google::api_manager::GRPCRequest> request);
};


class Env : public google::api_manager::ApiManagerEnvInterface,
            public Logger::Loggable<Logger::Id::http> {
 private:
  Server::Instance& server;
  Upstream::ClusterManager& cm_;
  std::unique_ptr<GrpcStreamingRequestCallbacks> streaming_callbacks_{nullptr};

 public:
  Env(Server::Instance& server)
      : server(server), cm_(server.clusterManager()){};

  virtual void Log(LogLevel level, const char* message) override;
  virtual std::unique_ptr<google::api_manager::PeriodicTimer>
  StartPeriodicTimer(std::chrono::milliseconds interval,
                     std::function<void()> continuation) override;
  virtual void RunHTTPRequest(
      std::unique_ptr<google::api_manager::HTTPRequest> request) override;
  virtual void RunGRPCRequest(
      std::unique_ptr<google::api_manager::GRPCRequest> request) override;
};
}
}
