#define LD_ARGS LD_PATH, \
	"-dynamic-linker", "/lib64/ld-linux-x86-64.so.2", \
	"/usr/bin/../lib/gcc/x86_64-linux-gnu/9/crtbegin.o", \
	"/usr/bin/../lib/gcc/x86_64-linux-gnu/9/crtend.o", \
	"/usr/bin/../lib/gcc/x86_64-linux-gnu/9/../../../x86_64-linux-gnu/crt1.o", \
	"/usr/bin/../lib/gcc/x86_64-linux-gnu/9/../../../x86_64-linux-gnu/crti.o", \
	"/usr/bin/../lib/gcc/x86_64-linux-gnu/9/../../../x86_64-linux-gnu/crtn.o", \
	"-L/usr/lib/gcc/x86_64-linux-gnu/9/", \
	"-L/usr/lib/gcc/x86_64-linux-gnu/9/", \
	"-L" STATICLIB_DIR, \
	infile, \
	"-levi", \
	"-lc", \
	"-lgcc", \
	"-o", outfile
	
#define LD_ARGC 17