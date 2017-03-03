
all:
	make -f com.mk
	make -C cli
	make -C ctrl
	make -C ft

clean:
	make -f com.mk clean
	make -C cli clean
	make -C ctrl clean
	make -C ft clean
