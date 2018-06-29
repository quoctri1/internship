Generate certificate:
openssl req -x509 -nodes -days 365 -newkey rsa:1024 -keyout mycert.pem -out mycert.pem

Compile:
gcc -Wall -o ssl-client SSL-Client.c (parse_url.c) -L/usr/lib -lssl -lcrypto


