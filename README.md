# HTTP bulk plugin

This plugin allows you to send JSON data bulked to HTTP endpoints (```http_bulk(id, json-data)```). The queue is file-backed and uses a [jlog](https://github.com/omniti-labs/jlog) database.

## Installation

Follow the [instructions](https://docs.halon.io/manual/comp_install.html#installation) in our manual to add our package repository and then run the below command.

### Ubuntu

```
apt-get install halon-extras-http-bulk
```

### RHEL

```
yum install halon-extras-http-bulk
```

## Configuration

For the configuration schema, see [http-bulk.schema.json](http-bulk.schema.json). Below is a sample configuration.

### smtpd.yaml

```
plugins:
  - id: http-bulk
    config:
      queues:
        - id: elastic
          path: /var/run/elastic.jlog
          format: ndjson
          url: https://1.2.3.4:9200/_bulk
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
        - id: custom-csv
          path: /var/run/custom-csv.jlog
          format: "custom"
          url: "http://1.2.3.4:8080/csv"
          max_items: 1000
          min_items: 1000
          max_interval: 60
          tls_verify: true
          preamble: "header1,header2,header3"
          headers:
            - "Content-Type: text/csv"
            - "Authorization: Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ=="
```
