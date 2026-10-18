"""Port of the native candidate's checked planar ear clipping algorithm."""
import math

def triangulate_snapshot(snapshot):
    result=[]
    for face_index,face in enumerate(snapshot['polygons']):
        corners=face['corners']; count=len(corners)
        if count<3:raise ValueError('OBJ polygon arity')
        origin=snapshot['positions'][corners[0][0]]
        p=[tuple(float(v)-origin[k] for k,v in enumerate(snapshot['positions'][c[0]])) for c in corners]
        scale=max(abs(v) for point in p for v in point)
        if not math.isfinite(scale) or scale==0:raise ValueError('OBJ collapsed polygon')
        epsilon=scale*scale*1e-12
        normal=[0.,0.,0.]
        for a,b in zip(p,p[1:]+p[:1]):
            normal[0]+=(a[1]-b[1])*(a[2]+b[2])
            normal[1]+=(a[2]-b[2])*(a[0]+b[0])
            normal[2]+=(a[0]-b[0])*(a[1]+b[1])
        length=math.sqrt(sum(v*v for v in normal))
        if length<=epsilon:raise ValueError('OBJ degenerate polygon')
        if any(abs(sum(q[k]*normal[k] for k in range(3)))/length>scale*1e-6 for q in p):
            raise ValueError('OBJ nonplanar polygon')
        drop=max(range(3),key=lambda k:abs(normal[k]));u=(drop+1)%3;v=(drop+2)%3
        def cross(a,b,c):return (p[b][u]-p[a][u])*(p[c][v]-p[a][v])-(p[b][v]-p[a][v])*(p[c][u]-p[a][u])
        def on(a,b,c):
            return abs(cross(a,b,c))<=epsilon and all(min(p[a][k],p[b][k])-scale*1e-12<=p[c][k]<=max(p[a][k],p[b][k])+scale*1e-12 for k in (u,v))
        for i in range(count):
            following=(i+1)%count
            if math.hypot(p[following][u]-p[i][u],p[following][v]-p[i][v])<=scale*1e-12:
                raise ValueError('OBJ repeated polygon corner')
            for j in range(i+1,count):
                end=(j+1)%count
                if j==following or end==i:continue
                a,b,c,d=cross(i,following,j),cross(i,following,end),cross(j,end,i),cross(j,end,following)
                crossing=((a>epsilon and b<-epsilon) or (a<-epsilon and b>epsilon)) and ((c>epsilon and d<-epsilon) or (c<-epsilon and d>epsilon))
                if crossing or on(i,following,j) or on(i,following,end) or on(j,end,i) or on(j,end,following):
                    raise ValueError('OBJ self-intersecting polygon')
        area=sum(p[i][u]*p[(i+1)%count][v]-p[(i+1)%count][u]*p[i][v] for i in range(count))
        if abs(area)<=epsilon:raise ValueError('OBJ zero polygon area')
        direction=1 if area>0 else -1
        ring=list(range(count))
        while len(ring)>3:
            for i,b in enumerate(ring):
                a,c=ring[i-1],ring[(i+1)%len(ring)]
                if direction*cross(a,b,c)<=epsilon:continue
                if any(direction*cross(a,b,o)>=-epsilon and direction*cross(b,c,o)>=-epsilon and direction*cross(c,a,o)>=-epsilon for o in ring if o not in (a,b,c)):continue
                result.append((face_index,tuple(corners[k] for k in (a,b,c))))
                del ring[i]
                break
            else:raise ValueError('OBJ polygon cannot be triangulated')
        if direction*cross(*ring)<=epsilon:raise ValueError('OBJ degenerate final triangle')
        result.append((face_index,tuple(corners[k] for k in ring)))
    return result
