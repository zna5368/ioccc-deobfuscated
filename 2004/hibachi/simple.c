#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 8192

/* "DOCUMENT_ROOT=" is 14 characters, "SCRIPT_FILENAME=" is 16 characters */
#define DOCROOT_PREFIX_LEN 14
#define SCRIPT_PREFIX_LEN 16

char request[BUF_SIZE];      /* raw request: request line plus headers */
char response[BUF_SIZE];     /* status line and headers we send back */
char query_env[BUF_SIZE];    /* "QUERY_STRING=..." for putenv() */
char host[BUF_SIZE];         /* value of the Host: header */
char docroot_env[BUF_SIZE];  /* "DOCUMENT_ROOT=<root>/<host>" */
char script_env[BUF_SIZE];   /* "SCRIPT_FILENAME=<root>/<host><path>" */
char file_buf[BUF_SIZE];     /* buffer used to copy a file to the client */
char *headers;               /* request text following the URI */

char *not_found = "404 Not Found";
char *server_error = "500 Error";

/*
 * Send an HTTP/1.0 status line and Content-Length header.  When length is
 * zero the status text itself is sent as the body of the response.
 */
void send_response(int sock, char *status, long length)
{
	long size;

	if (length) {
		size = snprintf(response, BUF_SIZE,
				"HTTP/1.0 %s\r\nContent-Length: %ld\r\n\r\n",
				status, length);
	} else {
		size = snprintf(response, BUF_SIZE,
				"HTTP/1.0 %s\r\nContent-Length: %ld\r\n\r\n%s",
				status, (long) strlen(status), status);
	}
	send(sock, response, size, 0);
}

/*
 * Copy the value of a request header into the storage of an environment
 * variable that already exists (hibachi-start.sh reserves room for it).
 * Header names were upper-cased with '-' turned into '_', so the variable
 * name minus its first five characters ("HTTP_" or "CONTE") is searched for.
 */
void copy_header_to_env(char *name, char *format)
{
	char *found = strstr(headers, name + 5);

	if (found) {
		char *value = found + strcspn(found, ":") + 1;
		sscanf(value, format, getenv(name));
	}
}

/*
 * Handle a single HTTP request on a connected socket.  Returns the exit
 * status for the child process.
 */
int handle_request(int client_fd, struct sockaddr_in *addr)
{
	int file_fd, newlines, i;
	int is_head;
	struct stat info;
	char *path, *filename;

	/*
	 * Read the request one byte at a time until a blank line.  Header names
	 * (the text between a newline and the next ':') are upper-cased with '-'
	 * changed to '_', e.g. "Content-Type:" becomes "CONTENT_TYPE:".
	 */
	newlines = 0;
	for (i = 0; newlines < 2 && i < BUF_SIZE; i++) {
		if (recv(client_fd, request + i, 1, 0) <= 0)
			return 1;
		if (request[i] == '\n')
			newlines++;
		if (request[i] == ':')
			newlines = 0;
		if (newlines)
			request[i] = request[i] == '-' ? '_' : toupper(request[i]);
	}
	if (BUF_SIZE <= i)
		return 0;
	request[i] = '\0';

	/* only GET, HEAD and POST are supported */
	if (request[0] != 'G' && request[0] != 'H' && request[2] != 'S') {
		send_response(client_fd, "501 Unsupported", 0);
		return 0;
	}
	is_head = request[0] == 'H';

	/* virtual hosts: the document root is a subdirectory per host name */
	host[0] = '\0';
	if ((path = strstr(request, "HOST:")))
		sscanf(path + 5, " %255[^:\r\n]", host);
	snprintf(docroot_env, BUF_SIZE, "DOCUMENT_ROOT=%s/%s",
		 getenv("DOCUMENT_ROOT"), host);
	putenv(docroot_env);

	/* skip "GET " (4 characters) or "HEAD " / "POST " (5 characters) */
	path = request + 4 + (request[0] != 'G');
	if (*path != '/' || strstr(path, "../")) {
		send_response(client_fd, not_found, 0);
		return 0;
	}
	i = strcspn(path, " ");
	path[i] = '\0';
	headers = path + i + 1;

	/* split off the query string */
	i = strcspn(path, "?");
	if (path[i]) {
		snprintf(query_env, BUF_SIZE, "QUERY_STRING=%s", path + i + 1);
		putenv(query_env);
		path[i] = '\0';
	}

	snprintf(script_env, BUF_SIZE, "SCRIPT_FILENAME=%s%s",
		 docroot_env + DOCROOT_PREFIX_LEN, path);
	filename = script_env + SCRIPT_PREFIX_LEN;

	if (stat(filename, &info) < 0) {
		send_response(client_fd, not_found, 0);
		return 0;
	}

	/* for a directory, serve the first "index.*" file found in it */
	if (S_ISDIR(info.st_mode)) {
		struct dirent *entry;
		DIR *dir = opendir(filename);

		while ((entry = readdir(dir))) {
			if (!strncmp(entry->d_name, "index.", 6)) {
				strncat(filename, entry->d_name,
					BUF_SIZE - SCRIPT_PREFIX_LEN);
				stat(filename, &info);
				break;
			}
		}
		closedir(dir);
		if (entry == NULL) {
			send_response(client_fd, not_found, 0);
			return 0;
		}
	}

	/* HEAD only gets the headers, even for CGI programs */
	if (is_head) {
		send_response(client_fd, "200 OK", info.st_size);
		return 0;
	}

	/* executable files are run as CGI programs */
	if (info.st_mode & 0111) {
		putenv(script_env);
		/* write straight into space reserved by hibachi-start.sh */
		sscanf(request, "%4s", getenv("REQUEST_METHOD"));
		sscanf(inet_ntoa(addr->sin_addr), "%15s", getenv("REMOTE_ADDR"));
		copy_header_to_env("HTTP_COOKIE", "%*[ \t]%80[^\r\n]");
		copy_header_to_env("CONTENT_LENGTH", "%20s");
		copy_header_to_env("CONTENT_TYPE", "%60s");
		dup2(client_fd, 0);
		dup2(client_fd, 1);
		if (system(filename))
			send_response(client_fd, server_error, 0);
		return 0;
	}

	/* otherwise send the file itself */
	file_fd = open(filename, O_RDONLY);
	if (file_fd < 0) {
		send_response(client_fd, server_error, 0);
		return 0;
	}
	send_response(client_fd, "200 OK", info.st_size);
	while (0 < (i = read(file_fd, file_buf, BUF_SIZE)))
		send(client_fd, file_buf, i, 0);

	return 0;
}

int main(int argc, char **argv)
{
	int listen_fd, client_fd, status, i;
	struct sockaddr_in addr;
	char *port;

	/*
	 * This early return is in hibachi.c (it is not in hibachi.orig.c), so the
	 * server exits right away and everything below is never run.
	 */
	return 1;

	port = getenv("SERVER_PORT");
	if (port == NULL) {
		return 4;
	}
	i = (int) strtol(port, NULL, 10);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons((unsigned short) i);
	addr.sin_family = AF_INET;

	listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_fd < 0) {
		return 5;
	}
	if (bind(listen_fd, (void *) &addr, sizeof addr)) {
		return 6;
	}
	if (listen(listen_fd, 10)) {
		return 7;
	}

	for (;;) {
		/* reap any finished child processes */
		while (waitpid(0, NULL, WNOHANG) > 0)
			;

		i = sizeof addr;
		client_fd = accept(listen_fd, (void *) &addr, &i);
		if (client_fd < 0) {
			close(client_fd);
			continue;
		}
		if (fork()) {
			/* parent: go back and wait for the next client */
			close(client_fd);
			continue;
		}
		/* child: handle this one request and then exit */
		break;
	}

	status = handle_request(client_fd, &addr);
	if (status == 0)
		shutdown(client_fd, SHUT_WR);
	return status;
}
