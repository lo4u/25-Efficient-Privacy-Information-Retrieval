#include "pir_io.h"

std::vector<uint64_t> loadFile(const std::string filename, const size_t ntotal, const int entrySize) {
  std::ifstream fin(filename);
  if (!fin.is_open()) {
    std::cout << "Failed to load file" << std::endl;
  }
  if (entrySize > 1) {
    std::cout << "FATAL: entrySize is too big..." << std::endl;
    exit(1);
  }

  // 导入
  size_t index = 0;
  std::vector<uint64_t> rawDB(ntotal);
  while (index < ntotal && fin >> rawDB[index])
    ++index;

  uint64_t temp;
  if (index < ntotal) {       // 检测是否有足够数据，不够补1
    for (;index < ntotal; ++index)
      rawDB[index] = 1;
  } else if ((fin >> temp)) { // 检测是否读入全部数据，若还有多，输出警告消息
    std::cout << "WARNING: elements are too many to be loaded entirely." << std::endl;
  }

  return rawDB;
}

std::vector<uint64_t> processRawDB(const std::vector<uint64_t> rawDB, size_t num_expansions, size_t further_dims) {
  //todo
  return std::vector<uint64_t>{};
}

void genDataBaseFromRawDB(
  uint64_t* &DB,
  const size_t num_expansions,
  const size_t further_dims,
  const std::vector<uint64_t>& rawDB,
  const int entrySize) {
  size_t total_n = (1 << num_expansions) * (1 << further_dims); // 总数据量
  size_t dim0 = 1 << num_expansions; // 第一维度的总数
  size_t num_per = 1 << further_dims; // 数据库总数据量 / 第一维度

  size_t num_bytes_B = sizeof(uint64_t) * dim0 * num_per * n0 * n2 * poly_len; // 2 * poly_len;
  // cout << "num_bytes_B: " << num_bytes_B << endl;
  DB = (uint64_t*)aligned_alloc(64, num_bytes_B);
  memset(DB, 0, num_bytes_B);

  uint64_t* BB = (uint64_t*)malloc(n0 * n2 * crt_count * poly_len * sizeof(uint64_t));
  size_t numBytesPlaintextRaw = n0 * n0 * num_bits_q * poly_len / 8;
  uint64_t* pt_raw = (uint64_t*)malloc(numBytesPlaintextRaw);
  memset(pt_raw, 0, numBytesPlaintextRaw);
  uint64_t* pt_buf = (uint64_t*)malloc(n0 * n0 * crt_count * poly_len * sizeof(uint64_t));
  memset(pt_buf, 0, n0 * n0 * crt_count * poly_len * sizeof(uint64_t));

  // cout << "starting generation of db" << endl;
  MatPoly H_nttd = to_ntt(H_mp);
  uint64_t* H_encd = H_nttd.data;
  MatPoly pt_tmp(n0, n0, false);
  MatPoly pt_encd_raw(n0, n2, false);
  pts_encd_test = MatPoly(n0, n2);
  pt_test = MatPoly(n0, n0);
  size_t counter = 0;
  for (size_t i = 0; i < total_n; i++) {
    for (size_t k = 0; k < n0 * n0 * poly_len; ++k) {
      pt_tmp.data[k] = rawDB[counter];
#ifdef PIRDEBUG
      // 检查是否会超出
      if (pt_tmp.data[k] >= p_db) {
        std::cout << "FATAL: loaded data exceeds p_db." << std::endl;
        exit(1);
      }
#endif
      ++counter;
    }
    MatPoly pt_real(n0, n0, false);
    cop(pt_real, pt_tmp);
    pt_reals.push_back(pt_real);

    pt_encd_raw = pt_tmp;
    for (size_t pol = 0; pol < n0 * n2 * poly_len; pol++) {
      int64_t val = (int64_t)pt_encd_raw.data[pol];
      assert(val >= 0 && val < p_db);
      if (val >= (p_db / 2)) {
        val = val - (int64_t)p_db;
      }
      if (val < 0) {
        val += Q_i;
      }
      assert(val >= 0 && val < Q_i);
      pt_encd_raw.data[pol] = val;
    }
    to_ntt(pts_encd_test, pt_encd_raw);
    // print_pt_crt(pts_encd_test);


    // b': i c n z j m
    size_t ii = i % num_per;
    size_t j = i / num_per;
    for (size_t m = 0; m < n0; m++) {
      for (size_t c = 0; c < n2; c++) {
        memcpy(BB, &pts_encd_test.data[(m * n2 + c) * crt_count * coeff_count], crt_count * coeff_count * sizeof(uint64_t));
        for (size_t z = 0; z < poly_len; z++) {
          size_t idx = z * (num_per * n2 * dim0 * n0) +
            ii * (n2 * dim0 * n0) +
            c * (dim0 * n0) +
            j * (n0)+m;
          DB[idx] = BB[z] | (BB[poly_len + z] << 32);
        }
      }
    }
  }
  free(BB);
}

