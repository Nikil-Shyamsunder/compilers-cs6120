clang -O1 -fpass-plugin=$(echo ../build/skeleton/SkeletonPass.*) \
      -isysroot $(xcrun --show-sdk-path) feedForward.c