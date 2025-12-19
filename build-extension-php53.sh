#!/bin/bash
# Build apm.so for different PHP versions and old GLIBC

set -e

PHP_VERSION=${1:-"5.3"}

echo "🔨 Building apm.so for PHP $PHP_VERSION (compatible with old GLIBC)..."
echo ""

# Create a temporary Dockerfile for building with old base
cat > Dockerfile.build-legacy << EOF
# Use old Debian for GLIBC compatibility
FROM debian:wheezy

# Configure old repositories
RUN echo "deb http://archive.debian.org/debian wheezy main" > /etc/apt/sources.list && \
    echo "deb http://archive.debian.org/debian-security wheezy/updates main" >> /etc/apt/sources.list

# Install PHP and build dependencies
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    php5-dev \
    php5-cli \
    git \
    autoconf \
    build-essential \
    libcurl4-openssl-dev \
    curl \
    --force-yes \
    && rm -rf /var/lib/apt/lists/*

# Copy source code
COPY . /tmp/php-apm
WORKDIR /tmp/php-apm

# Build extension
RUN phpize && \
    ./configure \
        --enable-apm \
        --without-sqlite3 \
        --without-mysql \
        --disable-statsd \
        --disable-socket \
        --enable-elasticsearch && \
        --with-debugfile=
    make && \
    make install

# Copy the built extension to output
RUN mkdir -p /output && \
    cp \$(php-config --extension-dir)/apm.so /output/apm.so && \
    php -v > /output/php-version.txt && \
    ldd \$(php-config --extension-dir)/apm.so > /output/dependencies.txt || true

CMD ["cat", "/output/apm.so"]
EOF

# Build the image
echo "📦 Building Docker image (this may take a while)..."
docker build -f Dockerfile.build-legacy -t apm-builder-legacy . 2>&1 | grep -E "(Step|Successfully|Error)" || true

# Create a container and copy files
echo "📄 Extracting apm.so..."
CONTAINER_ID=$(docker create apm-builder-legacy)
docker cp $CONTAINER_ID:/output/apm.so ./apm-php53.so
docker cp $CONTAINER_ID:/output/php-version.txt ./build-info.txt 2>/dev/null || true
docker cp $CONTAINER_ID:/output/dependencies.txt ./dependencies.txt 2>/dev/null || true
docker rm $CONTAINER_ID > /dev/null 2>&1

# Clean up
rm Dockerfile.build-legacy
docker rmi apm-builder-legacy > /dev/null 2>&1

# Get file info
FILE_SIZE=$(ls -lh apm-php53.so | awk '{print $5}')
PHP_INFO=$(cat build-info.txt 2>/dev/null | head -1 || echo "PHP 5.3")

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ Build completed successfully!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📦 Extension file: ./apm-php53.so"
echo "📏 Size: $FILE_SIZE"
echo "🔧 Built for: $PHP_INFO"
echo ""
echo "📋 Dependencies:"
[ -f dependencies.txt ] && cat dependencies.txt || echo "   (check dependencies.txt)"
echo ""
echo "🔧 To install:"
echo "   cp apm-php53.so /usr/lib/php5/20100525/apm.so"
echo "   echo 'extension=apm.so' > /etc/php5/conf.d/apm.ini"
echo ""
echo "📋 Verify:"
echo "   php -m | grep apm"
echo ""
