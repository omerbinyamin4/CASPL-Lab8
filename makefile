exec: c-libs
	gcc -g -m32 -o myELF myELF.o
	rm -f myELF.o 
	
c-libs: myELF.c
	gcc -g -m32 -c -o myELF.o myELF.c
	
.PHONY: clean
clean:
	rm -rf ./*.o myELF