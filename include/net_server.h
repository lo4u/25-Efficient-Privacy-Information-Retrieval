#pragma once

#include <memory>
#include <fstream>

#include "pir.h"
#include "pir_io.h"
#include "PIR_server.h"
#include "okvs.h"
#include "nlohmann/json.hpp"
#include "muduo/base/Logging.h"
#include "muduo/net/TcpServer.h"
#include "muduo/net/EventLoop.h"


struct Config {
  int port_;
  int maxClients_;
  string logDir_;

  // pir设置
  Config_PIR databaseConfig_;

  // 构造函数
  Config(const string& configFile) {
    load(configFile);
  }

  // 默认构造函数
  Config() = default;

  // 载入配置文件
  void load(const string& configFile) {
    std::ifstream in(configFile);
    nlohmann::json js;
    in >> js;

    port_ = js["port"];
    maxClients_ = js["max_clients"];
    logDir_ = js["log_dir"];
    databaseConfig_ = Config_PIR(js["databaseConfig"]);
  }
};

class NetServer {
public:
  // 默认构造函数
  NetServer(): NetServer("./config.json") {}

  // 带配置信息的构造函数 (委托构造函数)
  NetServer(const string& configFile):
    config_(configFile),
    pLoop_(std::make_unique<muduo::net::EventLoop>()),
    pServer_(std::make_unique<muduo::net::TcpServer>(pLoop_.get(),
      muduo::net::InetAddress(config_.port_),
      "PIRServer",
      muduo::net::TcpServer::kReusePort)),
    pPIRServer_(std::make_unique<PIRServer>(config_.databaseConfig_)) {
    pServer_->setConnectionCallback(
      std::bind(&NetServer::onConnection, this, std::placeholders::_1));
    pServer_->setMessageCallback(
      std::bind(&NetServer::onMessage, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3));
  }

  void start() {
    pServer_->start();
    pLoop_->loop();
  }

private:
  Config config_;
  std::unique_ptr<muduo::net::EventLoop> pLoop_;
  std::unique_ptr<muduo::net::TcpServer> pServer_;
  std::unique_ptr<PIRServer> pPIRServer_;

  void onConnection(const muduo::net::TcpConnectionPtr& conn) {
    if (conn->connected()) {
      LOG_INFO << "New connection from " << conn->peerAddress().toIpPort();
    } else {
      LOG_INFO << "Connection disconnected from " << conn->peerAddress().toIpPort();
    }
  }

  void onMessage(const muduo::net::TcpConnectionPtr& conn,
                  muduo::net::Buffer *buf,
                  muduo::Timestamp receiveTime) {
    // 读取消息
    std::stringstream msg;
    msg << buf->retrieveAllAsString(); 
    size_t msgSize = msg.str().size();
    LOG_INFO << "Received message of size " << msgSize;

    // // 处理PIR查询
    // std::stringstream response = pPIRServer_->processQueries(msg);

    // // 发送响应
    // conn->send(response.str());
    // LOG_INFO << "Sent response of size " << response.str().size();

    // TEST
    LOG_INFO << "Received message: " << msg.str();
    conn->send(msg.str());
    LOG_INFO << "Echoed back the message.";
  }
};