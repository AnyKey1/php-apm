# Docker Setup for PHP APM with Elasticsearch

Complete Docker environment for testing PHP APM extension with Elasticsearch driver.

## 🚀 Quick Start

### 1. Build and Start Containers

```bash
cd /Users/AnyKey/Projects/php-apm

# Build and start all services
docker-compose up -d

# View logs
docker-compose logs -f
```

### 2. Wait for Services to Start

The services will be available at:
- **PHP Application**: http://localhost:8080
- **Elasticsearch**: http://localhost:9200
- **Kibana**: http://localhost:5601

Wait about 30 seconds for Elasticsearch and Kibana to fully start.

### 3. Test APM

Open http://localhost:8080 in your browser and click "Run Test" to generate sample events.

## 📦 What's Included

### Services

1. **php-app** (PHP 5.6 with APM Extension)
   - Apache web server
   - APM extension with Elasticsearch driver enabled
   - Pre-configured to send data to Elasticsearch
   - Batch sending enabled (10 docs or 5 seconds)

2. **elasticsearch** (Elasticsearch 7.17.10)
   - Single-node cluster
   - No security (for testing only)
   - Stores APM events and statistics

3. **kibana** (Kibana 7.17.10)
   - Web UI for Elasticsearch
   - Visualize APM data
   - Create dashboards

### Configuration

APM is pre-configured with:
```ini
apm.enabled=On
apm.elasticsearch_enabled=On
apm.elasticsearch_host=elasticsearch
apm.elasticsearch_port=9200
apm.elasticsearch_index=apm-logs
apm.elasticsearch_batch_size=10
apm.elasticsearch_batch_timeout=5
```

## 🔍 Verifying Installation

### Check APM Extension

```bash
docker-compose exec php-app php -m | grep apm
```

Expected output: `apm`

### Check APM Configuration

```bash
docker-compose exec php-app php -i | grep elasticsearch
```

### Check Elasticsearch Connection

```bash
curl http://localhost:9200/_cat/health?v
```

## 📊 Viewing Data

### Via Command Line

```bash
# View all APM documents
curl http://localhost:9200/apm-logs/_search?pretty

# Count documents
curl http://localhost:9200/apm-logs/_count

# View index info
curl http://localhost:9200/_cat/indices?v | grep apm
```

### Via Kibana

1. Open http://localhost:5601
2. Go to **Management** → **Stack Management** → **Index Patterns**
3. Create index pattern: `apm-logs*`
4. Go to **Discover** to view documents
5. Create visualizations and dashboards

## 🛠️ Docker Commands

### Start Services

```bash
docker-compose up -d
```

### Stop Services

```bash
docker-compose down
```

### Stop and Remove Data

```bash
docker-compose down -v
```

### Rebuild PHP Container

```bash
docker-compose build --no-cache php-app
docker-compose up -d php-app
```

### View Logs

```bash
# All services
docker-compose logs -f

# Specific service
docker-compose logs -f php-app
docker-compose logs -f elasticsearch
```

### Execute Commands in Container

```bash
# PHP shell
docker-compose exec php-app bash

# Test curl from PHP container
docker-compose exec php-app curl http://elasticsearch:9200
```

## 🐛 Troubleshooting

### APM Extension Not Loading

Check build logs:
```bash
docker-compose build php-app 2>&1 | tee build.log
```

Verify extension file exists:
```bash
docker-compose exec php-app ls -l /usr/local/lib/php/extensions/
```

### No Data in Elasticsearch

1. Check if APM can reach Elasticsearch:
```bash
docker-compose exec php-app ping -c 3 elasticsearch
docker-compose exec php-app curl -v http://elasticsearch:9200
```

2. Check APM debug output (if compiled with `--with-debugfile`):
```bash
docker-compose exec php-app tail -f /tmp/apm.debug
```

3. Verify PHP errors are being triggered:
```bash
docker-compose exec php-app php -r "trigger_error('Test', E_USER_WARNING);"
```

### Elasticsearch Won't Start

Check memory limits:
```bash
docker stats
```

Elasticsearch needs at least 512MB RAM. Adjust in `docker-compose.yml` if needed.

### Port Already in Use

If ports are already taken, edit `docker-compose.yml`:
```yaml
ports:
  - "8081:80"  # Change 8080 to 8081
```

## 📈 Performance Tuning

### Batch Size

Larger batches = fewer HTTP requests but higher latency:
```ini
apm.elasticsearch_batch_size=50
```

### Batch Timeout

Shorter timeout = more real-time but more requests:
```ini
apm.elasticsearch_batch_timeout=1
```

### Disable Unused Features

```ini
apm.store_stacktrace=Off
apm.store_cookies=Off
apm.store_post=Off
```

## 🔐 Production Considerations

This setup is for **testing only**. For production:

1. Enable Elasticsearch security:
```yaml
environment:
  - xpack.security.enabled=true
  - ELASTIC_PASSWORD=changeme
```

2. Use strong passwords
3. Enable HTTPS
4. Restrict network access
5. Use proper resource limits
6. Enable authentication in APM config

## 📝 Example PHP Test Script

Create `test/custom-test.php`:
```php
<?php
// Generate some load
for ($i = 0; $i < 1000; $i++) {
    $data[] = md5(rand());
}
sleep(1);

// Trigger errors
trigger_error("Custom test warning", E_USER_WARNING);
trigger_error("Custom test notice", E_USER_NOTICE);

echo "Test completed! Check Elasticsearch for logs.\n";
?>
```

Run it:
```bash
docker-compose exec php-app php /var/www/html/custom-test.php
```

## 🎓 Next Steps

1. **Create Kibana Dashboards**
   - Visualize error rates
   - Monitor response times
   - Track memory usage

2. **Set Up Alerts**
   - Alert on error spikes
   - Monitor slow requests
   - Track resource usage

3. **Optimize Configuration**
   - Tune batch settings
   - Adjust thresholds
   - Enable/disable features

4. **Test in Production-Like Environment**
   - Add load testing
   - Test with realistic data
   - Monitor performance impact
