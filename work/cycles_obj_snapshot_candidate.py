"""Checked reader for the native prototype's embedded OBJ snapshot v1."""
import math
import struct
from cycles_obj_triangulation_candidate import triangulate_snapshot

LIMIT = 256 * 1024 * 1024
MISSING = 0xffffffff

def decode_session_snapshot(session):
    blob = session.master.get('sfm64ImportedObjSnapshotV1')
    if blob is None:
        return None
    if not isinstance(blob, bytes):
        raise ValueError('Embedded OBJ snapshot must be a binary attribute')
    return decode_snapshot(blob)

def insert_snapshot_in_scenes(snapshot, scenes):
    if not scenes:
        raise ValueError('Embedded OBJ requires converted shot scenes')
    obj = create_snapshot_object(snapshot, scenes[0])
    for scene in scenes[1:]:
        scene.collection.objects.link(obj)
    return obj

def decode_snapshot(blob):
    data = memoryview(blob)
    if len(data) > LIMIT:
        raise ValueError('OBJ snapshot exceeds size limit')
    offset = 0
    def raw(count):
        nonlocal offset
        if count < 0 or count > len(data) - offset:
            raise ValueError('Truncated OBJ snapshot')
        result = data[offset:offset + count]
        offset += count
        return result
    def integer():
        return struct.unpack('<I', raw(4))[0]
    def count(minimum=1):
        value = integer()
        if value > (len(data)-offset)//minimum:
            raise ValueError('OBJ count exceeds remaining bytes')
        return value
    def number():
        value = struct.unpack('<f', raw(4))[0]
        if not math.isfinite(value):
            raise ValueError('Nonfinite OBJ value')
        return value
    def text():
        length = count()
        if length > 1024*1024:
            raise ValueError('OBJ string exceeds size limit')
        value = bytes(raw(length))
        if b'\0' in value:
            raise ValueError('Null in OBJ string')
        return value.decode('utf-8')
    if bytes(raw(8)) != b'SFM64OBJ' or integer() != 1:
        raise ValueError('Unsupported OBJ snapshot')
    scale = number()
    if scale <= 0:
        raise ValueError('OBJ scale must be positive')
    vectors = [[tuple(number() for _ in range(3)) for _ in range(count(12))]
               for _ in range(3)]
    positions, uvs, normals = vectors
    polygons = []
    polygon_count = count(20)
    if not 0 < polygon_count <= 1000000:
        raise ValueError('Invalid OBJ polygon count')
    for _ in range(polygon_count):
        polygon = dict(zip(('object','group','material','smoothing'), (text() for _ in range(4))))
        corner_count = count(12)
        if corner_count < 3:
            raise ValueError('OBJ polygon has fewer than three corners')
        corners = []
        for _ in range(corner_count):
            corner = tuple(integer() for _ in range(3))
            if corner[0] >= len(positions) or any(index != MISSING and index >= len(values)
                for index, values in zip(corner[1:], (uvs, normals))):
                raise ValueError('Invalid OBJ corner index')
            corners.append(corner)
        polygon['corners'] = corners
        polygons.append(polygon)
    libraries = [text() for _ in range(count(4))]
    images = {}
    for _ in range(count(16)):
        name = text()
        width, height, length = integer(), integer(), count()
        if not 0 < width <= 16384 or not 0 < height <= 16384 or length != width*height*4:
            raise ValueError('Invalid OBJ image dimensions')
        pixels = bytes(raw(length))
        if name in images or any(alpha != 255 for alpha in pixels[3::4]):
            raise ValueError('Duplicate material or unsupported transparent image')
        images[name] = dict(width=width, height=height, rgba=pixels)
    if offset != len(data) or set(images) != {p['material'] for p in polygons}:
        raise ValueError('Trailing bytes or mismatched OBJ material images')
    return dict(scale=scale, positions=positions, uvs=uvs, normals=normals,
                polygons=polygons, libraries=libraries, images=images)

def snapshot_corner_normals(snapshot):
    faces = [[0.,0.,0.] for _ in snapshot['polygons']]
    shared = {}
    for index,corners in triangulate_snapshot(snapshot):
        a,b,c = [snapshot['positions'][corner[0]] for corner in corners]
        x,y = [b[k]-a[k] for k in range(3)], [c[k]-a[k] for k in range(3)]
        normal = (x[1]*y[2]-x[2]*y[1], x[2]*y[0]-x[0]*y[2], x[0]*y[1]-x[1]*y[0])
        for k in range(3):
            faces[index][k] += normal[k]
    for polygon,normal in zip(snapshot['polygons'],faces):
        group = polygon['smoothing']
        if group not in ('','off','0'):
            for position in {c[0] for c in polygon['corners']}:
                total = shared.setdefault((position,group), [0.,0.,0.])
                for k in range(3):
                    total[k] += normal[k]
    result = []
    for polygon,face in zip(snapshot['polygons'],faces):
        group = polygon['smoothing']
        for position,uv,index in polygon['corners']:
            normal = snapshot['normals'][index] if index != MISSING else (
                shared[(position,group)] if group not in ('','off','0') else face)
            length = math.sqrt(sum(v*v for v in normal))
            if not math.isfinite(length) or length == 0:
                raise ValueError('OBJ zero or nonfinite normal')
            x,y,z = (v/length for v in normal)
            result.append((-z,-x,y))
    return result

def scaled_snapshot_positions(snapshot):
    triangles=triangulate_snapshot(snapshot)
    used={corner[0] for _,corners in triangles for corner in corners}
    center=[(min(snapshot['positions'][i][k] for i in used)+max(snapshot['positions'][i][k] for i in used))*.5 for k in range(3)]
    scaled={}
    for index in used:
        point=[]
        for k in range(3):
            value=center[k]+(snapshot['positions'][index][k]-center[k])*snapshot['scale']
            try:value=struct.unpack('<f',struct.pack('<f',value))[0]
            except (OverflowError,struct.error):raise ValueError('OBJ scale position overflow')
            if not math.isfinite(value):raise ValueError('OBJ scale position overflow')
            point.append(value)
        scaled[index]=tuple(point)
    for _,corners in triangles:
        a,b,c=[scaled[corner[0]] for corner in corners]
        x,y=[b[k]-a[k] for k in range(3)],[c[k]-a[k] for k in range(3)]
        if (x[1]*y[2]-x[2]*y[1],x[2]*y[0]-x[0]*y[2],x[0]*y[1]-x[1]*y[0])==(0.,0.,0.):
            raise ValueError('OBJ scale collapses triangle')
    return scaled

def create_snapshot_object(snapshot, scene, name='Imported OBJ'):
    """Create owned Blender geometry; normal/triangulation parity remains pending."""
    import bpy
    import numpy as np
    scaled_positions=scaled_snapshot_positions(snapshot)
    vertices, faces, uv_values, normal_values, material_names = [], [], [], [], []
    source_normals = snapshot_corner_normals(snapshot)
    offsets=[];offset=0
    for polygon in snapshot['polygons']:
        offsets.append(offset);offset+=len(polygon['corners'])
    for index,corners in triangulate_snapshot(snapshot):
        polygon = snapshot['polygons'][index]
        face = []
        for corner in corners:
            position,uv,normal = corner
            x,y,z = scaled_positions[position]
            converted = (-z*.0254, -x*.0254, y*.0254)
            if not all(math.isfinite(v) for v in converted):
                raise ValueError('Scaled OBJ position overflow')
            face.append(len(vertices))
            vertices.append(converted)
            uv_values.append(snapshot['uvs'][uv][:2] if uv != MISSING else (0.,0.))
            normal_values.append(source_normals[offsets[index]+polygon['corners'].index(corner)])
        faces.append(face)
        material_names.append(polygon['material'])
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    mesh.normals_split_custom_set([normal_values[loop.vertex_index] for loop in mesh.loops])
    uv_layer = mesh.uv_layers.new(name='Source OBJ UV')
    for loop in mesh.loops:
        uv_layer.data[loop.index].uv = uv_values[loop.vertex_index]
    slots = {}
    for material_name, source in snapshot['images'].items():
        image = bpy.data.images.new(name+' '+material_name, width=source['width'], height=source['height'], alpha=True)
        pixels = np.frombuffer(source['rgba'], dtype=np.uint8).astype(np.float32)/255.
        image.pixels.foreach_set(pixels)
        image.pack()
        material = bpy.data.materials.new(name+' '+material_name)
        material.use_nodes = True
        tree = material.node_tree
        tree.nodes.clear()
        texture = tree.nodes.new('ShaderNodeTexImage')
        texture.image = image
        texture.interpolation = 'Closest'
        emission = tree.nodes.new('ShaderNodeEmission')
        output = tree.nodes.new('ShaderNodeOutputMaterial')
        tree.links.new(texture.outputs['Color'], emission.inputs['Color'])
        tree.links.new(emission.outputs[0], output.inputs['Surface'])
        slots[material_name] = len(mesh.materials)
        mesh.materials.append(material)
    for polygon, material_name in zip(mesh.polygons, material_names):
        polygon.material_index = slots[material_name]
    obj = bpy.data.objects.new(name, mesh)
    scene.collection.objects.link(obj)
    obj['sfm64_obj_scale'] = snapshot['scale']
    obj['sfm64_obj_normal_parity_verified'] = False
    return obj
