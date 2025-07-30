#include "okvs.h"

std::vector<MatPoly> test_Batch_PIR_with_GCT_okvs(
    size_t num_expansions, 
    size_t further_dims,
    std::vector<size_t> IDX_TARGETs
) {
    do_MatPol_test();

    setup_constants();

    generate_gadgets();

    // cout << "Look at this !!!" << endl;

    uint64_t *B;

    // cout << random_data << endl;
    // cout << "target index is:" << IDX_TARGET << endl;

    // std::iota(IDX_TARGETs.begin(), IDX_TARGETs.end(), 1);

    genDataBase(B, num_expansions, further_dims, IDX_TARGETs);

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
    // cout << "============" << endl;

    // for (size_t i = 0 ; i < ind_vec.size() ; i++) {
    //     cout << ind_vec[i] << ' ';
    // }
    // cout << endl;

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
    std::vector<MatPoly> pts = client_A.MultiResponseDecrypt(rs);

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
            double log_var = check_relation(rs[i], modswitch_on_server, seq-1);
        }
    }
    return pts;
}

int main(int argc, char *argv[]) {
    size_t num_expansions = 2; // max = 7 //used to be 8
    size_t further_dims = 2; // v2
    size_t total_n = (1 << num_expansions) * (1 << further_dims);

    // size_t IDX_TARGET = 14;
    // size_t IDX_DIM0 = IDX_TARGET / (1 << further_dims);
    #ifndef __EMSCRIPTEN__

    build_table();

    // scratch = (uint64_t *)malloc(crt_count * poly_len * sizeof(uint64_t));

    ntt_qprime = new intel::hexl::NTT(2048, arb_qprime);

    bool ubench = false;
    bool high_rate = false;
    // cout << "argc is:" << argc << endl;
		std::vector<size_t> IDX_TARGETs;
		
    if (argc > 1) {
			num_expansions = strtol(argv[1], NULL, 10); // max = 7 //used to be 8
			further_dims = strtol(argv[2], NULL, 10);
			total_n = (1 << num_expansions) * (1 << further_dims);
			// IDX_TARGET = strtol(argv[3], NULL, 10);
			IDX_TARGETs.resize(argc-3);
			
			size_t idx = 3;
			for (auto &target: IDX_TARGETs) {
				target = std::stol(argv[idx++]);
			}
    } else {
			num_expansions = 6;
			further_dims = 4;
			total_n = (1 << num_expansions) * (1 << further_dims);
			IDX_TARGETs.resize(8);
			std::iota(IDX_TARGETs.begin(), IDX_TARGETs.end(), 1);
		}
    // 查询接口在这里, 数据库是 1024 个条目, 每个条目 8 KB
    // 8 KB 有亿点点大, 可以只在前端中展示一部分
    // 查询结果在这里
    auto res = test_Batch_PIR_with_GCT_okvs(num_expansions, further_dims, IDX_TARGETs);
    // uint64_t *B;
    // genDataBase(B, num_expansions, further_dims, IDX_TARGETs);
    // for (size_t i = 0; i < 10; i++) {
    //     cout << B[i] << endl;
    // }
    for(size_t i = 0; i < res.size(); i++) {
        output_pt(res[i]);
    }
    #endif
}