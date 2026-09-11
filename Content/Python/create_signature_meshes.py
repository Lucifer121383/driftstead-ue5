"""Original low-poly hook, rope coil and stairs, generated as importable OBJ.

Only generates our own source files and assets; never modifies binary assets as
text. Curved silhouettes complement the licensed Kenney environment kit.
"""
from pathlib import Path
import math
import unreal
from import_demo_art import import_file, DEST


class Shape:
    def __init__(self):
        self.vertices, self.faces = [], []

    def tube(self, points, radius=3, sides=8):
        start = len(self.vertices)
        for i, p in enumerate(points):
            before, after = points[max(0, i-1)], points[min(len(points)-1, i+1)]
            t = [after[k]-before[k] for k in range(3)]
            n = math.sqrt(sum(v*v for v in t)) or 1
            t = [v/n for v in t]
            helper = (0, 0, 1) if abs(t[2]) < .9 else (0, 1, 0)
            a = (t[1]*helper[2]-t[2]*helper[1], t[2]*helper[0]-t[0]*helper[2], t[0]*helper[1]-t[1]*helper[0])
            n = math.sqrt(sum(v*v for v in a)) or 1
            a = [v/n for v in a]
            b = (t[1]*a[2]-t[2]*a[1], t[2]*a[0]-t[0]*a[2], t[0]*a[1]-t[1]*a[0])
            for j in range(sides):
                ang = j*math.tau/sides
                self.vertices.append(tuple(p[k]+radius*(a[k]*math.cos(ang)+b[k]*math.sin(ang)) for k in range(3)))
        for i in range(len(points)-1):
            for j in range(sides):
                self.faces.append((start+i*sides+j, start+i*sides+(j+1)%sides, start+(i+1)*sides+(j+1)%sides, start+(i+1)*sides+j))
        self.faces.append(tuple(start+j for j in reversed(range(sides))))
        self.faces.append(tuple(start+(len(points)-1)*sides+j for j in range(sides)))

    def box(self, center, size):
        start = len(self.vertices)
        for x, y, z in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]:
            self.vertices.append((center[0]+x*size[0]/2,center[1]+y*size[1]/2,center[2]+z*size[2]/2))
        for face in [(3,2,1,0),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]:
            self.faces.append(tuple(start+i for i in face))

    def save(self, name):
        folder=Path(unreal.Paths.project_dir())/'SourceArt/Original'
        folder.mkdir(parents=True,exist_ok=True)
        path=folder/(name+'.obj')
        lines=['# Driftstead original procedural art; OBJ uses Y-up for FBX importer','o '+name,'vt .5 .5']
        lines += ['v %.5f %.5f %.5f' % (x,z,-y) for x,y,z in self.vertices]
        lines += ['f '+' '.join(str(i+1)+'/1' for i in face) for face in self.faces]
        path.write_text('\n'.join(lines),encoding='utf-8')
        return path


def solid(name, color, metallic=0, glow=.12):
    mat=unreal.load_asset(f'{DEST}/M_{name}')
    if mat:
        return mat
    mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_'+name,DEST,unreal.Material,unreal.MaterialFactoryNew())
    lib=unreal.MaterialEditingLibrary
    base=lib.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-200,0)
    base.constant=unreal.LinearColor(*color,1)
    lib.connect_material_property(base,'',unreal.MaterialProperty.MP_BASE_COLOR)
    ambient=lib.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-200,100)
    ambient.constant=unreal.LinearColor(*(v*glow for v in color),1)
    lib.connect_material_property(ambient,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    metal=lib.create_material_expression(mat,unreal.MaterialExpressionConstant,-200,200)
    metal.r=metallic
    lib.connect_material_property(metal,'',unreal.MaterialProperty.MP_METALLIC)
    lib.recompile_material(mat)
    return mat


def main():
    solid('Beacon',(.8,.45,.08),0,4)
    hook=Shape()
    hook.tube([(0,0,4),(0,0,58)],4)
    hook.tube([(8*math.cos(i*math.tau/24),0,66+8*math.sin(i*math.tau/24)) for i in range(25)],2.6)
    for a in [0,math.tau/3,math.tau*2/3]:
        hook.tube([(24*math.sin(i*math.pi*.72/18)*math.cos(a),24*math.sin(i*math.pi*.72/18)*math.sin(a),6+24*(1-math.cos(i*math.pi*.72/18))) for i in range(19)],3.3)
    coil=Shape()
    coil.tube([((20+i*.025)*math.cos(i*.12),(20+i*.025)*math.sin(i*.12),4+i*.022) for i in range(150)],3.4)
    stairs=Shape()
    for i in range(7):
        stairs.box((0,i*22,i*17+9),(105,25,12))
    for x in [-53,53]:
        stairs.tube([(x,-8,35),(x,144,145)],5)
        stairs.tube([(x,-8,8),(x,-8,67)],4)
        stairs.tube([(x,144,110),(x,144,175)],4)
        stairs.tube([(x,-8,67),(x,144,175)],4)
    for name,shape,mat in [('GrapplingHook',hook,solid('HookSteel',(.23,.36,.40),.45)),('RopeCoil',coil,solid('CoilFiber',(.47,.29,.12))),('DeckStairs',stairs,solid('StairWood',(.48,.23,.085)))]:
        options=unreal.FbxImportUI()
        options.import_as_skeletal=False
        options.import_materials=False
        options.import_textures=False
        options.automated_import_should_detect_type=False
        options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
        options.static_mesh_import_data.combine_meshes=True
        mesh=import_file(shape.save(name),'S_'+name,options)
        mesh.set_material(0,mat)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    unreal.EditorAssetLibrary.save_directory(DEST,only_if_is_dirty=True,recursive=True)


if __name__=='__main__':
    main()
