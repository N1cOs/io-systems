import os
import shutil
import itertools as it
import subprocess


def bench(src_dir, dst_dir, file, cnt):
    shutil.copy(file, src_dir)

    src = f"{src_dir}/{os.path.basename(file)}"
    dst = f"{dst_dir}/some_random_file_name"
    cmd = ["dd", f"if={src}", f"of={dst}"]

    results = []
    units = "MB/s"
    for _ in range(cnt):
        out = subprocess.run(cmd, stderr=subprocess.PIPE)
        stderr = out.stderr.decode()
        stats_line = stderr.strip().split("\n")[-1]
        speed, units = stats_line.split()[-2:]
        results.append(float(speed.replace(",", ".")))

    results = sorted(results)
    print(
        f"src={src_dir}, dst={dst_dir}, median speed={results[len(results) // 2]}, units={units}"
    )

    os.remove(src)
    os.remove(dst)


if __name__ == "__main__":
    partitions = os.environ["PARTITIONS"].split()
    bench_file = os.environ["BENCH_FILE_NAME"]
    bench_cnt = os.getenv("BENCHMARK_COUNT", 5)

    paths = [f"/mnt/{p}" for p in partitions]
    paths.append(os.environ["HOME"])  # directory which mounted on real SSD
    for p1, p2 in it.permutations(paths, 2):
        bench(p1, p2, bench_file, bench_cnt)
