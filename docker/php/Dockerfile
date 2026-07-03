# GameLogger PHP backend.
# PHP 7.4 is used deliberately: the original code relies on short open tags (<?)
# and unquoted array keys (array(apm => ...)), both of which are fatal in PHP 8.
FROM php:7.4-apache

# mysqli is required by server/gamelogger/database.php
RUN docker-php-ext-install mysqli && docker-php-ext-enable mysqli

# Enable short_open_tag so the legacy <? ... ?> blocks are parsed.
COPY docker/php/php.ini /usr/local/etc/php/conf.d/gamelogger.ini

# The PHP source is bind-mounted at runtime via docker-compose (./server) for
# local dev. It's also copied into the image here so the built image is
# self-contained for standalone/production use (the bind mount, when present,
# overrides this at runtime).
COPY server/ /var/www/html/

WORKDIR /var/www/html
