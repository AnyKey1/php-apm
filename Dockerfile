# PHP 5.6 with APM Extension and Elasticsearch Support
FROM php:5.6-apache

RUN echo "deb http://archive.debian.org/debian stretch main" > /etc/apt/sources.list
# Install dependencies (only curl needed for Elasticsearch driver)
RUN apt-get update && apt-get install -y \
    git \
    autoconf \
    build-essential \
    libcurl4-openssl-dev \
    --allow-unauthenticated \
    && rm -rf /var/lib/apt/lists/*

# Copy APM source code
COPY . /tmp/php-apm

# Build and install APM extension
WORKDIR /tmp/php-apm
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

# Install APM extension
RUN echo "extension=apm.so" > /usr/local/etc/php/conf.d/apm.ini

# Configure APM for Elasticsearch
RUN echo "; APM Configuration" >> /usr/local/etc/php/conf.d/apm.ini \
    && echo "apm.enabled=On" >> /usr/local/etc/php/conf.d/apm.ini \
    && echo "apm.event_enabled=On" >> /usr/local/etc/php/conf.d/apm.ini \
    && echo "apm.application_id=\"Docker PHP 5.6 App\"" >> /usr/local/etc/php/conf.d/apm.ini \
    && echo "" >> /usr/local/etc/php/conf.d/apm.ini \
    && echo "; Elasticsearch driver" >> /usr/local/etc/php/conf.d/apm.ini \
    && echo "apm.elasticsearch_enabled=On" >> /usr/local/etc/php/conf.d/apm.ini \
    && echo "apm.elasticsearch_stats_enabled=On" >> /usr/local/etc/php/conf.d/apm.ini \
    && echo "apm.elasticsearch_host=elasticsearch" >> /usr/local/etc/php/conf.d/apm.ini \
    && echo "apm.elasticsearch_port=9200" >> /usr/local/etc/php/conf.d/apm.ini \
    && echo "apm.elasticsearch_index=apm-logs" >> /usr/local/etc/php/conf.d/apm.ini \
    && echo "apm.elasticsearch_batch_size=10" >> /usr/local/etc/php/conf.d/apm.ini \
    && echo "apm.elasticsearch_batch_timeout=5" >> /usr/local/etc/php/conf.d/apm.ini \
    && echo "" >> /usr/local/etc/php/conf.d/apm.ini \
    && echo "; Disable other drivers" >> /usr/local/etc/php/conf.d/apm.ini \
    && echo "apm.sqlite_enabled=Off" >> /usr/local/etc/php/conf.d/apm.ini \
    && echo "apm.mysql_enabled=Off" >> /usr/local/etc/php/conf.d/apm.ini \
    && echo "apm.statsd_enabled=Off" >> /usr/local/etc/php/conf.d/apm.ini \
    && echo "apm.socket_enabled=Off" >> /usr/local/etc/php/conf.d/apm.ini

# Clean up
RUN rm -rf /tmp/php-apm

# Enable Apache mod_rewrite
RUN a2enmod rewrite

# Set working directory
WORKDIR /var/www/html

# Expose port 80
EXPOSE 80

CMD ["apache2-foreground"]
