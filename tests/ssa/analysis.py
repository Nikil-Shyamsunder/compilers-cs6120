import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Load CSV
df = pd.read_csv("res.csv")

# Filter out 'incorrect' entries
df = df[df["result"] != "incorrect"]

# Convert results to numeric
df["result"] = pd.to_numeric(df["result"])

# Pivot for easier comparison
pivot = df.pivot(index="benchmark", columns="run", values="result")

# Compute overheads
pivot["ssa_overhead"] = (pivot["ssa"] / pivot["baseline"] - 1) * 100
pivot["roundtrip_overhead"] = (pivot["roundtrip"] / pivot["baseline"] - 1) * 100

# Compute averages
ssa_avg = pivot["ssa_overhead"].mean()
roundtrip_avg = pivot["roundtrip_overhead"].mean()

print(f"Average SSA overhead: {ssa_avg:.2f}%")
print(f"Average roundtrip overhead: {roundtrip_avg:.2f}%")

# Plot raw instruction counts
plt.figure(figsize=(10,6))
x = np.arange(len(pivot.index))
width = 0.25

plt.bar(x - width, pivot["baseline"], width, label="Baseline")
plt.bar(x, pivot["ssa"], width, label="SSA")
plt.bar(x + width, pivot["roundtrip"], width, label="Roundtrip")

plt.yscale("log")
plt.xticks(x, pivot.index, rotation=45, ha="right")
plt.ylabel("Instruction count (log scale)")
plt.title("Benchmark Results (log scale)")
plt.legend()
plt.tight_layout()
plt.show()
