posix:
	gcc main.c cliview.c -DPosix_compile -lpthread

# compilazione su MACOS con interfaccia grafica
macos-gui:
	clang main.c gui.c $(shell pkg-config --cflags --libs gtk4) -DGUI
