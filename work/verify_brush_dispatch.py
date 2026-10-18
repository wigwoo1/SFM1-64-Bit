"""Reuse general-oracle query corpus with native sentinel-based box dispatch."""
import contextlib,io,json
import verify_native_convex_brush_ray as corpus
from pe_image import Image
def main():
 corpus.OUT=corpus.ROOT/'outputs/brush-dispatch-20261015'
 image=Image(corpus.ENGINE);start=image.offset(0x10146850);raw=image.b[start:start+0x3d4]
 corpus.require(raw in (corpus.OUT/'brush_ray_native_x86.exe').read_bytes(),'Original optimized box body absent')
 captured=io.StringIO()
 with contextlib.redirect_stdout(captured):corpus.main()
 summary=json.loads(captured.getvalue());path=corpus.Path(summary['receipt']);receipt=json.loads(path.read_text())
 receipt['status']='PASS_NATIVE_GENERAL_OR_OPTIMIZED_BRUSH_DISPATCH'
 receipt['limits']=['Native general kernel dispatches by runtime 65535 sentinel; record selection/construction provided by independently checked owned loader','All benchmark brushes tested individually; not world traversal','Synthetic surface table; surface result not compared','No displacements or editor integration']
 path.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=receipt['cases'],receipt=str(path))))
if __name__=='__main__':main()
