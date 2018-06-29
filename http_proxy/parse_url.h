
#ifndef __PARSE_URL___
#define __PARSE_URL___



typedef struct url_parser_url {
    char *protocol;
    char *host;
    int port;
    char *path;
    char *query_string;
    int host_exists;
    char *host_ip;
} url_parser_url_t;


void free_parsed_url(url_parser_url_t *url_parsed);
int parse_url(const char *url, bool verify_host, url_parser_url_t *parsed_url);


#endif //__PARSE_URL___
