bl_info = {
    "name": "Leary Engine (.mdl)",
    "category": "Import-Export",
}

import bpy
from bpy.props import StringProperty, BoolProperty, FloatProperty, EnumProperty
from bpy_extras.io_utils import ExportHelper

class LearyExporter(bpy.types.Operator, ExportHelper):
    """Exporter plugin for Leary Engine"""
    bl_idname = "leary_export.mdl"
    bl_label = "Export Leary Model (.mdl)"
    bl_options = {'REGISTER', 'UNDO'}

    filename_ext = ".mdl"

    object_types = EnumProperty(
            name    = "Object Types",
            options = {"ENUM_FLAG"},
            items   = (("EMPTY", "Empty", ""),
                ("MESH", "Mesh", "")),
            default = {"EMPTY", "MESH" })

    def execute(self, context):
        if not self.filepath:
            raise Exception("filepath not set")

        f = open(self.filepath, "w")

        scene = context.scene
        for obj in scene.objects:
            f.write(obj.name)

        f.close()


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

