# save as generate_output_fast.py
total_lines = 8192 * 1024
special = {255, 255, 255, 255, 255}

with open("database.txt", "w") as f:
    for i in range(total_lines):
        if i in special:
            f.write(f"{i}\n")
        else:
            f.write("100\n")
