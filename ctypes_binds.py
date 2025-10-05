import ctypes

lib = ctypes.CDLL("./build/shredlib.so")

lib.shred_files.argtypes = [ctypes.POINTER(ctypes.c_char_p), ctypes.c_int, ctypes.c_int]
lib.shred_files.restype = None

def shred_files_cpp(file_paths, mode):
    files = [f.encode('utf-8') for f in file_paths]
    arr = (ctypes.c_char_p * len(files))(*files)
    lib.shred_files(arr, len(files), mode)