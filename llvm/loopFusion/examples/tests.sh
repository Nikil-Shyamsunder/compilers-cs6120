# Compile with skeleton pass (loop fusion optimized)
clang -O1 -fpass-plugin=$(echo ../build/skeleton/SkeletonPass.*) \
      -isysroot $(xcrun --show-sdk-path) feedForward.c -o feedForward_optimized

# Compile without skeleton pass (baseline)
clang -O1 -isysroot $(xcrun --show-sdk-path) feedForward.c -o feedForward_baseline