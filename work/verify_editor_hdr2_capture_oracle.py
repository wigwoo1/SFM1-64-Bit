"""Independent literal RGB checks on captures from the actual owned editor."""
from pathlib import Path
import hashlib,json,sys
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(x,s):
    if not x:raise RuntimeError(s)
def main():
    need(len(sys.argv)==2,'supply a completed editor-route results.json')
    receipt=Path(sys.argv[1]);r=json.loads(receipt.read_text());need(r['status'].startswith('VERIFIED explicit experimental HDR2 editor route'),'verified route prerequisite')
    run=receipt.parent;clear=(13,13,20,255);metrics={}
    # The authored fixture has white base texels, constant red/green texlights,
    # style .5, instance(.5,.75,1), exposure1/2. They imply RGB(.25,0,0) and
    # (0,.375,0), then(.5,0,0)/(0,.75,0). UNORM quantization permits one LSB.
    expected={'half-nearest':[(64,0,0,255),(0,96,0,255)],
              'half-exposure2':[(128,0,0,255),(0,191,0,255)],
              'numeric-linear':[(0,0,0,255)],'all-unadmitted':[(255,255,255,255)]}
    masks={}
    for label,colors in expected.items():
        files=sorted((run/('captures-'+label)).glob('*.rgba'));need(files,'missing captures '+label)
        for p in files:
            b=p.read_bytes();need(sha(p)==r['editor_runs'][label]['captures'][p.name],'capture hash changed');need(len(b)==512*512*4,'owned capture dimensions')
            pixels=list(zip(*[iter(b)]*4));surface=[];seen=set();maximum=0
            for n,pixel in enumerate(pixels):
                if pixel==clear:continue
                options=[max(abs(a-c) for a,c in zip(pixel,want)) for want in colors];best=min(options)
                need(best<=1,'literal RGB mismatch '+label+' pixel'+str(n)+': '+str(pixel));maximum=max(maximum,best);seen.add(options.index(best));surface.append(n)
            need(surface and len(seen)==len(colors),'nonempty red/green/other coverage '+label)
            if label=='half-nearest':masks[p.name]=surface
            else:need(surface==masks[p.name],'changed geometry coverage '+label)
            metrics[label+'/'+p.name]=dict(pixels=len(pixels),lit_pixels=len(surface),max_error_lsb=maximum,sha256=sha(p))
    blank=run/'captures-blank-restore/vulkan-retained-frame-2.rgba';b=blank.read_bytes();need(sha(blank)==r['editor_runs']['blank-restore']['captures'][blank.name],'gap capture hash')
    need(b==bytes(clear)*(512*512),'gap retained pixels')
    out=run/'capture-oracle.json';result=dict(status='VERIFIED actual editor fixture RGB, exposure and gap clear',receipt_sha256=sha(receipt),
        implementation_editor_sha256=r['editor']['sha256'],oracle_source_sha256=sha(Path(__file__)),metrics=metrics,blank_pixels=512*512,
        limits=['Literal values and cross-mode coverage, not an independent rasterizer or native SFM visual oracle.','The separate real GPU probe checks full independently predicted spatial images.'])
    out.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(status=result['status'],checked_editor_pixels=sum(v['pixels'] for v in metrics.values())+512*512,receipt=str(out),sha256=sha(out))))
if __name__=='__main__':main()
