all: fhttpd
LIBS = -lpthread

fhttpd: fhttpd.c
	gcc $< $(LIBS) -o $@

