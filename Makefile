all:juicemorph.so

juicemorph.so: juicemorph.c
	gcc -Wall -fPIC -dPIC -c -o juicemorph.o juicemorph.c
	ld -shared -o juicemorph.so juicemorph.o

install:
	cp juicemorph.so /usr/lib/ladspa
	chmod 644 /usr/lib/ladspa/juicemorph.so

clean:
	rm *.o *.so
