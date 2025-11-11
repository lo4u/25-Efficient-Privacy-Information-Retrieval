import subprocess

spiral_exec = "./spiral"
dim0 = 6
dim1 = 4
database = "database.txt"
output = "result.txt"

# 生成 128 个查询点：0, 10000, 20000, ..., 1270000
query = [i * 10000 for i in range(128)]

cmd = [
    spiral_exec,
    "--dim0", str(dim0),
    "--dim1", str(dim1),
    "--database", database,
    "--output", output,
    "--query", *map(str, query)
]

print(f"Running query of {len(query)} elements: {query[0]} ... {query[-1]}")
subprocess.run(cmd, check=True)
print("✅ 查询完成！结果已写入 result.txt")
