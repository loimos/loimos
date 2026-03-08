#!/bin/bash
# Loimos Test Environment Setup for WSL
# Run this script from the loimos repository root

set -e

echo "=== Loimos Test Environment Setup ==="
echo ""

# Check if we're in the right directory
if [ ! -f "src/Makefile" ]; then
    echo "ERROR: Please run this script from the loimos repository root"
    exit 1
fi

# Set default paths
export CHARM_HOME=${CHARM_HOME:-~/charm}
export GTEST_HOME=${GTEST_HOME:-$(pwd)/src/googletest}
export PROTOBUF_HOME=${PROTOBUF_HOME:-/usr}

echo "Using:"
echo "  CHARM_HOME=$CHARM_HOME"
echo "  GTEST_HOME=$GTEST_HOME"
echo "  PROTOBUF_HOME=$PROTOBUF_HOME"
echo ""

# Verify Charm++
if [ ! -f "$CHARM_HOME/include/charm++.h" ]; then
    echo "ERROR: Charm++ not found at $CHARM_HOME"
    echo "Please install Charm++ or set CHARM_HOME environment variable"
    exit 1
fi
echo "Charm++ found"

# Verify GoogleTest
if [ ! -f "$GTEST_HOME/googletest/include/gtest/gtest.h" ]; then
    echo "ERROR: GoogleTest not found at $GTEST_HOME"
    exit 1
fi
if [ ! -f "$GTEST_HOME/build/lib/libgtest.a" ]; then
    echo "GoogleTest not built. Building now..."
    cd "$GTEST_HOME"
    mkdir -p build && cd build
    cmake .. -DCMAKE_CXX_STANDARD=11
    make -j$(nproc)
    cd - > /dev/null
fi
echo "GoogleTest found and built"

# Verify protoc
if ! command -v protoc &> /dev/null; then
    echo "ERROR: protoc not found. Install with: sudo apt install protobuf-compiler libprotobuf-dev"
    exit 1
fi
echo "protoc found: $(protoc --version)"

# Build protobufs if needed
if [ ! -f "src/protobuf/data.pb.h" ]; then
    echo "Building protobuf definitions..."
    cd src/protobuf
    make
    cd - > /dev/null
fi
echo "Protobuf definitions built"

# Generate Charm++ headers if needed
if [ ! -f "src/loimos.decl.h" ]; then
    echo "Generating Charm++ headers..."
    cd src
    $CHARM_HOME/bin/charmc loimos.ci
    cd - > /dev/null
fi
echo "Charm++ headers generated"

echo ""
echo "=== Setup Complete ==="
echo ""
echo "To build and run tests:"
echo "  cd src/tests"
echo "  export CHARM_HOME=$CHARM_HOME"
echo "  export GTEST_HOME=$GTEST_HOME"
echo "  export PROTOBUF_HOME=$PROTOBUF_HOME"
echo "  make all"
echo "  make test-all"
