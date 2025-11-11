#include "okvs.h"
#include "pir_io.h"

std::vector<MatPoly> Batch_PIR_with_GCT_okvs(
    size_t num_expansions, 
    size_t further_dims,
    std::vector<size_t> IDX_TARGETs,
    std::string database=""
) {
    setup_constants();
    generate_gadgets();

    // 数据库
    uint64_t *B;
    if (database == "") {
      genDataBase(B, num_expansions, further_dims, IDX_TARGETs);      
    } else {
      auto rawDB = loadFile(database, total_n * n0 * n0 * poly_len);
      genDataBaseFromRawDB(B, num_expansions, further_dims, rawDB);
    }

    // print_DB(B, num_expansions, further_dims);

    // 下面先进行一些准备工作
    // Both: 新建布谷鸟编码
    CuckooCode code(IDX_TARGETs.size(), 3, 1.5);
    // Server: 对数据库进行 PBC 编码
    auto collections = PBC_Encode(&code, B, num_expansions, further_dims);
    // Client: 获取神谕
    OracleTy god = getOracle(&code, num_expansions, further_dims);
    auto godOracle = std::get<0>(god);
    auto godSizes = std::get<1>(god);
    // Client: 得到调度计划
    auto schedule = PBC_GetSchedule(&code, IDX_TARGETs);
    // Client: 创建一个新的哈希表，将 schedule 中的每个键映射到一个索引值
    std::unordered_map<size_t, size_t> indexes;
    for (const auto& entry : schedule) {
        size_t key = entry.first;
        assert(entry.second.size() == 1);
        size_t bucket = entry.second[0];

        // 在 oracle 中查找对应的索引
        auto& oracle_bucket = godOracle[bucket];
        auto it = std::find_if(oracle_bucket.begin(), oracle_bucket.end(), [key](const size_t& e) {
            return e == key;
        });

        assert(it != oracle_bucket.end());
        indexes[bucket] = std::distance(oracle_bucket.begin(), it);
    }
    // Server: 重建 1.5k 个桶, 变为可用于 Spiral 查询的形式
    std::vector<uint64_t *> DBs(collections.size());
    size_t cLen = collections.size();
    for (size_t i = 0 ; i < cLen ; i++) {
        // cout << "=================== Start to Rebuild the " << i << " Bucket ===================" << endl;
        DBs[i] = NULL;
        // print_bucket(collections[i]);
        reBuildDB(DBs[i], godSizes[i].num_expansions, godSizes[i].further_dims, collections[i]);
        // cout << "==============================================================" << endl;
        // print_DB(DBs[i], godSizes[i].num_expansions, godSizes[i].further_dims);
        // cout << "=================== Finish to Rebuild the " << i << " Bucket ===================" << endl;
    }

    // Client: 重构多请求向量
    std::vector<size_t> real_ind;
    std::vector<size_t> ind_vec;
    for (size_t bucket = 0 ; bucket < godOracle.size() ; ++bucket) {
        if (indexes.find(bucket) != indexes.end()) {
            ind_vec.push_back(static_cast<uint32_t>(indexes[bucket]));
            real_ind.push_back(bucket);
        } else {
            ind_vec.push_back(static_cast<uint32_t>(0));
        }
    }


    // Client: 创建 Client 类
    MultiPirClient client_A(godSizes, ind_vec, godOracle, indexes);
    // 创建密钥
    client_A.setup();
    // Client: 生成 packed_poly 向量
    std::vector<MatPoly> packed_pt_vec = client_A.MultiPackedIndexGen();

    // Create OKVS class
    GCTObvKVStore client_OKVS(real_ind.size(), 3, 1.3, packed_pt_vec.size());

    // test: the correctness of OKVS
    auto pt_hat = client_OKVS.Encode(packed_pt_vec, real_ind);
    // cout << "============" << endl;

    auto cvs_hat = client_A.MultiPackedIndexEncrypt(pt_hat); // In NTT

    GCTObvKVStore server_OKVS(real_ind.size(), 3, 1.3, packed_pt_vec.size());
    auto cvs = server_OKVS.Decode(cvs_hat);
    MultiPirServer server_A(godSizes, DBs, cvs);
    // cout << cvs.size() << endl;
    // cout << server_A.handles.size() << endl;
    std::vector<MatPoly> rs = server_A.MultiServerAnswer();
    std::vector<MatPoly> pts = client_A.MultiAnswerDecrypt(rs);

    for (size_t i = 0 ; i < DBs.size() ; i++) {
        if (indexes.find(i) != indexes.end()) {
            size_t seq = 0;
            for (const auto& entry : schedule) {
                size_t key = entry.first;
                assert(entry.second.size() == 1);
                size_t bucket = entry.second[0];
                if (bucket == i) {
                    seq = key;
                    break;
                }
            }
            double log_var = check_relation(rs[i], modswitch_on_server, seq);
        }
    }

    std::vector<MatPoly> result;
    result.reserve(IDX_TARGETs.size());
    for (size_t i = 0; i < IDX_TARGETs.size(); ++i) {
      size_t target = IDX_TARGETs[i];
      size_t targetBucket = schedule[target][0];
      result.push_back(pts[targetBucket]);
    }
    return result;
}

int main(int argc, char *argv[]) {
    #ifndef __EMSCRIPTEN__

    // 生成常数、NTT表等参数
    build_table();
    // scratch = (uint64_t *)malloc(crt_count * poly_len * sizeof(uint64_t));
    ntt_qprime = new intel::hexl::NTT(2048, arb_qprime);
    bool ubench = false;
    bool high_rate = false;
    std::vector<size_t> originalIdx;
		std::vector<size_t> IDX_TARGETs;
    ArgParser argParser;
		
    if (argc == 1) { // 无输入，运行测试
			num_expansions = 6;
			further_dims = 4;
			total_n = (1 << num_expansions) * (1 << further_dims);
			IDX_TARGETs.resize(32);
			std::iota(IDX_TARGETs.begin(), IDX_TARGETs.end(), 0);
      std::vector<MatPoly> result = Batch_PIR_with_GCT_okvs(num_expansions, further_dims, IDX_TARGETs);
      return 0;
    }
    // 读取命令行参数
    argParser.parse(argc, argv);
    num_expansions = std::stoull(argParser.get("dim0")); // max = 7 //used to be 8
    further_dims = std::stoull(argParser.get("dim1"));
    total_n = (1 << num_expansions) * (1 << further_dims);
    for (auto& s : argParser.getList("query")) {
      originalIdx.push_back(std::stoull(s));
      size_t matIndex = getIdxAndOffline(std::stoull(s)).first;
      IDX_TARGETs.push_back(matIndex);
    }
    // 查询
#ifdef DEBUG
    std::cout << "查询的矩阵索引: " << std::endl;
    for (auto i: IDX_TARGETs)
      std::cout << i << " ";
    std::cout << std::endl;
#endif
    std::vector<MatPoly> result = Batch_PIR_with_GCT_okvs(num_expansions, further_dims, IDX_TARGETs, argParser.get("database"));

    std::ofstream fout(argParser.get("output"));
    for (size_t i = 0; i < originalIdx.size(); ++i) {
      size_t offline = getIdxAndOffline(originalIdx[i]).second;
      std::cout << "find the " << i << " th entry: " << result[i].data[offline] << std::endl;
      fout << result[i].data[offline] << std::endl;
    }

    return 0;
#endif
}