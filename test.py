import zlib
x = zlib.compress(b"Hello World", level=0)
f = open("python_hello_world.txt", "wb")
f.write(x)
