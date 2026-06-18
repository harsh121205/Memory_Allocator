CXX = clang++
CXXFLAGS = -std=c++17 -O3 -Wall
INCLUDES = -I./include -isystem /opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lbenchmark -lpthread

# Executables
all: bench_matrix bench_stress allocator_compare

bench_matrix: benchmarks/bench_matrix.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

bench_stress: benchmarks/bench_stress.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

allocator_compare: benchmarks/allocator_compare.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

clean:
	rm -f bench_matrix bench_stress allocator_compare