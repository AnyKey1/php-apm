#!/bin/bash
# Build standalone apm.so extension

set -e

echo "🔨 Building standalone apm.so for PHP 5.6..."
echo ""

# Create a temporary Dockerfile for building
cat > Dockerfile.build << 'EOF'
FROM php:5.6-cli

# Fix Debian repositories
RUN echo "deb http://archive.debian.org/debian stretch main" > /etc/apt/sources.list

# Install build dependencies
RUN apt-get update && apt-get install -y \
    git \
    autoconf \
    build-essential \
    libcurl4-openssl-dev \
    --allow-unauthenticated \
    && rm -rf /var/lib/apt/lists/*

# Copy source code
COPY . /tmp/php-apm
WORKDIR /tmp/php-apm

# Build extension
RUN phpize \
    && ./configure \
        --enable-apm \
        --without-sqlite3 \
        --without-mysql \
        --disable-statsd \
        --disable-socket \
        --enable-elasticsearch \
    && make \
    && make install

# Copy the built extension to a known location
RUN mkdir -p /output && \
    cp $(php-config --extension-dir)/apm.so /output/apm.so

CMD ["cat", "/output/apm.so"]
EOF

# Build the image
echo "📦 Building Docker image..."
docker build -f Dockerfile.build -t apm-builder . > /dev/null 2>&1

# Create a container and copy the .so file
echo "📄 Extracting apm.so..."
CONTAINER_ID=$(docker create apm-builder)
docker cp $CONTAINER_ID:/output/apm.so ./apm.so
docker rm $CONTAINER_ID > /dev/null 2>&1

# Clean up
rm Dockerfile.build
docker rmi apm-builder > /dev/null 2>&1

# Get file info
FILE_SIZE=$(ls -lh apm.so | awk '{print $5}')
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ Build completed successfully!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📦 Extension file: ./apm.so"
echo "📏 Size: $FILE_SIZE"
echo ""
echo "🔧 To install:"
echo "   1. Copy to PHP extension directory:"
echo "      cp apm.so \$(php-config --extension-dir)/"
echo ""
echo "   2. Add to php.ini:"
echo "      echo 'extension=apm.so' >> \$(php --ini | grep 'Loaded Configuration' | awk '{print \$4}')"
echo ""
echo "   3. Configure APM (see apm.ini for examples)"
echo ""
echo "📋 Verify installation:"
echo "   php -m | grep apm"
echo ""
