import zlib
x = zlib.compress(b"Hello World")
f = open("python_hello_world.txt", "wb")
f.write(x)
f.close()
