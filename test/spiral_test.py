import subprocess
import re
import time
import sys
from datetime import timedelta

# ===================== 配置区 =====================
DIM0 = 6
DIM1 = 4
DB_FILE = "database.txt"
OUTPUT_FILE = "output.txt"
ERROR_FILE = "error.txt"

QUERY_START = 4       # 每轮最少查询数量
QUERY_END = 32         # 每轮最大查询数量
STEP = 10000          # 查询步长
ROUNDS = 5            # 测试轮数
TIMEOUT = None        # 超时时间（秒），None 表示不设置超时
# ==================================================


def progress_bar(current, total, bar_length=30):
    """打印动态进度条"""
    percent = current / total
    filled = int(bar_length * percent)
    bar = "#" * filled + "-" * (bar_length - filled)
    sys.stdout.write(f"\r[{bar}] {percent*100:5.1f}%")
    sys.stdout.flush()


def run_tests():
    total_tests = (QUERY_END - QUERY_START + 1)
    total_passed = total_failed = total_timeout = total_exception = 0
    all_start_time = time.time()

    with open(OUTPUT_FILE, "w") as out_f, open(ERROR_FILE, "w") as err_f:
        for r in range(1, ROUNDS + 1):
            print(f"\n================= 🧪 Starting Round {r}/{ROUNDS} =================")
            round_passed = round_failed = 0
            round_start = time.time()

            for t in range(QUERY_START, QUERY_END + 1):
                queries = " ".join(str(i * STEP) for i in range(t))
                cmd = f"./spiral --dim0 {DIM0} --dim1 {DIM1} --database {DB_FILE} --output {OUTPUT_FILE} --query {queries}"

                test_start_time = time.time()
                try:
                    result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=TIMEOUT)
                    output = result.stdout
                    out_f.write(f"===== Round {r} | Test {t-QUERY_START+1}/{total_tests} =====\n{output}\n")

                    incorrect = re.findall(r"check (\d+)'s answer's correctness\s+Is correct\?: 0", output)
                    test_time = timedelta(seconds=time.time() - test_start_time)

                    if incorrect:
                        total_failed += 1
                        round_failed += 1
                        err_f.write(f"\n[Round {r} | Test {t-QUERY_START+1}] Command: {cmd}\n")
                        for i in incorrect:
                            err_f.write(f"check {i}'s answer's correctness\nIs correct?: 0\n")
                        print(f"\n❌  Round {r} | Test {t-QUERY_START+1}/{total_tests} — {len(incorrect)} errors ❌ ({test_time})")
                    else:
                        total_passed += 1
                        round_passed += 1
                        print(f"\n✅  Round {r} | Test {t-QUERY_START+1}/{total_tests} — {t} queries passed ({test_time})")

                except subprocess.TimeoutExpired:
                    total_timeout += 1
                    round_failed += 1
                    print(f"\n⏱️  Round {r} | Test {t-QUERY_START+1}/{total_tests} — timeout ({TIMEOUT}s)")
                    err_f.write(f"[Timeout] Round {r} | Test {t-QUERY_START+1}: {cmd}\n")

                except Exception as e:
                    total_exception += 1
                    round_failed += 1
                    print(f"\n⚠️  Round {r} | Test {t-QUERY_START+1}/{total_tests} — Exception: {e}")
                    err_f.write(f"[Exception] Round {r} | Test {t-QUERY_START+1}: {e}\n")

                progress_bar(t - QUERY_START + 1, total_tests)

            round_duration = time.time() - round_start
            round_rate = round_passed / (round_passed + round_failed) * 100 if (round_passed + round_failed) else 0
            print(f"\n================= ✅ Round {r} finished in {round_duration:.2f} s =================")
            print(f"Round {r} Summary: 通过 {round_passed}/{round_passed + round_failed} ({round_rate:.1f}%)\n")

    all_duration = time.time() - all_start_time
    total_executed = total_passed + total_failed + total_timeout + total_exception
    pass_rate = total_passed / total_executed * 100 if total_executed else 0

    print("\n================= 📊 测试总结 =================")
    print(f"总测试轮数: {ROUNDS}")
    print(f"总测试数:   {total_tests * ROUNDS}")
    print(f"通过数:     {total_passed}")
    print(f"错误数:     {total_failed}")
    print(f"超时数:     {total_timeout}")
    print(f"异常数:     {total_exception}")
    print(f"通过率:     {pass_rate:.1f}%")
    print(f"总用时:     {all_duration:.2f} 秒")
    print(f"输出文件:   {OUTPUT_FILE}")
    print(f"错误文件:   {ERROR_FILE}")


if __name__ == "__main__":
    run_tests()
