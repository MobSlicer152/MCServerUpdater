#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <json-c/json.h>

#include <curl/curl.h>

/* For MSVC */
#pragma warning(disable : 4996)

char *read_file(const char *);
size_t curl_write_data(void *, size_t, size_t, FILE *);
void dl_file(const char *, const char *);

int main()
{
	CURLcode result;
	char *json_buf;
	struct json_object *manifest;
	struct json_object *version_json;
	struct json_object *latest;
	struct json_object *version;
	struct json_object *versions;
	struct json_object *server;
	struct json_object *server_downloads;
	struct json_object *url_server = NULL;
	struct json_object *url_version = NULL;
	struct json_object *tmp;
	struct json_object *current_id;

	printf("Initializing libcurl...\n\n");
	result = curl_global_init(CURL_GLOBAL_ALL);

	if (result != 0) {
		fprintf(stderr, "Error: unable to initialize libcurl\n");
		exit(-1);
	}

	remove("server.jar");

	printf("Fetching new version manifest...\n\n");

	dl_file("launchermeta.mojang.com/mc/game/version_manifest.json",
		"version_manifest.json");

	json_buf = read_file("version_manifest.json");
	manifest = json_tokener_parse(json_buf);

	json_object_object_get_ex(manifest, "latest", &latest);
	json_object_object_get_ex(latest, "snapshot", &version);
	json_object_object_get_ex(manifest, "versions", &versions);

	printf("Looking for version %s...\n\n",
	       json_object_get_string(version));

	printf("Finding URL of %s version JSON...",
	       json_object_get_string(version));

	for (unsigned int i = 0; i < json_object_array_length(versions); i++) {
		tmp = json_object_array_get_idx(versions, i);
		json_object_object_get_ex(tmp, "id", &current_id);
		if (strcmp(json_object_get_string(current_id),
			   json_object_get_string(version))) {
			json_object_object_get_ex(
				/* For some reason it overshoots by one if 1 isn't subtracted from i here */
				json_object_array_get_idx(versions, i - 1),
				"url", &url_version);
			break;
		}
	}

	if (url_version == NULL) {
		fprintf(stderr,
			"Error: unable to find the URL of the version JSON\n");
		exit(-1);
	} else
		printf("%s\n\n", json_object_get_string(url_version));

	char version_json_fname[15];
	strcpy(version_json_fname, json_object_get_string(version));
	strcat(version_json_fname, ".json\0");
	printf("Downloaded %s\n\n", version_json_fname);

	dl_file(json_object_get_string(url_version), version_json_fname);

	printf("Looking for %s server download URL in %s...",
	       json_object_get_string(version), version_json_fname);

	json_buf = read_file(version_json_fname);
	version_json = json_tokener_parse(json_buf);

	json_object_object_get_ex(version_json, "downloads", &server_downloads);
	json_object_object_get_ex(server_downloads, "server", &server);
	json_object_object_get_ex(server, "url", &url_server);

	if (url_server == NULL) {
		fprintf(stderr,
			"Error: unable to locate the server download URL\n");
		exit(-1);
	} else
		printf("%s\n\n", json_object_get_string(url_server));

	char server_jar_fname[15];
	strcpy(server_jar_fname, "server.jar\0");
	printf("Downloaded %s\n\n", server_jar_fname);

	dl_file(json_object_get_string(url_server), server_jar_fname);

	printf("Cleaning up old files...\n\n");
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

	buf = malloc(length * sizeof(size_t));
	if (buf == NULL) {
		fprintf(stderr,
			"Error: unable to allocate memory for buffer\n");
		exit(-1);
	}

	if (fread(buf, 1, length, fp) == NULL) {
		fprintf(stderr, "Error: unable to read file\n");
		exit(-1);
	}

	buf[length] = "\0";
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
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

		result = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		fclose(fp);
	} else {
		fprintf(stderr, "Error: unable to download file");
		exit(-1);
	}
}
