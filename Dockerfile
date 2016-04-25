################################################################################
# Dockerfile for php extensions development
################################################################################

# base image
FROM alpine

# author
MAINTAINER Johann Zelger <zelger@me.com>

# define vars
ENV PHP_EXT_NAME fhreads

# install build dependencies
RUN apk add --update alpine-sdk autoconf bison && \
# clone php-src tag for PHP-7.0
	git clone https://github.com/php/php-src.git --branch PHP-7.0 --single-branch

# add current ext sources
ADD . /php-src/ext/$PHP_EXT_NAME

# pre build php with extension
RUN cd /php-src && \
	git apply ext/fhreads/patches/zend_objects_API.patch && \
	./buildconf --force && \
	./configure --disable-all --enable-${PHP_EXT_NAME} --enable-maintainer-zts --enable-sockets && \
	make -j4

# set default entrypoint to be sh
ENTRYPOINT ["sh"]