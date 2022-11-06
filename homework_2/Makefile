.PHONY := clean
object = client server
all: $(object)

$(object): %: %.c
		gcc $< -o $@ -lrt -pthread -g


.PHONY := clean
clean:
		rm -rf $(object)
