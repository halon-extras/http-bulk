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

## Commands

This plugin can be controlled using the ``halonctl`` tool. The following commands are available though ``halonctl plugin command http-bulk ...``.

| Command | |
|------|------|
| start \<queue> | Resume the queue if stopped |
| start-one \<queue> | Resume the queue if stopped, with maxItems = 1, useful for debugging |
| stop \<queue> | Stop the queue |
| status \<queue> | Show the current status of the queue |
| count \<queue> | Show items in queue |
| head \<queue> | Show the first item in queue |
| pop \<queue> | Remove the first item in queue |
| clear \<queue> | Clear the queue |
| last-error \<queue> | Show the last http error from the queue |

Example 

```
halonctl plugin command http-bulk stop elastic 
halonctl plugin command http-bulk clear elastic 
halonctl plugin command http-bulk start elastic 
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
          url: "https://1.2.3.4:9200/_bulk"
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
          preamble: "header1,header2,header3\n"
          headers:
            - "Content-Type: text/csv"
            - "Authorization: Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ=="
```

## Example

```
import { http_bulk } from "extras://http-bulk";
http_bulk("elastic", json_encode([]));
```
