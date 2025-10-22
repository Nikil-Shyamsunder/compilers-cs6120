import numpy as np
import matplotlib.pyplot as plt

# Load data
res = np.loadtxt("res.out")
res_normal = np.loadtxt("res_normal.out")

assert len(res) == len(res_normal), f"Length mismatch: {len(res)} vs {len(res_normal)}"
x = np.arange(len(res))

# Clean up crazy values so matplotlib can render
def sanitize(arr, clip=1e3):
    arr = np.nan_to_num(arr, nan=0.0, posinf=clip, neginf=-clip)
    return np.clip(arr, -clip, clip)

res_clean = sanitize(res)
res_normal_clean = sanitize(res_normal)

# Create two stacked subplots
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(9, 6), sharex=True)

# Normal
ax1.plot(x, res_normal_clean, lw=1, color="tab:blue")
ax1.set_title("Normal π Approximation (Leibniz Series)")
ax1.set_ylabel("Partial Sum")
ax1.grid(True, alpha=0.3)

# Swapped
ax2.plot(x, res_clean, lw=1, color="tab:red")
ax2.set_title("After Subtraction-Operand Swap Pass")
ax2.set_xlabel("Iteration")
ax2.set_ylabel("Partial Sum (clipped)")
ax2.grid(True, alpha=0.3)

plt.tight_layout()
plt.show()
