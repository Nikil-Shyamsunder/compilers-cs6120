import sys
import math
import pandas as pd
import matplotlib.pyplot as plt

def main():
    df = pd.read_csv(sys.stdin)
    required = {"benchmark", "run", "result"}
    if not required.issubset(df.columns):
        raise SystemExit(f"CSV must contain columns: {required}")

    # Pivot to get baseline and opt columns
    pivot = df.pivot(index="benchmark", columns="run", values="result")

    # Keep only rows with both baseline and opt present and baseline > 0
    valid = pivot.dropna(subset=["baseline", "opt"]).copy()
    valid = valid[valid["baseline"] > 0]

    # Compute percent reduction
    valid["reduction_pct"] = (valid["baseline"] - valid["opt"]) / valid["baseline"] * 100.0

    # Print full per-benchmark table
    print("Per-benchmark results:")
    print(valid[["baseline", "opt", "reduction_pct"]].to_csv(index=True))

    finite = valid["reduction_pct"].replace([math.inf, -math.inf], pd.NA).dropna()
    if finite.empty:
        print("No valid rows to compute statistics.")
        return

    mean_red = finite.mean()
    median_red = finite.median()
    print(f"Mean reduction (%): {mean_red:.3f}")
    print(f"Median reduction (%): {median_red:.3f}")

    reduced = valid[valid["reduction_pct"] > 0.0].sort_values("reduction_pct", ascending=False)
    if not reduced.empty:
        print("\nBenchmarks with reduction (opt < baseline):")
        for b, row in reduced.iterrows():
            print(f"- {b}: {row['reduction_pct']:.3f}%")
    else:
        print("\nNo benchmarks showed a reduction.")

    top5 = valid.sort_values("reduction_pct", ascending=False).head(5)
    print("\nTop 5 benchmarks by % reduction:")
    for b, row in top5.iterrows():
        print(f"- {b}: {row['reduction_pct']:.3f}%")

    # Plot histogram of percentage reduction
    plt.hist(finite, bins=10, edgecolor="black")
    plt.xlabel("Reduction percentage")
    plt.ylabel("Count")
    plt.title("Distribution of % reduction across benchmarks")
    plt.tight_layout()
    plt.savefig("reduction_histogram.png", dpi=200)
    print("\nHistogram saved as reduction_histogram.png")

if __name__ == "__main__":
    main()
