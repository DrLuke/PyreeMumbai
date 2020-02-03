from __future__ import annotations

from pathlib import Path

from PyreeEngine.layers import BaseEntry, LayerContext
from PyreeEngine.shaders import HotloadingShader
from PyreeEngine.simpleshader import SimpleShader
from pyutil.transition import *
from PyreeEngine.basicObjects import ModelObject
from PyreeEngine.camera import Camera
from PyreeEngine.framebuffers import RegularFramebuffer, DefaultFramebuffer

import random

import asyncio

import json
import os

import numpy as np


def tri_coords(x, y):
    side_length = math.sqrt(0.5 ** 2 + 1)
    heron_s = (side_length * 2 + 1) / 2
    base_height = 2 * math.sqrt(heron_s * (heron_s - 1) * (heron_s - side_length) ** 2) / side_length
    intersect_length = math.sqrt(1 - (base_height ** 2)) * 1.115

    p1 = np.array([-0.5, 1])
    p2 = np.array([0.5, 1])

    s1 = np.array([1, 0]) + p1 / np.linalg.norm(p1) * (intersect_length / np.linalg.norm(p1))
    s2 = np.array([-1, 0]) + p2 / np.linalg.norm(p2) * (intersect_length / np.linalg.norm(p2))
    s3 = np.array([0, -1])

    v1 = np.array([x, y])
    v2 = np.array([x - 1, y])
    v3 = np.array([x - 0.5, y - 1])

    return (
        np.dot(s1, v1) / (np.linalg.norm(s1) ** 2),
        np.dot(s2, v2) / (np.linalg.norm(s2) ** 2),
        np.dot(s3, v3) / (np.linalg.norm(s3) ** 2),
    )


class LayerEntry(BaseEntry):
    def __init__(self, context):
        super(LayerEntry, self).__init__(context)

        self.reloadcounter = 0
        with open("config.json", "r") as f:
            config = json.load(f)

        client_name = os.environ["PYREE_CLIENT"]
        self.client_info = config[client_name]

        self.effect_fb = RegularFramebuffer(self.context.resolution)
        self.effect_shader = HotloadingShader(
            Path("res/glsl/default_vert.glsl"),
            Path("res/glsl/tri_effect.glsl")
        )
        self.tri_effect_obj = ModelObject()
        self.tri_effect_obj.shader = self.effect_shader
        c0, c1, c2 = tri_coords(0, 0), tri_coords(1, 0), tri_coords(0.5, 1)
        self.tri_effect_obj.loadFromVerts([
            -1, -1, 0, 0, 0, c0[0], c0[1], c0[2],
            1, -1, 0, 1, 0, c1[0], c1[1], c1[2],
            0, 1, 0, 0.5, 1, c2[0], c2[1], c2[2]
        ])
        self.tri_effect_obj.textures = [
            self.effect_fb.texture
        ]

        self.triobj = ModelObject()
        self.triobj.loadFromVerts(self.tri_verts())
        self.trishader = HotloadingShader(
            Path("res/glsl/trivert.glsl"),
            Path("res/glsl/trifrag.glsl")
        )
        self.triobj.shader = self.trishader
        self.triobj.textures = [
            self.effect_fb.texture
        ]

        self.dfb = DefaultFramebuffer()

    def tri_verts(self):
        # XYZ UV NxNyNz
        verts = []

        subdiv = 24 + 1
        step = 1 / subdiv
        # Bottom row
        for i in range(subdiv):
            for j in range(subdiv - i):
                x0, y0 = (i + j * 0.5) * step, j * step  # Bottom left
                x1, y1 = (i + 1 + j * 0.5) * step, j * step  # Bottom Right
                x2, y2 = (i + 0.5 + j * 0.5) * step, (j + 1) * step  # Top

                c0, c1, c2 = tri_coords(x0, y0), tri_coords(x1, y1), tri_coords(x2, y2)

                verts += [
                    x0, y0, 0, x0, y0, c0[0], c0[1], c0[2],
                    x1, y1, 0, x1, y1, c1[0], c1[1], c1[2],
                    x2, y2, 0, x2, y2, c2[0], c2[1], c2[2],
                ]

        # Inverted row
        for i in range(subdiv - 1):
            for j in range(subdiv - i - 1):
                x0, y0 = (i + 1 + j * 0.5) * step, j * step  # Bottom
                x1, y1 = (i + 1.5 + j * 0.5) * step, (j + 1) * step  # Top Right
                x2, y2 = (i + 0.5 + j * 0.5) * step, (j + 1) * step  # Top Left

                c0, c1, c2 = tri_coords(x0, y0), tri_coords(x1, y1), tri_coords(x2, y2)

                verts += [
                    x0, y0, 0, x0, y0, c0[0], c0[1], c0[2],
                    x1, y1, 0, x1, y1, c1[0], c1[1], c1[2],
                    x2, y2, 0, x2, y2, c2[0], c2[1], c2[2],
                ]

        return verts

    def init(self):
        pass

    def __del__(self):
        pass

    # Tick
    # ======
    def tick(self):
        self.effect_shader.tick()
        self.trishader.tick()

        self.reloadcounter += 1
        if self.reloadcounter > 100:
            with open("config.json", "r") as f:
                try:
                    config = json.load(f)
                    client_name = os.environ["PYREE_CLIENT"]
                    self.client_info = config[client_name]
                except json.JSONDecodeError as e:
                    print("JSON LOAD ERROR")
                    print(e)

        self.tri_effect_obj.setuniform("time", self.context.time)
        self.tri_effect_obj.setuniform("dt", self.context.dt)
        self.tri_effect_obj.setuniform("frame", self.context.frame)

        self.effect_fb.bindFramebuffer()
        self.effect_fb.clearDepth()
        self.tri_effect_obj.render(Camera().viewMatrix)

        self.triobj.setuniform("time", self.context.time)
        self.triobj.setuniform("dt", self.context.dt)
        self.triobj.setuniform("frame", self.context.frame)

        self.dfb.bindFramebuffer()

        for tri_config in self.client_info["tris"]:
            for i in range(3):
                self.triobj.setuniform(f"c{i}pos", tri_config["verts"][i])
            self.triobj.setuniform("dist_amount", tri_config["bends"])
            self.triobj.setuniform("dist_amount_weight", tri_config["bends_weight"])
            self.triobj.render(Camera().viewMatrix)

        #self.effect_fb.rendertoscreen()