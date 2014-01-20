all:juicemorph.so

juicemorph.so: juicemorph.c
	gcc -Wall -fPIC -DPIC -c -o juicemorph.o juicemorph.c
	ld -shared -o juicemorph.so juicemorph.o

install:
	install -d $(DESTDIR)/usr/lib/ladspa
	install -m 644 juicemorph.so $(DESTDIR)/usr/lib/ladspa

clean:
	rm *.o *.so
