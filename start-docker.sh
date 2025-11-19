#!/bin/bash

echo "🚀 Starting PHP APM with Elasticsearch..."
echo ""

# Check if Docker is running
if ! docker info > /dev/null 2>&1; then
    echo "❌ Error: Docker is not running. Please start Docker first."
    exit 1
fi

# Build and start containers
echo "📦 Building and starting containers..."
docker-compose up -d --build

if [ $? -ne 0 ]; then
    echo "❌ Failed to start containers"
    exit 1
fi

echo ""
echo "⏳ Waiting for services to start..."
echo ""

# Wait for Elasticsearch
echo "Waiting for Elasticsearch..."
for i in {1..30}; do
    if curl -s http://localhost:9200/_cluster/health > /dev/null 2>&1; then
        echo "✅ Elasticsearch is ready!"
        break
    fi
    echo -n "."
    sleep 2
done

echo ""
echo "Waiting for PHP application..."
for i in {1..10}; do
    if curl -s http://localhost:8080 > /dev/null 2>&1; then
        echo "✅ PHP application is ready!"
        break
    fi
    echo -n "."
    sleep 1
done

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ All services started successfully!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📍 Services:"
echo "   🌐 PHP Application:     http://localhost:8080"
echo "   📊 Elasticsearch:       http://localhost:9200"
echo "   📈 Kibana:              http://localhost:5601"
echo ""
echo "🔍 Quick checks:"
echo "   View APM config:        http://localhost:8080"
echo "   Run test:               http://localhost:8080?test=1"
echo "   View logs:              curl http://localhost:9200/apm-logs/_search?pretty"
echo ""
echo "📚 Documentation:         cat DOCKER_README.md"
echo ""
echo "🛑 To stop:               docker-compose down"
echo "🔄 To restart:            docker-compose restart"
echo "📋 View logs:             docker-compose logs -f"
echo ""
