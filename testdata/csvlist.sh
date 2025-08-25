#!/bin/bash

find . -path "./ral_colors" -prune -o -type f \( -name "*.txt" -o -name "*.csv" \) -exec echo "=== {} ===" \; -exec cat {} \;
#find . -type f \( -name "*.txt" -o -name "*.csv" \) ! -name "materials.csv" ! -name "stock.csv" -exec echo "=== {} ===" \; -exec cat {} \;
#find . -type f \( -name "*.txt" -o -name "*.csv" \) -exec echo "=== {} ===" \; -exec cat {} \;

#find . -path "./testdata/ral_colors" -prune -o -type f \( -name "*.txt" -o -name "*.csv" \) ! -name "materials.csv" ! -name "stock.csv" -exec echo "=== {} ===" \; -exec cat {} \;
