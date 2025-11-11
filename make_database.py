# save as generate_output_fast.py
# import random
total_lines = 8192 * 1024
total_segment = 1024
# 0,10000, 20000, ... 为254，其它为100

# with open("database.txt", "w") as f:
#   index = 0
#   while (1):
#     if index >= total_lines:
#       break
#     if index % 10000 == 0:
#       f.write("254\n")
#     else:
#       f.write("100\n")
#     index += 1

with open("database.txt", "w") as f:
  index = 0
  while (index < total_lines):
    f.write(f"{index % 256}\n")
    index += 1