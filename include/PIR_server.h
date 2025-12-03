#pragma once

#include <memory>
#include <fstream>

#include "pir.h"
#include "pir_io.h"
#include "okvs.h"
#include "nlohmann/json.hpp"

struct Config_PIR {
  std::string databaseFile_;
  int batch_threshold;

  Config_PIR() = default;
  Config_PIR(const Config_PIR& other) = default;
  Config_PIR(const nlohmann::json& js) {
    load(js);
  }

  void load(const nlohmann::json& js) {
    databaseFile_ = js["database"];
    batch_threshold = js["batch_threshold"];
  }
};

class PIRServer {
public:
  // 默认构造函数
  PIRServer() = delete;

  // 带配置信息的构造函数
  PIRServer(const Config_PIR& config): config_(config) {
    build_table();
    ntt_qprime = new intel::hexl::NTT(2048, arb_qprime);
    setup_constants();
    generate_gadgets();
    auto rawDB = loadFile(config_.databaseFile_, total_n * n0 * n0 * poly_len);
    uint64_t* ptr;
    genDataBaseFromRawDB(ptr, num_expansions, further_dims, rawDB);
    pDB_ = std::unique_ptr<uint64_t[]>(ptr);

  }

  std::stringstream processQueries(std::stringstream& queriesStream);

private:
  std::unique_ptr<uint64_t[]> pDB_;
  int cuckooNum_ = 3;
  double bucketRate_ = 1.5;
  double hyperGraphRate_ = 1.3;
  Config_PIR config_;

  std::vector<MatPoly> Answer(size_t batchSize, std::vector<MatPoly>& cvs_hat, uint16_t counter);

  CuckooCode getCuckooCode(int batchSize) {
    return CuckooCode(batchSize, cuckooNum_, bucketRate_);
  }
};