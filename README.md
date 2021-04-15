## httpbulk

This plugin allows you to send JSON data bulked to HTTP endpoints (```httpbulk(id, json-data)```). The queue is file-backed and uses a [jlog](https://github.com/omniti-labs/jlog) database. To use this plugin also add the libjlog.so to any of lib/ folders in not installed by a package manager (eg. /usr/local/lib).

```
queues:
 - id: elastic
   path: /var/run/elastic.jlog
   format: ndjson
   url: http://54.152.103.33:9200/_bulk
   maxitems: 500
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
