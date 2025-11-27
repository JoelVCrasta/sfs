import ctypes
import os

lib = ctypes.CDLL(os.path.join(os.path.dirname(__file__), "build", "shredlib.so"))

lib.shred_files.argtypes = [
    ctypes.POINTER(ctypes.c_char_p), 
    ctypes.c_int, 
    ctypes.c_int]
lib.shred_files.restype = ctypes.c_int

def shred_files_cpp(file_paths, mode: int) -> int:
    """Call into the C++ shred library.

    mode: 0 = quick, 1 = secure
    returns: -1 for invalid input/mode, or number of files that failed (0..len(file_paths))
    """
    if not file_paths:
        return -1
    if mode not in (0, 1):
        return -1

    files = [f.encode("utf-8") for f in file_paths]
    arr = (ctypes.c_char_p * len(files))(*files)
    return lib.shred_files(arr, len(files), mode)