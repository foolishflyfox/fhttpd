all: fhttpd login.cgi
LIBS = -lpthread

fhttpd: fhttpd.c
	gcc $< $(LIBS) -o $@

login.cgi: login.c
	gcc $< -o $@
