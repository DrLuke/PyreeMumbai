from PyreeEngine.textures import Texture
from viddecoder.viddecoder import VidDecoder

from pathlib import Path

from OpenGL.GL import *

import numpy as np

class VideoTexture(Texture):
    def __init__(self, path: Path):
        super(VideoTexture, self).__init__()

        self.decoder = VidDecoder(path)

        if self.textures is not None:
            glDeleteTextures(self.textures)  # Clean up old texture
        self.textures = [glGenTextures(1)]

        self.endCounter = 0

    def fetchNext(self) -> bool:
        nextFrame = self.decoder.loadNextFrame()
        if nextFrame is not None:
            """imdata = []

            for j in range(self.decoder.height):
                for i in range(self.decoder.width):
                    pix = i + j*self.decoder.width
                    imdata += [nextFrame[0][pix], nextFrame[0][pix], nextFrame[0][pix]]

            imdata = np.array(imdata, np.uint8)
            #imdata = np.array(imdata, 'B')"""

            new_array = self.decoder.frameToNumpyArray(nextFrame)

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1)
            glBindTexture(GL_TEXTURE_2D, self.textures[0])
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT)
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT)
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, self.decoder.width, self.decoder.height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                         new_array)
            #print(imdata)
            glGenerateMipmap(GL_TEXTURE_2D)

            return True
        else:
            self.endCounter += 1
            if self.endCounter < 10:
                return self.fetchNext()
            else:
                return False


    def getTexture(self):
        return self.textures[0]