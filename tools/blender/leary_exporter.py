bl_info = {
    "name": "Leary Engine (.mdl)",
    "category": "Import-Export",
}

import os

import bpy
import bmesh
from bpy.props import StringProperty, BoolProperty, FloatProperty, EnumProperty
from bpy_extras.io_utils import ExportHelper

from ctypes import *

class Vector2(Structure):
    _fields_ = [("x", c_float), ("y", c_float)]

class Vector3(Structure):
    _fields_ = [("x", c_float), ("y", c_float), ("z", c_float)]

class Vector3i(Structure):
    _fields_ = [("x", c_int), ("y", c_int), ("z", c_int)]

class Vertex(Structure):
    _fields_ = [("point", Vector3), ("normal", Vector3)]

class Model(Structure):
    _fields_ = [
            ("name", c_char_p),
            ("scale", Vector3),
            ("vertex_count", c_int),
            ("vertices", POINTER(Vertex)),
            ("index_count", c_int),
            ("indices", POINTER(Vector3i)),
            ("uv_count", c_int),
            ("uvs", POINTER(Vector2))]

class LearyExporter(bpy.types.Operator, ExportHelper):
    """Exporter plugin for Leary Engine"""
    bl_idname = "leary_export.mdl"
    bl_label = "Export Leary Model (.mdl)"
    bl_options = {'REGISTER', 'UNDO'}

    filename_ext = ".mdl"

    object_types = EnumProperty(
            name    = "Object Types",
            options = {"ENUM_FLAG"},
            items   = (("EMPTY", "Empty", ""), ("MESH", "Mesh", "")),
            default = {"EMPTY", "MESH" })

    def execute(self, context):
        if not self.filepath:
            raise Exception("filepath not set")

        valid_objects = []

        root     = os.path.abspath(os.path.dirname(__file__))
        exporter = cdll.LoadLibrary(os.path.join(root, "exporter.dll"))

        path = c_char_p(self.filepath.encode())
        exporter.init(path)

        for obj in context.scene.objects:
            if obj in valid_objects:
                continue

            o = obj
            while (o is not None):
                if (o not in valid_objects):
                    valid_objects.append(o)
                o = o.parent

        for obj in valid_objects:
            if (obj.type == "MESH"):
                mdl = Model()
                mdl.name = obj.name.encode()
                mdl.scale.x = obj.scale[0]
                mdl.scale.y = obj.scale[1]
                mdl.scale.z = obj.scale[2]

                mesh = obj.to_mesh(context.scene, True, "RENDER")
                mesh.calc_normals()

                bm = bmesh.new()
                bm.from_mesh(mesh)
                bmesh.ops.triangulate(bm, faces = bm.faces)
                bm.to_mesh(mesh)
                bm.free()

                vertices = []
                indices  = []
                uvs      = []

                for fi in range(len(mesh.polygons)):
                    f = mesh.polygons[fi]

                    for lt in range(f.loop_total):
                        loop_index = f.loop_start + lt
                        ml = mesh.loops[loop_index]
                        mv = mesh.vertices[ml.vertex_index]

                        vertex = Vertex()
                        vertex.point.x = mv.co[0]
                        vertex.point.y = mv.co[1]
                        vertex.point.z = mv.co[2]
                        vertex.normal.x = ml.normal[0]
                        vertex.normal.y = ml.normal[1]
                        vertex.normal.z = ml.normal[2]
                        vertices.append(vertex)

                        for xt in mesh.uv_layers:
                            uv = Vector2()
                            uv.x = xt.data[loop_index].uv[0]
                            uv.y = xt.data[loop_index].uv[1]
                            uvs.append(uv)

                mdl.vertex_count = len(vertices)
                mdl.vertices = (Vertex * len(vertices))(*vertices)

                mdl.index_count = len(indices)
                mdl.indices = (Vector3i * len(indices))(*indices)

                mdl.uv_count = len(uvs)
                mdl.uvs = (Vector2 * len(uvs))(*uvs)

                exporter.add_model(mdl)

        exporter.finish()

        windll.kernel32.FreeLibrary(exporter._handle)
        return {'FINISHED'}

def menu_func(self, context):
    self.layout.operator(LearyExporter.bl_idname)

def register():
    bpy.utils.register_class(LearyExporter)
    bpy.types.INFO_MT_file_export.append(menu_func)

def unregister():
    bpy.utils.unregister_class(LearyExporter)
    bpy.types.INFO_MT_file_export.remove(menu_func)

if __name__ == "__main__":
    register()

