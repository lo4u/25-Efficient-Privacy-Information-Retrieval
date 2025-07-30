#include "okvs.h"

void test_GCT_okvs(
    size_t num_expansions, 
    size_t further_dims
) {
    do_MatPol_test();

    setup_constants();

    generate_gadgets();

    cout << "Look at this !!!" << endl;

    uint64_t *B;

    cout << random_data << endl;
    cout << "target index is:" << IDX_TARGET << endl;

    std::vector<size_t> IDX_TARGETs(16);
    std::iota(IDX_TARGETs.begin(), IDX_TARGETs.end(), 1);

    genDataBase(B, num_expansions, further_dims, IDX_TARGETs);

    // print_DB(B, num_expansions, further_dims);

    // 下面先进行一些准备工作
    // Both: 新建布谷鸟编码
    CuckooCode code(IDX_TARGETs.size(), 3, 1.5);
    // Server: 对数据库进行 PBC 编码， collection为存储具体数据的vector
    auto collections = PBC_Encode(&code, B, num_expansions, further_dims);
    // Client: 获取神谕
    OracleTy god = getOracle(&code, num_expansions, further_dims);
		// godOracle是一个二维数组，第一维对应每个桶，第二维对应桶内元素。它是来自Simple hash的结果
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
				// 这里记录偏移量。因为每一个桶只对应一个查询，因此每个桶记录一个内部索引即可。只记录了有效桶
        indexes[bucket] = std::distance(oracle_bucket.begin(), it);
    }
    // Server: 重建 1.5k 个桶, 变为可用于 Spiral 查询的形式
    std::vector<uint64_t *> DBs(collections.size());
    size_t cLen = collections.size();
    for (size_t i = 0 ; i < cLen ; i++) {
        cout << "=================== Start to Rebuild the " << i << " Bucket ===================" << endl;
        DBs[i] = NULL;
        // print_bucket(collections[i]);
				// 这里collections[i]对应具体数据
        reBuildDB(DBs[i], godSizes[i].num_expansions, godSizes[i].further_dims, collections[i]);
        // cout << "==============================================================" << endl;
        // print_DB(DBs[i], godSizes[i].num_expansions, godSizes[i].further_dims);
        cout << "=================== Finish to Rebuild the " << i << " Bucket ===================" << endl;
    }

    // Client: 重构多请求向量
		// real_index记录了有效的桶的索引
		// ind_vec记录每个桶内部的查询索引
    std::vector<size_t> real_ind;
    std::vector<size_t> ind_vec;
    for (size_t bucket = 0 ; bucket < godOracle.size() ; ++bucket) {
        if (indexes.find(bucket) != indexes.end()) {
            ind_vec.push_back(static_cast<uint32_t>(indexes[bucket]));
            real_ind.push_back(bucket);
        } else {
						// 混淆用的桶
            ind_vec.push_back(static_cast<uint32_t>(0)) ;
        }
    }

    for (size_t i = 0 ; i < ind_vec.size() ; i++) {
        cout << ind_vec[i] << ' ';
    }
    cout << endl;

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

    cout << "==============================" << endl;

		// 加密
    auto cvs_hat = client_A.MultiPackedIndexEncrypt(pt_hat); // In NTT

    GCTObvKVStore server_OKVS(real_ind.size(), 3, 1.25, packed_pt_vec.size());
		// 不经意解压缩
    auto cvs = server_OKVS.Decode(cvs_hat);
    cout << "==============================" << endl;
    MultiPirServer server_A(godSizes, DBs, cvs);
    cout << cvs.size() << endl;
    cout << server_A.handles.size() << endl;
    std::vector<MatPoly> rs = server_A.MultiServerAnswer();
    // cout << "==============================" << endl;
    std::vector<MatPoly> pts = client_A.MultiAnswerDecrypt(rs);

		// 下面按照桶的顺序去验证查询正确与否
    for (size_t i = 0 ; i < DBs.size() ; i++) {
        if (indexes.find(i) != indexes.end()) {
						// 如果是有效桶
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

    // test_trival_multi_PIR(B, num_expansions, further_dims, IDX_TARGETs);
}

void test_speed_of_3H_GCT_and_RB_Matrix(
    size_t num_expansions,
    size_t further_dims,
    size_t IDX_TARGET
) {
    do_MatPol_test();

    setup_constants();

    generate_gadgets();

    cout << "Look at this !!!" << endl;

    uint64_t *B;

    cout << random_data << endl;
    cout << "the number of target index is:" << IDX_TARGET << endl;

    std::vector<size_t> IDX_TARGETs(IDX_TARGET);
    std::iota(IDX_TARGETs.begin(), IDX_TARGETs.end(), 1);

    // 下面先进行一些准备工作
    // Both: 新建布谷鸟编码
    CuckooCode code(IDX_TARGETs.size(), 3, 1.5);

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

    for (size_t i = 0 ; i < ind_vec.size() ; i++) {
        cout << ind_vec[i] << ' ';
    }
    cout << endl;

    // Client: 创建 Client 类
    MultiPirClient client_A(godSizes, ind_vec, godOracle, indexes);
    // 创建密钥
    client_A.setup();
    // Client: 生成 packed_poly 向量
    start_timing();
    std::vector<MatPoly> packed_pt_vec = client_A.MultiPackedIndexGen();
    cout << "Time used to do pack is " << end_timing() << endl;

    // Create OKVS class
    GCTObvKVStore client_OKVS(real_ind.size(), 3, 1.3, packed_pt_vec.size());
    cout << "================== Benchmark ==================" << endl;

    // test: the speed of OKVS
    start_timing();
    auto pt_hat = client_OKVS.Encode(packed_pt_vec, real_ind);
    double time_3H_GCT_Compress = end_timing();
    cout << "Time used to do OCD.Compress with 3H-GCT is " << time_3H_GCT_Compress << endl;

    start_timing();
    auto cvs_hat = client_A.MultiPackedIndexEncrypt(pt_hat); // In NTT
    double time_encrypt = end_timing();
    cout << "ALL Time used to do OCD.Compress with 3H-GCT is " << time_3H_GCT_Compress + time_encrypt << endl;
    
    GCTObvKVStore server_OKVS(real_ind.size(), 3, 1.3, packed_pt_vec.size());
    start_timing();
    auto cvs = server_OKVS.Decode(cvs_hat);
    double time_3H_GCT_Decompress = end_timing();
    cout << "Time used to do OCD.Decompress with 3H-GCT is " << time_3H_GCT_Decompress << endl;
    cout << "Total time used to do OCD with 3H-GCT is " << time_3H_GCT_Decompress + time_3H_GCT_Compress + time_encrypt << endl;

    // // Create OCD class
    // size_t w = 8 + ceil(log2(real_ind.size()));
    // LSObvDecompress client_OCD(packed_pt_vec.size(), real_ind.size(), 0.05, w);

    // // test: the speed of ocd
    // start_timing();
    // auto p_hat = client_OCD.Compress(packed_pt_vec, real_ind);
    // double time_RB_Matrix = end_timing();
    // cout << "Time used to do OCD with random bond matrix is " << time_RB_Matrix << endl;

    // start_timing();
    // auto cvs_hat = client_A.MultiPackedIndexEncrypt(pt_hat);
    // cout << "Time used to encrypt is " << end_timing() << endl;
}

void test_for_noice_of_encrypt(
    size_t num_expansions,
    size_t further_dims,
    size_t IDX_TARGET
) {
    do_MatPol_test();

    setup_constants();

    generate_gadgets();

    cout << "Look at this !!!" << endl;

    uint64_t *B;

    cout << random_data << endl;
    cout << "the number of target index is:" << IDX_TARGET << endl;

    std::vector<size_t> IDX_TARGETs(IDX_TARGET);
    std::iota(IDX_TARGETs.begin(), IDX_TARGETs.end(), 1);

    // 下面先进行一些准备工作
    // Both: 新建布谷鸟编码
    CuckooCode code(IDX_TARGETs.size(), 3, 1.5);

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

    for (size_t i = 0 ; i < ind_vec.size() ; i++) {
        cout << ind_vec[i] << ' ';
    }
    cout << endl;

    // Client: 创建 Client 类
    MultiPirClient client_A(godSizes, ind_vec, godOracle, indexes);
    // 创建密钥
    client_A.setup();
}
