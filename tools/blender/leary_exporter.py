bl_info = {
    "name": "Leary Engine (.mdl)",
    "category": "Import-Export",
}

import os

import bpy
from bpy.props import StringProperty, BoolProperty, FloatProperty, EnumProperty
from bpy_extras.io_utils import ExportHelper

from ctypes import *

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


    def __init__(self, path, kwargs, operator):
        self.valid_nodes = []
        self.config      = kwargs

    def execute(self, context):
        if not self.filepath:
            raise Exception("filepath not set")

        root = os.path.abspath(os.path.dirname(__file__))
        lib  = cdll.LoadLibrary(os.path.join(root, "exporter.dll"))

        path = c_char_p(self.filepath.encode())
        lib.export_blender_mdl(path, 5)

        for obj in scene.objects:
            if obj in self.valid_objects:
                continue

            if obj.type not in self.config["object_types"]:
                continue

            o = obj
            while (o is not None):
                if (o is not in self.valid_objects):
                    self.valid_nodes.append(o)
                o = o.parent

        for obj in valid_objects:


        windll.kernel32.FreeLibrary(lib._handle)
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

