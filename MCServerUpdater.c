#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

#include "curl/curl.h"

/* For MSVC */
#pragma warning(disable : 4996)

char *read_file(const char *);
size_t curl_write_data(void *, size_t, size_t, FILE *);
void dl_file(const char *, const char *);

int main(int argc, char *argv[])
{
	CURLcode result;
	char *json_buf;
	const char *version;
	struct cJSON *manifest;
	struct cJSON *version_json;
	struct cJSON *latest;
	struct cJSON *versions;
	struct cJSON *server;
	struct cJSON *server_downloads;
	struct cJSON *url_server = NULL;
	struct cJSON *url_version = NULL;
	struct cJSON *tmp;
	struct cJSON *current_id;

	printf("Initializing libcurl...\n\n");
	result = curl_global_init(CURL_GLOBAL_ALL);

	if (result != 0) {
		fprintf(stderr, "Error: unable to initialize libcurl\n");
		exit(-1);
	}

	printf("Fetching new version manifest...\n\n");

	dl_file("launchermeta.mojang.com/mc/game/version_manifest.json",
		"version_manifest.json");

	json_buf = read_file("version_manifest.json");
	manifest = cJSON_Parse(json_buf);

	if (argc < 2 || strcmp(argv[1], "snapshot") == 0 || strcmp(argv[1], "release") == 0) {
		latest = cJSON_GetObjectItem(manifest, "latest");
		version = cJSON_GetStringValue(cJSON_GetObjectItem(latest, argc > 1 ? argv[1] : "snapshot"));
	} else {
		version = argv[1];
	}
	versions = cJSON_GetObjectItem(manifest, "versions");

	printf("Looking for version %s...\n\n", version);

	printf("Finding URL of %s version JSON...", version);

	for (unsigned int i = 0; i < cJSON_GetArraySize(versions); i++) {
		tmp = cJSON_GetArrayItem(versions, i);
		current_id = cJSON_GetObjectItem(tmp, "id");
		if (strcmp(cJSON_GetStringValue(current_id), version) == 0) {
			url_version = cJSON_GetArrayItem(versions, i);
			url_version = cJSON_GetObjectItem(url_version, "url");
			break;
		}
	}

	if (url_version == NULL) {
		fprintf(stderr,
			"Error: unable to find the URL of the version JSON\n");
		exit(-1);
	} else {
		printf("%s\n\n", cJSON_GetStringValue(url_version));
	}

	char version_json_fname[15] = { 0 };
	snprintf(version_json_fname, 15, "%s.json", version);
	printf("Downloading %s...", version_json_fname);

	dl_file(cJSON_GetStringValue(url_version), version_json_fname);
	printf("done\n\n");

	printf("Looking for %s server download URL in %s...", version, version_json_fname);

	json_buf = read_file(version_json_fname);
	version_json = cJSON_Parse(json_buf);

	server_downloads = cJSON_GetObjectItem(version_json, "downloads");
	server = cJSON_GetObjectItem(server_downloads, "server");
	url_server = cJSON_GetObjectItem(server, "url");

	if (url_server == NULL) {
		fprintf(stderr,
			"Error: unable to locate the server download URL\n");
		exit(-1);
	} else
		printf("%s\n\n", cJSON_GetStringValue(url_server));

	char server_jar_fname[15];
	snprintf(server_jar_fname, 15, "%s.jar", version);
	remove(server_jar_fname);
	printf("Downloading %s...", server_jar_fname);

	dl_file(cJSON_GetStringValue(url_server), server_jar_fname);
	printf("done\n\n");

	printf("Cleaning up temporary files...\n\n");
	remove(version_json_fname);
	remove("version_manifest.json");

	return 0;
}

char *read_file(const char *fname)
{
	FILE *fp;
	char *buf;
	size_t length;

	fp = fopen(fname, "r");
	if (fp == NULL) {
		fprintf(stderr, "Error: unable to open file\n");
		exit(-1);
	}

	fseek(fp, 0L, SEEK_END);
	length = ftell(fp);
	rewind(fp);

	buf = calloc(length, sizeof(size_t));
	if (buf == NULL) {
		fprintf(stderr,
			"Error: unable to allocate memory for buffer\n");
		exit(-1);
	}

	if (fread(buf, 1, length, fp) < length) {
		fprintf(stderr, "Error: unable to read file\n");
		exit(-1);
	}

	buf[length] = '\0';
	fclose(fp);
	return buf;
}

size_t curl_write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t written;
	written = fwrite(ptr, size, nmemb, stream);
	return written;
}

void dl_file(const char *url, const char *fname)
{
	CURL *curl;
	FILE *fp;
	CURLcode result;
	curl = curl_easy_init();
	if (curl) {
		fp = fopen(fname, "wb");

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

		result = curl_easy_perform(curl);
		if (result != CURLE_OK) {
			fprintf(stderr, "Error: unable to download file\n");
			exit(-1);
		}
		curl_easy_cleanup(curl);
		fclose(fp);
	} else {
		fprintf(stderr, "Error: unable to download file\n");
		exit(-1);
	}
}
