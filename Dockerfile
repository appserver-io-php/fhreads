################################################################################
# Dockerfile for php extensions development
################################################################################

# base image
FROM alpine

# author
MAINTAINER Johann Zelger <zelger@me.com>

# define vars
ENV PHP_EXT_NAME fhreads

# add extension source
ADD . /tmp/${PHP_EXT_NAME}

# compile php7 with fhreads
RUN apk add --update alpine-sdk wget curl openssl-dev autoconf bison && \
	git clone https://github.com/php/php-src.git --branch PHP-7.0 --single-branch && \
	cd /php-src && \
	mv /tmp/${PHP_EXT_NAME} ext/ && \
	git apply ext/fhreads/patches/zend_objects_API.patch && \
	./buildconf --force && \
	./configure \
		--disable-all \
		--enable-maintainer-zts \
		--enable-${PHP_EXT_NAME} \
		--enable-sockets \
		--enable-json \
		--enable-phar \
		--enable-filter \
		--enable-hash \
		--enable-iconv \
		--with-openssl && \
	make -j4 && \
	make install && \
	curl -sS https://getcomposer.org/installer | php -- --install-dir=/usr/bin --filename=composer && \
	rm -rf /php-src && \
	apk del alpine-sdk wget curl openssl-dev autoconf bison && \
	rm -rf /var/cache/apk/*

ENTRYPOINT ["sh"]