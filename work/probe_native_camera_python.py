import sys,os
root=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game'
os.environ['PATH']=root+r'\bin;'+root+r'\bin\tools;'+os.environ.get('PATH','')
sys.path.insert(0,root+r'\sdktools\python\2.7\win32\Lib\site-packages\vs')
gLoadDataModel=False
import vs
print('native_vs_imported',hasattr(vs,'ComputeProjectionMatrix'))
