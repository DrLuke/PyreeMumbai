from PyreeEngine.layers import LayerConfig, ProgramConfig, LayerManager, LayerContext

from pathlib import Path
from time import sleep
import json

import asyncio

with open("programconfig.json", "r") as f:
    pcon = ProgramConfig(**json.load(f))
    print(pcon)

#layercontext: LayerContext = LayerContext()
#layercontext.setresolution(123, 456)
#layercontext.time = glfw.get_time()
#lman = LayerManager(pcon, layercontext)


from PyreeEngine.engine import Engine

a = Engine()

asyncio.run(a.startmainloop())
