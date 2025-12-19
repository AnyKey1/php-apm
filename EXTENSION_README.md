# PHP APM Extension - Multiple Versions

Built extensions for different PHP versions and system compatibility.

## Available Builds

### 1. apm.so - PHP 5.6 (Modern Systems)
- **PHP Version**: 5.6.x
- **API Version**: 20131226
- **GLIBC**: 2.14+ (Debian 8+, Ubuntu 14.04+)
- **Size**: 238K
- **Built on**: Debian Stretch

**Installation:**
```bash
cp apm.so $(php-config --extension-dir)/
echo 'extension=apm.so' > /etc/php/5.6/mods-available/apm.ini
php5enmod apm
```

### 2. apm-php53.so - PHP 5.3/5.4 (Old Systems)
- **PHP Version**: 5.3.x, 5.4.x
- **API Version**: 20100525 (PHP 5.3), 20100412 (PHP 5.4)
- **GLIBC**: 2.13+ (Debian 7, Ubuntu 12.04+)
- **Size**: 203K
- **Built on**: Debian Wheezy

**Installation for PHP 5.3:**
```bash
cp apm-php53.so /usr/lib/php5/20100525/apm.so
echo 'extension=apm.so' > /etc/php5/conf.d/apm.ini
service php5-fpm restart
```

**Installation for PHP 5.4:**
```bash
cp apm-php53.so /usr/lib/php5/20100412/apm.so
echo 'extension=apm.so' > /etc/php5/conf.d/apm.ini
service php5-fpm restart
```

## Checking Your PHP Version

```bash
# Get PHP version
php -v

# Get API version (extension directory)
php-config --extension-dir

# Check GLIBC version
ldd --version
```

## Common Installation Directories

| PHP Version | Extension Directory | Config Directory |
|-------------|-------------------|------------------|
| PHP 5.3 | `/usr/lib/php5/20090626` or `/usr/lib/php5/20100525` | `/etc/php5/conf.d/` |
| PHP 5.4 | `/usr/lib/php5/20100412` | `/etc/php5/conf.d/` |
| PHP 5.5 | `/usr/lib/php/20121212` | `/etc/php/5.5/mods-available/` |
| PHP 5.6 | `/usr/lib/php/20131226` | `/etc/php/5.6/mods-available/` |

## Troubleshooting

### Error: GLIBC version not found

**Problem:**
```
version `GLIBC_2.14' not found
```

**Solution:** Use `apm-php53.so` instead - it's compiled for older GLIBC.

### Error: Undefined symbol

**Problem:**
```
undefined symbol: zend_parse_parameters_ex
```

**Solution:** You're using the wrong build. Check your PHP API version:
```bash
php -i | grep "PHP API"
```

Then use the matching build.

### Error: Cannot load shared libraries

**Problem:**
```
libcurl.so.4: cannot open shared object file
```

**Solution:** Install libcurl:
```bash
# Debian/Ubuntu
apt-get install libcurl3

# CentOS/RHEL  
yum install libcurl
```

### Checking Dependencies

```bash
ldd apm-php53.so
```

This shows all required shared libraries.

## Configuration

After installation, configure APM in php.ini or a separate config file:

```ini
; Enable APM
extension=apm.so
apm.enabled=On
apm.event_enabled=On

; Elasticsearch driver
apm.elasticsearch_enabled=On
apm.elasticsearch_host=localhost
apm.elasticsearch_port=9200
apm.elasticsearch_index=apm-logs

; Batch sending
apm.elasticsearch_batch_size=10
apm.elasticsearch_batch_timeout=5
```

See [apm.ini](apm.ini) for all configuration options.

## Rebuilding

### For PHP 5.6:
```bash
./build-extension.sh
```

### For PHP 5.3/5.4:
```bash
./build-extension-php53.sh
```

## Testing

After installation, verify the extension is loaded:

```bash
php -m | grep apm
```

Should output: `apm`

View configuration:

```bash
php -i | grep apm
```

## Supported Features

All builds include:
- ✅ Elasticsearch driver with batch sending
- ✅ Event tracking (errors, exceptions)
- ✅ Statistics collection (CPU, memory, duration)
- ✅ Configurable thresholds
- ❌ SQLite3 driver (disabled)
- ❌ MySQL driver (disabled)
- ❌ StatsD driver (disabled)
- ❌ Socket driver (disabled)

## System Requirements

### Minimum:
- Linux x86_64
- GLIBC 2.13+
- libcurl4

### Recommended:
- GLIBC 2.14+
- PHP 5.4+
- Elasticsearch 6.x or 7.x

## Notes

- Extensions are not portable between different PHP versions
- Always match the API version (check extension directory)
- Debian Wheezy build (`apm-php53.so`) has better old system compatibility
- Both builds are 64-bit only
