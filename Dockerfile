FROM alpine:3.7

ENV DIR /tmp/pg_tail/
ADD * $DIR
RUN apk update \ 
    && apk add postgresql-libs \
    && apk add --virtual .build-deps curl build-base postgresql-dev \
    && cd $DIR && make && make install \
    && apk del .build-deps \
    && rm -rf $DIR /var/cache/apk

ENTRYPOINT ["pg_tail"]
CMD ["--help"]
