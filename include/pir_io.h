#pragma once

#include "pir.h"


class ArgParser {
public:
  ArgParser(int argc, char *argv[]) {
    parse(argc, argv);
  }

  ArgParser() {}

  void parse(int argc, char *argv[]) {
    std::string current_key;
    for (int i = 1; i < argc; ++i) {
      std::string token = argv[i];
      if (token.rfind("--", 0) == 0) {    // 若是以--开头，则是一个key
        current_key = token.substr(2);
        args_[current_key] = {};
      } else if (!current_key.empty()) {  // 存入刚才的key中
        args_[current_key].push_back(token);
      } else {
        throw std::runtime_error("Invalid argument format: " + token);
      }
    }
  }

  bool has(const std::string &name) const {
    return args_.count(name) > 0;
  }

  // 输出单个参数
  std::string get(const std::string &name) {
    auto iter = args_.find(name);
    if (iter != args_.end() && !iter->second.empty())
      return iter->second[0];
    return "";
  }

  // 输出多个参数，针对queries等
  std::vector<std::string> getList (const std::string &name) {
    auto iter = args_.find(name);
    if (iter != args_.end() && !iter->second.empty())
      return iter->second;
    return {};
  }
private:
  std::unordered_map<std::string, std::vector<std::string>> args_;
};

std::vector<uint64_t> loadFile(const std::string filename, const size_t ntotal, const int entrySize=1);

void genDataBaseFromRawDB(uint64_t* &DB, const size_t num_expansions, const size_t further_dims, const std::vector<uint64_t>& rawDB, const int entrySize=1);

inline std::pair<size_t, size_t> getIdxAndOffline(size_t idx) {
  size_t matIndex = idx / (n0 * n0 * poly_len);
  size_t offline = idx % (n0 * n0 * poly_len);

  return { matIndex, offline };
}
