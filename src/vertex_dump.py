from PIL import Image
import numpy as np
vertices = []
texture = []
final = ""


# torso -> [pixel data for torso.jpg]
# load in the texture data
l_legs = np.array(Image.open("../assets/futurefemale/legs1.tga").convert("RGB"))
h_head = np.array(Image.open("../assets/futurefemale/head1.tga").convert("RGB"))
u_torso = np.array(Image.open("../assets/futurefemale/torso1.tga").convert("RGB"))

texture_id_to_texture_data = {
    "l_legs" : l_legs.flatten().tolist(),
    "h_head": h_head.flatten().tolist(),
    "u_torso": u_torso.flatten().tolist()
}

texture_id_to_xyzuv = {}
current_tex = None
with open("../assets/futurefemale/jkm_female1.obj") as obj:
    while line := obj.readline():
        line = line[:-1]
        if line.startswith("o "):
            if current_tex is not None:
                texture_id_to_xyzuv[current_tex] = final
                final = ""
            current_tex = line.split(" ")[-1]
            continue
        if line.startswith("v "):
            spaces_split = line.split(" ")
            x = float(spaces_split[1])
            y = float(spaces_split[2])
            z = float(spaces_split[3])
            vertices.append((x, y, z))
        if line.startswith("vt"):
            spaces_split = line.split(" ")
            u = float(spaces_split[1])
            v = float(spaces_split[2])
            texture.append((u, v))
        if line.startswith("f "):
            faces = line.split(" ")
            if len(faces) != 4:
                continue
            for face in faces:
                if face == "f":
                    continue
                face_vertices = face.split("/")
                selected_vertex = vertices[int(face_vertices[0]) - 1]
                selected_tex = texture[int(face_vertices[1]) - 1]
                final += str((selected_vertex[0]))
                final += " "
                final += str((selected_vertex[1]))
                final += " "
                final += str((selected_vertex[2]))
                final += " "
                final += str(selected_tex[0])
                final += " "
                final += str(selected_tex[1])
                final += "\n"

if current_tex is not None: 
    texture_id_to_xyzuv[current_tex] = final
    for tex_id, vert_dat in texture_id_to_xyzuv.items():
        with open(f"{tex_id}", "w") as file:
            file.write(vert_dat)

"""
with open("tex_id_dat_pairing.json", "w") as file:
    json.dump(texture_id_to_texture_data, file)
                final += str((selected_tex[1]))
with open("tex_id_vert_pairing.json", "w") as file:
    json.dump(texture_id_to_xyzuv, file)
"""
