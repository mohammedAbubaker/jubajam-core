import json
import base64
import struct

f = open("assets/Monster.gltf", "r")

json_object = json.loads(f.read())
b = base64.b64decode(json_object["buffers"]["Monster"]["uri"][37:])
accessor = json_object["meshes"]["monster-mesh"]["primitives"][0]["attributes"]["POSITION"]
buffer_view_index = json_object["accessors"][accessor]["bufferView"]
buffer_view_object = json_object["bufferViews"][buffer_view_index]
buffer_start = int(buffer_view_object["byteOffset"])
buffer_end = buffer_start + int(buffer_view_object["byteLength"])
lol = f"{int((buffer_end - buffer_start) / 4)}f"
vertex_dat = struct.unpack(lol, b[buffer_start: buffer_end])
vertices = []
for i in range(0, len(vertex_dat), 3):
    vertices.append((vertex_dat[i], vertex_dat[i+1], vertex_dat[i+2]))  
index_accessor = json_object["accessors"]["accessor_21"]["bufferView"]
buffer_view_other = json_object["bufferViews"][index_accessor]
ibuffer_start = int(buffer_view_other["byteOffset"])
ibuffer_end = ibuffer_start + int(buffer_view_other["byteLength"])
lol2 = f"2652h"
ppooop_dat = struct.unpack(lol2, b[ibuffer_start : ibuffer_end])
print(ppooop_dat)
f.close()
