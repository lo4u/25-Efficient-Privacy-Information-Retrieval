#include "PIR_server.h"

std::stringstream PIRServer::processQueries(std::stringstream& queriesStream) {
  std::stringstream out(std::ios::in | std::ios::out | std::ios::binary);
  std::vector<MatPoly> cvs_hat;
  size_t batchSize;
  uint16_t counter;
  queriesStream.read(reinterpret_cast<char*>(&batchSize), sizeof(batchSize));
  queriesStream.read(reinterpret_cast<char*>(&counter), sizeof(counter));
  int m = ceil(batchSize * hyperGraphRate_);
  cvs_hat.reserve(m);

  for (int i = 0; i < m; ++i) {
    MatPoly mat;
    queriesStream >> mat;
    cvs_hat.push_back(mat);
  }
  assert(!queriesStream.get() && "could not read the entire query stream");
    
  auto response = Answer(batchSize, cvs_hat, counter);
  size_t size = response.size();
  out.write(reinterpret_cast<const char*>(&size), sizeof(size));
  for (const auto& mat : response) {
    out << mat;
  }
  return out;
}

std::vector<MatPoly> PIRServer::Answer(size_t batchSize, std::vector<MatPoly> &cvs_hat, uint16_t counter) {
  int bucketNum = ceil(batchSize * bucketRate_);
  // 获取布谷鸟哈希配置
  auto code = getCuckooCode(batchSize);
  // PBC编码,将每个桶映射到其包含的数据（指针）集合
  auto collections = PBC_Encode(&code, pDB_.get(), num_expansions, further_dims);
  // 获取神谕, god是一个二元组，第一个元素将每个桶映射到其包含的数据（索引）集合
  // 第二个元素是将每个桶映射到其内部参数<num_expansions, further_dims>
  auto god = getOracle(&code, num_expansions, further_dims);
  auto godOracle = std::get<0>(god);
  auto godSizes = std::get<1>(god);


  // // 获取调度计划， 将idx映射到桶。正常来说，schedule的值（vector）的长度为1
  // auto schedule = PBC_GetSchedule(&code, targetIdxes);
  // // 获取桶中内部索引，innerIdxes将桶映射到其内部索引，仅包含有效桶
  // std::unordered_map<size_t, size_t> innerIdxes;
  // for (const auto& entry : schedule) {
  //   size_t key = entry.first;
  //   assert(entry.second.size() == 1);
  //   size_t bucket = entry.second[0];

  //   // 在 oracle 中查找对应的索引
  //   auto& oracle_bucket = godOracle[bucket];
  //   auto it = std::find_if(oracle_bucket.begin(), oracle_bucket.end(), [key](const size_t& e) {
  //     return e == key;
  //     });

  //   assert(it != oracle_bucket.end());
  //   innerIdxes[bucket] = std::distance(oracle_bucket.begin(), it);
  // }


  // 将桶重建为数据库形式
  const size_t totalBucketNum = collections.size();
  std::vector<uint64_t*> DBs(totalBucketNum, nullptr);
  for (size_t i = 0; i < totalBucketNum; i++) {
#ifdef PIRDEBUG
    std::cout << "=================== Start to Rebuild the " << i << " Bucket ===================" << std::endl;
    // print_bucket(collections[i]);
#endif
    reBuildDB(DBs[i], godSizes[i].num_expansions, godSizes[i].further_dims, collections[i]);
#ifdef PIRDEBUG
    cout << "==============================================================" << endl;
    print_DB(DBs[i], godSizes[i].num_expansions, godSizes[i].further_dims);
    cout << "=================== Finish to Rebuild the " << i << " Bucket ===================" << endl;
#endif
  }


  // // realIdxes将查询索引映射到有效桶的索引
  // // innerIdxes_total将所有桶映射到其内部索引（注意上面的innerIdxes只包含有效桶）
  // std::vector<size_t> realIdxes;
  // std::vector<size_t> innerIdxes_total;
  // for (size_t bucket = 0; bucket < godOracle.size(); ++bucket) {
  //   if (innerIdxes.find(bucket) != innerIdxes.end()) {
  //     innerIdxes_total.push_back(static_cast<uint32_t>(innerIdxes[bucket]));
  //     realIdxes.push_back(bucket);
  //   } else {
  //     innerIdxes_total.push_back(static_cast<uint32_t>(0));
  //   }
  // }


  // 创建Server方的OKVS解码器
  GCTObvKVStore serverOKVS(batchSize, cuckooNum_, hyperGraphRate_, bucketNum);
  auto cts = serverOKVS.Decode(cvs_hat, counter);

  // 创建MultiPirServer类 处理密态查询
  MultiPirServer servers(godSizes, DBs, cts);
  std::vector<MatPoly> responses = servers.MultiServerAnswer();
  return responses;
}