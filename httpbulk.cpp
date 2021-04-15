#include <HalonMTA.h>
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <curl/curl.h>
#include <syslog.h>
#include <memory.h>
#include <condition_variable>
#include <jlog.h>
#include <unistd.h>
#include <memory>
#include <map>

struct bulkQueue
{
	bool quit = false;
	std::thread subscriberThread;

	std::mutex writerMutex;
	std::condition_variable writeCV;
	bool writeNotify = false;
	jlog_ctx* writerContext;
	jlog_ctx* readerContext;

	size_t minItems = 1;
	size_t maxItems = 1;
	size_t maxInterval = 0;

	std::string url;
	bool ndjson = false;
	bool tls_verify = true;
	std::vector<std::string> headers;
};

std::map<std::string, std::shared_ptr<bulkQueue>> bulkQueues;

bool curlQuit = false;
std::thread curlThread;
CURLM *curlMultiHandle = NULL;

std::mutex curlQueueLock;
std::queue<CURL*> curlQueue;

struct curlResult {
	std::condition_variable cv;
	long status;
	std::string result;
};

void curl_multi()
{
	do {
		CURLMcode mc;

		int still_running;
		mc = curl_multi_perform(curlMultiHandle, &still_running);

		struct CURLMsg *m;
		do {
			int msgq = 0;
			m = curl_multi_info_read(curlMultiHandle, &msgq);
			if (m && (m->msg == CURLMSG_DONE))
			{
				CURL *e = m->easy_handle;

				curlResult* h;
				curl_easy_getinfo(e, CURLINFO_PRIVATE, &h);

				if (m->data.result != CURLE_OK)
					h->status = -1;
				else
					curl_easy_getinfo(e, CURLINFO_RESPONSE_CODE, &h->status);

				h->cv.notify_one();

				curl_multi_remove_handle(curlMultiHandle, e);
				curl_easy_cleanup(e);
			}
		} while (m);

		int numfds;
		mc = curl_multi_poll(curlMultiHandle, NULL, 0, 10000, &numfds);

		curlQueueLock.lock();
		while (!curlQueue.empty())
		{
			CURL* curl = curlQueue.front();
			curl_multi_add_handle(curlMultiHandle, curl);
			curlQueue.pop();
		}
		curlQueueLock.unlock();
	} while (!curlQuit);
}

size_t write_callback(char *data, size_t size, size_t nmemb, std::string* writerData)
{
	if (writerData == NULL)
		return 0;
	writerData->append((const char*)data, size*nmemb);
	return size * nmemb;
}

HALON_EXPORT
int Halon_version()
{
	return HALONMTA_PLUGIN_VERSION;
}

HALON_EXPORT
void httpbulk(HalonHSLContext* hhc, HalonHSLArguments* args, HalonHSLValue* ret)
{
	HalonHSLValue* id_ = HalonMTA_hsl_argument_get(args, 0);
	if (!id_ || HalonMTA_hsl_value_type(id_) != HALONMTA_HSL_TYPE_STRING)
		return;
	HalonHSLValue* payload_ = HalonMTA_hsl_argument_get(args, 1);
	if (!payload_ || HalonMTA_hsl_value_type(payload_) != HALONMTA_HSL_TYPE_STRING)
		return;

	char* id = nullptr;
	HalonMTA_hsl_value_get(id_, HALONMTA_HSL_TYPE_STRING, &id, nullptr);
	char* payload = nullptr;
	size_t payloadlen = 0;
	HalonMTA_hsl_value_get(payload_, HALONMTA_HSL_TYPE_STRING, &payload, &payloadlen);

	auto x = bulkQueues.find(id);
	if (x == bulkQueues.end())
		return;

	std::unique_lock<std::mutex> lck(x->second->writerMutex);
	if (jlog_ctx_write(x->second->writerContext, payload, payloadlen) != 0)
		syslog(LOG_CRIT, "httpbulk: jlog_ctx_write failed: %d %s", jlog_ctx_err(x->second->writerContext), jlog_ctx_err_string(x->second->writerContext));

	x->second->writeNotify = true;
	x->second->writeCV.notify_one();
	return;
}

void subscriber(std::shared_ptr<bulkQueue> log)
{
	size_t failures = 0;
	auto lastSend = std::chrono::steady_clock::now();

	struct curl_slist* hdrs = nullptr;
	if (log->ndjson)
		hdrs = curl_slist_append(hdrs, "Content-Type: application/x-ndjson");
	else
		hdrs = curl_slist_append(hdrs, "Content-Type: application/json");
	for (const auto & i : log->headers)
		hdrs = curl_slist_append(hdrs, i.c_str());

	while (!log->quit)
	{
		jlog_id begin, end;
		int count = jlog_ctx_read_interval(log->readerContext, &begin, &end);
		if (count < log->minItems)
		{
			std::unique_lock<std::mutex> lk(log->writerMutex);
			if (count > 0 && log->maxInterval > 0)
			{
				log->writeCV.wait_until(lk, lastSend + std::chrono::seconds(log->maxInterval), [&log] { return log->writeNotify || log->quit; });
				if (log->writeNotify)
				{
					log->writeNotify = false;
					continue;
				}
				if (log->quit)
					break;
			}
			else
			{
				log->writeCV.wait(lk, [&log] { return log->writeNotify || log->quit; });
				log->writeNotify = false;
				continue;
			}
		}
		lastSend = std::chrono::steady_clock::now();

		std::string payload;

		if (log->maxItems > 1 && !log->ndjson)
			payload = "[";

		for (size_t items = 0; items < std::min(log->maxItems, (size_t)count); items++, JLOG_ID_ADVANCE(&begin))
		{
			end = begin;
			jlog_message m;
			if (jlog_ctx_read_message(log->readerContext, &begin, &m) != 0)
			{
				syslog(LOG_CRIT, "httpbulk: jlog_ctx_read_message failed: %d %s", jlog_ctx_err(log->readerContext), jlog_ctx_err_string(log->readerContext));
				return;
			}
			if (log->maxItems > 1 && items != 0 && !log->ndjson)
				payload += ",";
			payload += std::string((char*)m.mess, m.mess_len);
			if (log->ndjson)
				payload += "\n";
		}
		if (log->maxItems > 1 && !log->ndjson)
			payload += "]";

		auto h = new curlResult;

		CURL* curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, log->url.c_str());
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
		curl_easy_setopt(curl, CURLOPT_PRIVATE, (void*)h);

		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.size());

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ::write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &h->result);
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);

		if (!log->tls_verify)
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

		curlQueueLock.lock();
		curlQueue.push(curl);
		curl_multi_wakeup(curlMultiHandle);
		curlQueueLock.unlock();

		std::mutex mtx;
		std::unique_lock<std::mutex> lck(mtx);
		h->cv.wait(lck);

		if (h->status == 200)
		{
			jlog_ctx_read_checkpoint(log->readerContext, &end);
			failures = 0;
		}
		else
		{
			++failures;
			if (failures == 1)
				syslog(LOG_CRIT, "httpbulk: failed to send request to %s: %zu %s", log->url.c_str(), h->status, h->result.c_str());
			if (failures > 30)
				syslog(LOG_CRIT, "httpbulk: still unable to send request to %s: %zu %s", log->url.c_str(), h->status, h->result.c_str());
			sleep(1);
		}

		delete h;
	}
	curl_slist_free_all(hdrs);
}

HALON_EXPORT
bool Halon_init(HalonInitContext* hic)
{
	HalonConfig* cfg = nullptr;
	HalonMTA_init_getinfo(hic, HALONMTA_INIT_CONFIG, nullptr, 0, &cfg, nullptr);
	if (!cfg)
		return false;

	try {
		auto queues = HalonMTA_config_object_get(cfg, "queues");
		if (queues)
		{
			size_t l = 0;
			HalonConfig* log;
			while ((log = HalonMTA_config_array_get(queues, l++)))
			{
				auto x = std::make_shared<bulkQueue>();

				const char* id = HalonMTA_config_string_get(HalonMTA_config_object_get(log, "id"), nullptr);
				const char* path = HalonMTA_config_string_get(HalonMTA_config_object_get(log, "path"), nullptr);
				const char* url = HalonMTA_config_string_get(HalonMTA_config_object_get(log, "url"), nullptr);

				if (!id || !path || !url)
					throw std::runtime_error("missing required property");

				const char* format = HalonMTA_config_string_get(HalonMTA_config_object_get(log, "format"), nullptr);
				const char* maxitems = HalonMTA_config_string_get(HalonMTA_config_object_get(log, "max_items"), nullptr);
				const char* minitems = HalonMTA_config_string_get(HalonMTA_config_object_get(log, "min_items"), nullptr);
				const char* maxinterval = HalonMTA_config_string_get(HalonMTA_config_object_get(log, "max_interval"), nullptr);
				const char* tls_verify = HalonMTA_config_string_get(HalonMTA_config_object_get(log, "tls_verify"), nullptr);

				HalonConfig* headers = HalonMTA_config_object_get(log, "headers");
				if (headers)
				{
					size_t h = 0;
					HalonConfig* header;
					while ((header = HalonMTA_config_array_get(headers, h++)))
						x->headers.push_back(HalonMTA_config_string_get(header, nullptr));
				}

				jlog_ctx* ctx = jlog_new(path);
				if (jlog_ctx_init(ctx) != 0)
				{
					if (jlog_ctx_err(ctx) != JLOG_ERR_CREATE_EXISTS)
						throw std::runtime_error(jlog_ctx_err_string(ctx));
					jlog_ctx_add_subscriber(ctx, "subscriber1", JLOG_BEGIN);
				}
				jlog_ctx_close(ctx);
				ctx = jlog_new(path);
				if (jlog_ctx_open_writer(ctx) != 0)
					throw std::runtime_error(jlog_ctx_err_string(ctx));
				x->writerContext = ctx;

				ctx = jlog_new(path);
				jlog_ctx_add_subscriber(ctx, "subscriber1", JLOG_BEGIN);
				if (jlog_ctx_open_reader(ctx, "subscriber1") != 0)
					throw std::runtime_error(jlog_ctx_err_string(ctx));
				x->readerContext = ctx;

				x->url = url;
				x->ndjson = !format || strcmp(format, "ndjson") == 0 ? true : false;
				x->tls_verify = !tls_verify || strcmp(tls_verify, "true") == 0 ? true : false;
				x->maxItems = !maxitems ? 1 : strtoul(maxitems, nullptr, 10);
				x->minItems = !minitems ? 1 : strtoul(minitems, nullptr, 10);
				x->maxInterval = !maxinterval ? 0 : strtoul(maxinterval, nullptr, 10);
				x->subscriberThread = std::thread([x] {
					subscriber(x);
				});

				bulkQueues[id] = x;
			}
		}
	} catch (const std::runtime_error& e) {
		syslog(LOG_CRIT, "httpbulk: %s", e.what());
		return false;
	}

	curlMultiHandle = curl_multi_init();
	curlThread = std::thread(curl_multi);

	return true;
}
 
HALON_EXPORT
void Halon_cleanup()
{
	for (auto & i : bulkQueues)
	{
		i.second->quit = true;
		i.second->writeCV.notify_all();
		i.second->subscriberThread.join();
		jlog_ctx_close(i.second->writerContext);
		jlog_ctx_close(i.second->readerContext);
	}
	curlQuit = true;
	curl_multi_wakeup(curlMultiHandle);
	curlThread.join();
}

HALON_EXPORT
bool Halon_hsl_register(HalonHSLRegisterContext* ptr)
{
	HalonMTA_hsl_register_function(ptr, "httpbulk", &httpbulk);
	return true;
}
