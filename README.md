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

> **_Important!_**
> 
> If using the `/var/log/halon/` folder as in the sample below and it does not exist, when you create it - give it the same permission as the `smtpd` process is using. Eg.
> ```
> mkdir /var/log/halon
> chown halon:staff /var/log/halon
> ```

> **_Important!_**
> 
> If concurrency is higher than ``1`` multiple subqueues will be created for each queue starting with ``.2``.
> When injecting items (using ``http_bulk``) you can only use the primary queue id (without ``.X``) and the items will be distributed within subqueue using round-robin.
> However when using the plugin commands you need to specify which subqueue the action should be taken on. 
> ```
> halonctl plugin command http-bulk status elastic
> halonctl plugin command http-bulk status elastic.2
> ```

### smtpd.yaml

```
plugins:
  - id: http-bulk
    config:
      queues:
        - id: elastic
          concurrency: 1
          path: /var/log/halon/elastic.jlog
          format: ndjson
          url: "https://1.2.3.4:9200/_bulk"
          max_items: 500
          tls_verify: false
        - id: opensearch
          concurrency: 1
          path: /var/log/halon/elastic.jlog
          format: ndjson
          url: "https://1.2.3.4:9200/_bulk"
          max_items: 500
          username: secret
          password: secret
          aws_sigv4: aws:amz:us-east-1:aoss
          headers:
            - "x-amz-content-sha256: UNSIGNED-PAYLOAD"
        - id: custom-ndjson
          concurrency: 1
          path: /var/log/halon/custom-ndjson.jlog
          format: "ndjson"
          url: "http://1.2.3.4:8080/ndjson"
          max_items: 1000
          min_items: 1000
          max_interval: 60
          tls_verify: true
          headers:
            - "Authorization: Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ=="
        - id: custom-csv
          concurrency: 1
          path: /var/log/halon/custom-csv.jlog
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

### Operation of `min_items`, `max_items` and `interval`

During busy periods, when there are many queued events, batches of size `max_items` will be sent.

During quiet periods, to avoid excessive queue latency, the `min_items` and `interval` values are used together.

The plugin will wait for events to arrive:
* for _up to_ `interval` seconds, or
* when `min_items` is reached (whichever happens soonest).

A `min_items` value of 1 effectively means "send immediately when we have just one event". For efficient batching, set 1 > `min_items` >= `max_items`.

## Example

```
import { http_bulk } from "extras://http-bulk";
http_bulk("elastic", json_encode([]));
```

## Autoload

This plugin creates files needed and used by the `smtpd` process, hence this plugin does not autoload when running the `hsh` script interpreter. There are two issues that may occur

1) Bad file permission if logs are created by the user running `hsh` not the `smtpd` process.
2) Mulitple subscribers on the logs file (`smtpd` and `hsh`). Do use this plugin in `hsh` if `smtpd` is running and vice versa.

To overcome the first issue, run `hsh` as `root` and use the `--privdrop` flag to become the same user as `smtpd` is using.

```
hsh --privdrop --plugin http-bulk
```
