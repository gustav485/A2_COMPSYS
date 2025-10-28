#!/bin/bash
set -e

# Program and test directories
PROG="../fauxgrep-mt"
TEST_DIR="fauxgrepTestFiles/"
mkdir -p "$TEST_DIR"

# Compile the program
echo "Compiling the project..."
(cd .. && make)

# Creating small test files
echo "Creating small test files..."
echo -e "hello world\nDIKU\nneedle here" > "$TEST_DIR/file1.txt"
echo -e "another line\nneedle appears again\nnothing here" > "$TEST_DIR/file2.txt"
echo -e "just some text\nmore text" > "$TEST_DIR/file3.txt"

# Expected output
EXPECTED_OUTPUT="$TEST_DIR/expected.txt"
echo -e "$TEST_DIR/file1.txt:3: needle here\n$TEST_DIR/file2.txt:2: needle appears again" > "$EXPECTED_OUTPUT"


# Large test files for testing performance
echo "Creating large test file for performance tests..."
for i in {1..3}; do
    FILE="$TEST_DIR/large_file$i.txt"
    for j in {1..1000000}; do
        if (( j % 2500 == 0 )); then
            echo "needle at line $j" >> "$FILE"
        else
            echo "some random text line $j" >> "$FILE"
        fi
    done
done

# Function to run test with threads
run_test() {
    local threads=$1
    echo -e "\nRunning test with $threads thread(s)..."
    start=$(date +%s.%N)
    "$PROG" -n $threads "needle" "$TEST_DIR"/large_file*.txt > /dev/null
    end=$(date +%s.%N)
    elapsed=$(echo "$end - $start" | bc)
    echo "Threads: $threads — Time: ${elapsed}s"
}


#Run small tests
echo -e "\nRunning tests..."
OUTPUT="$TEST_DIR/output.txt"
"$PROG" "needle" "$TEST_DIR/file1.txt" "$TEST_DIR/file2.txt" "$TEST_DIR/file3.txt" > "$OUTPUT"
rm -f "$TEST_DIR"/large_file*.txt
if diff -q "$OUTPUT" "$EXPECTED_OUTPUT" > /dev/null; then
    echo "TESTS PASSED"
else
    echo "TEST FAILED"
    diff "$OUTPUT" "$EXPECTED_OUTPUT"
fi

# Performance tests
for t in 1 2 4 8; do
    run_test $t
done


# Cleanup
echo -e "\nCleaning up test files..."
rm -rf "$TEST_DIR"

echo "Testing complete!"
