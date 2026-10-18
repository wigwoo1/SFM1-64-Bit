from pathlib import Path
text=Path('work/recover_native_camera_method_tables.py').read_text()
start=text.index("for filename,names in ");end=text.index(':\n im=Image',start)
text=text[:start]+"for filename,names in [('valve_pyd.pyd',['DmeTime_t_GetSeconds','DmeTime_t_GetTicks','DmeFramerate_t_GetSecondsPerFrame','new_DmeTime_t'])]"+text[end:]
text=text.replace('sfm64-native-camera-method-addresses.json','sfm64-native-time-method-addresses.json').replace("if len(records)<5:","if len(records)<3:").replace('expected camera method-table entries missing','expected time method-table entries missing')
exec(compile(text,'native_time_method_scan','exec'))
