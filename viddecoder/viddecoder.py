import os

from cffi import FFI

from pathlib import Path

import numpy as np

class VidDecoder():
    def __init__(self, path:Path):
        self.ffi = FFI()
        self.ffi.cdef("""int init(void* dec, char path[]);
void* mallocDecoderData();
uint8_t** nextFrame(void* dec);
void releaseResources(void* dec);
int convertToRGB(void* dec, uint8_t* dest);
int getWidth(void* dec);
int getHeight(void* dec);""")

        self._pyrVidDecode = self.ffi.dlopen(os.path.abspath("viddecoder/viddecode.so"))

        if path.exists:
            self.decdata_p = self._pyrVidDecode.mallocDecoderData()
            cpath = self.ffi.new("char[]", path.absolute().__str__().encode("ascii"))

            self._pyrVidDecode.init(self.decdata_p, cpath)
        else:
            raise Exception

        self.width = self._pyrVidDecode.getWidth(self.decdata_p)
        self.height = self._pyrVidDecode.getHeight(self.decdata_p)

    def loadNextFrame(self):
        result = self._pyrVidDecode.nextFrame(self.decdata_p)
        if(result == self.ffi.NULL):
            return None
        else:
            return result

    def frameToNumpyArray(self, frame):
        a = np.zeros((self.width * self.height * 3), np.uint8)
        pa = self.ffi.cast("uint8_t*", a.ctypes.data)

        #self.ffi.memmove(pa, frame[0], self.width * self.height)

        self._pyrVidDecode.convertToRGB(self.decdata_p, pa)

        return a


    def __del__(self):
        self._pyrVidDecode.releaseResources(self.decdata_p)

