# HTTP bulk plugin

This plugin allows you to send JSON data bulked to HTTP endpoints (```http_bulk(id, json-data)```). The queue is file-backed and uses a [jlog](https://github.com/omniti-labs/jlog) database. Build and install the jlog library with ```CFLAGS=-fPIC ./configure```.

## Configuration example

### smtpd.yaml

```
plugins:
  - id: http-bulk
    path: /opt/halon/plugins/http-bulk.so
    config:
      queues:
        - id: elastic
          path: /var/run/elastic.jlog
          format: ndjson
          url: http://54.152.103.33:9200/_bulk
          max_items: 500
          tls_verify: false
        - id: custom-ndjson
          path: /var/run/custom-ndjson.jlog
          format: "ndjson"
          url: "http://1.2.3.4:8080/ndjson"
          max_items: 1000
          min_items: 1000
          max_interval: 60
          tls_verify: true
          headers:
            - "Authorization: Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ=="
```
