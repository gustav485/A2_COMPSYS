#!/bin/bash
set -e

# Program and test directories
PROG="../fhistogram-mt"
TEST_DIR="Histogram_Tests/"
mkdir -p "$TEST_DIR"

# Compile the program
echo "Compiling the project..."
(cd .. && make)

# Create small test files
echo "Creating small test files..."
echo "abcabcabc" > "$TEST_DIR/file1.txt"
echo "helloHELLO" > "$TEST_DIR/file2.txt"
echo "1234567890" > "$TEST_DIR/file3.txt"

# Create large test files for performance testing
echo "Creating large test files..."
for i in {1..3}; do
    FILE="$TEST_DIR/large_file$i.txt"
    base_text="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
    for j in {1..50000}; do
        echo "$base_text" >> "$FILE"
    done
done

# Function to run histogram tests
run_test() {
    local threads=$1
    echo -e "\nRunning histogram test with $threads thread(s)..."
    start=$(date +%s.%N)
    "$PROG" -n $threads "$TEST_DIR" > "$TEST_DIR/output_${threads}.txt"
    end=$(date +%s.%N)
    elapsed=$(echo "$end - $start" | bc)
    echo "Threads: $threads — Time: ${elapsed}s"
}

# Run functional test on small files
echo -e "\nRunning functional correctness test..."
"$PROG" "$TEST_DIR/file1.txt" "$TEST_DIR/file2.txt" "$TEST_DIR/file3.txt" > "$TEST_DIR/small_output.txt"

if [ -s "$TEST_DIR/small_output.txt" ]; then
    echo "Functional test PASSED (output generated)"
else
    echo "Functional test FAILED (no output)"
fi

# Run performance tests
for t in 1 2 4 8; do
    run_test $t
done

# Cleanup
echo -e "\nCleaning up test files..."
rm -rf "$TEST_DIR"

echo "Histogram test script complete!"
