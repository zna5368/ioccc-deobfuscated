#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

/*
 * mynx - a tiny text mode web browser.
 *
 * The page (a local file, or the body of an HTTP/1.0 response saved to a
 * temporary file) is read one character at a time.  Text is word wrapped to
 * the terminal width, a few tags start new lines, and links are numbered so
 * that the user can follow one by typing its number at the "? " prompt.
 */

#define LENGTH 512 /* size of every buffer and table */

/* Current page: a local file or a temporary copy of the HTTP response. */
FILE *page;

/* Terminal size, filled in by ioctl(TIOCGWINSZ); 24 x 80 if that fails. */
struct winsize window = {.ws_row = 24, .ws_col = 80};

int link_count;  /* number of links found on the current page */
int line_length; /* number of characters in line[] */
int row;         /* number of screen rows printed so far */
int port;        /* port of the web server */

char *links[LENGTH];     /* links[1..link_count]: URLs of the numbered links */
char line[LENGTH];       /* line being built up before it is printed */
char format_buf[LENGTH]; /* output of print_format() before it is wrapped */
char text[LENGTH];       /* attribute value, entity name or user input */
char host[LENGTH];       /* host name (and ":port") taken from the URL */

/*
 * Character entities read from entities.txt, e.g. "copy 169".
 * An entry with code 0 is unused.
 */
struct entity {
  char code;
  char name[9];
} entities[LENGTH];

/*
 * Convert the name between '&' and ';' into a character:
 * "#65" is a character code, anything else is looked up in the table.
 */
int entity_char(char *name) {
  int i;

  if (*name == '#')
    return (int)strtol(name + 1, 0, 0);

  for (i = 0; i < LENGTH; i++) {
    if (entities[i].code && !strcmp(name, entities[i].name))
      return entities[i].code;
  }
  return '?';
}

/*
 * Add one character to the word wrapped output.
 *
 * line[] holds the text of the current screen line.  It is printed when a
 * newline arrives or when it gets wider than the screen.  Every following
 * line starts with "\n" and three spaces of indent.  Runs of white space are
 * squeezed into a single space.
 */
void print_char(int ch) {
  int printed;

  if (ch == '\n' || window.ws_col - 3 < line_length) {
    /* print the line up to the end (or up to where it was cut) */
    line[line_length] = 0;
    line[line_length + 1] = 0;
    printed = printf("%s", line);
    /* start the next line with the word that did not fit */
    line_length = snprintf(line, LENGTH, "\n   %s", line + printed + 1);
    row++;
  }

  if (line[line_length - 1] != ' ' || !isspace(ch)) {
    line[line_length++] = isspace(ch) ? ' ' : ch;

    /* too wide: cut the line at the last space, before the word */
    if (window.ws_col - 3 < line_length)
      *strrchr(line, ' ') = 0;
  }
}

/* printf() into the word wrapped output. */
void print_format(char *format, ...) {
  va_list args;
  char *p;

  va_start(args, format);
  vsnprintf(format_buf, LENGTH, format, args);
  for (p = format_buf; *p; p++)
    print_char(*p);
  va_end(args);
}

/*
 * Try to match pattern at the current position of the page, ignoring
 * case.  A '*' in the pattern skips ahead until the rest of the pattern is
 * found.  On success the page is left after the matched text and 1 is
 * returned; on failure the page goes back to where it was and 0 is returned.
 */
int match(char *pattern) {
  int ch;
  char *p = pattern;
  char *restart = pattern; /* last '*' seen, or the start of the pattern */
  long start = ftell(page);

  while (*p && (ch = fgetc(page)) != EOF) {
    if (*p == '*')
      restart = p++;
    if (tolower(ch) != *p) {
      if (*restart != '*')
        break;     /* no '*' to fall back on: no match */
      p = restart; /* try again after the '*' with the next character */
    }
    p++;
  }

  if (*p)
    fseek(page, start, SEEK_SET);
  return !*p;
}

/*
 * <p>, <bl...>, <h1>..<h6>, <li>: end the current line and leave a blank
 * line.  Headings start at the left margin, without the indent.
 */
void block_tag(char *tag) {
  if (4 < line_length)
    print_char('\n');
  print_char('\n');
  if (*tag == 'h')
    line_length = 1;
}

/*
 * Tags with a quoted attribute value, and tags that only break the line.
 *
 * The value after "href=" or "src=" is read.  <a>, <frame> and <iframe>
 * get it added to the link table and shown as " [number] ".  Every tag
 * except <a> then prints the value (empty for <br>, <hr>, </h1>, ...)
 * followed by one newline for <br>, <hr> and </hN>, or two for the rest.
 */
void value_tag(char *tag) {
  int is_link = tag[0] == 'a' || tag[0] == 'f' || tag[1] == 'f';
  int is_anchor = tag[0] == 'a';
  int one_newline = tag[0] == 'b' || tag[1] == 'h';
  int quote;

  *text = 0;
  quote = fgetc(page);
  ungetc(quote, page);
  fscanf(page, quote == '"' ? "\"%511[^\"]" : "'%511[^']", text);

  if (is_link) {
    free(links[++link_count]);
    links[link_count] = strdup(text);
    print_format(" [%d] ", link_count);
  }
  if (!is_anchor)
    print_format("%s%s\n", text, one_newline ? "" : "\n");

  match("*>"); /* skip the rest of the tag */
}

/*
 * What to do after a '<'.  The first pattern that matches is used.
 * The handler is given the pattern so that it can tell the tags apart.
 */
struct tag_handler {
  char *pattern;
  void (*handler)(char *tag);
} tags[] = {
    {"!--*-->", 0},          /* comment: skip it */
    {"a *href=", value_tag}, /* link */
    {"ifr*src=", value_tag}, /* inline frame: link */
    {"fra*src=", value_tag}, /* frame: link */
    {"img*src=", value_tag}, /* image: show its file name */
    {"p*>", block_tag},
    {"bl*>", block_tag},
    {"br", value_tag},
    {"hr", value_tag},
    {"h*>", block_tag},
    {"/h", value_tag},
    {"/tr", value_tag},
    {"/dt", value_tag},
    {"/dd", value_tag},
    {"li*>", block_tag},
    {"*>", 0}, /* any other tag: skip it */
    {0, 0},
};

int main(int argc, char **argv) {
  int ch, url_length = 0;
  char *url = argv[1];

  ioctl(0, TIOCGWINSZ, &window);

  /* load the entity table: "name code" per line */
  page = fopen("entities.txt", "r");
  if (page) {
    int i = 0;
    while (fscanf(page, "%s %d", entities[i].name, &ch) == 2)
      entities[i++].code = ch;
    fclose(page);
  }

  while (url) {
    int sock = 0, count;
    struct hostent *server;
    struct sockaddr_in address;

    link_count = 0;
    line_length = 1;

    /* note: host keeps its value from the last page if this is not http:// */
    if (!sscanf(url, "http://%255[^/]%n", host, &url_length) && !*host) {
      /* not a web address: open it as a local file */
      page = fopen(url, "rb");
      if (!page)
        return 1;
    } else {
      /* split "host:port" */
      if (0 < sscanf(host, "%*[^:]%n:%d", &ch, &port))
        host[ch] = 0;
      else
        port = 80;

      server = gethostbyname(host);
      if (!server)
        return 1;
      memcpy(&address.sin_addr.s_addr, server->h_addr, server->h_length);
      address.sin_port = htons((short)port);
      address.sin_family = server->h_addrtype;

      page = tmpfile();
      if (!page)
        return 1;

      /* keep trying with a new socket until connect() succeeds */
      while (connect(sock = socket(address.sin_family, SOCK_STREAM, 0),
                     (void *)&address, sizeof address) ||
             errno == EINTR)
        ;

      /* send the request and save the whole response in the temporary file */
      count = snprintf(line, LENGTH, "GET %s%s HTTP/1.0\r\nHost: %s:%d\r\n\r\n",
                       url[url_length] == '/' ? "" : "/", url + url_length,
                       host, port);
      if (send(sock, line, count, 0) == count) {
        while (0 < (count = recv(sock, line, LENGTH, 0)))
          fwrite(line, 1, count, page);
      }
      close(sock);
      /*
       * The original program used one variable for both, so a following
       * link that is not an http:// URL (but still goes to this host) is
       * requested from this offset, usually 0.
       */
      url_length = count;
      rewind(page);

      /* skip the HTTP headers */
      match("*\r\n\r\n");
    }

    /* show one screen of the page at a time; an empty line shows the next */
    do {
      row = 3;
      *line = ' ';
      while (row < window.ws_row && (ch = fgetc(page)) != EOF) {
        if (ch == '<') {
          struct tag_handler *tag;
          for (tag = tags; tag->pattern; tag++) {
            if (match(tag->pattern)) {
              if (tag->handler)
                tag->handler(tag->pattern);
              break;
            }
          }
        } else {
          if (ch == '&') {
            fscanf(page, "%9[^;];", text);
            ch = entity_char(text);
          }
          print_char(isspace(ch) ? ' ' : ch);
        }
      }

      printf("\n? ");
      fflush(stdout);
      fgets(text, LENGTH, stdin);
    } while (*text == '\n');

    /* follow the chosen link; an unused number ends the program */
    url = links[strtol(text, 0, 0)];
    fclose(page);
  }
  return 0;
}
