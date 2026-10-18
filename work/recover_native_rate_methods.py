from pathlib import Path
text=Path('work/recover_native_time_methods.py').read_text()
text=text.replace("['DmeTime_t_GetSeconds','DmeTime_t_GetTicks','DmeFramerate_t_GetSecondsPerFrame','new_DmeTime_t']", "['new_DmeFramerate_t','DmeFramerate_t_SetFramerate','DmeFramerate_t_SetFramerateNTSC']")
text=text.replace('sfm64-native-time-method-addresses.json','sfm64-native-rate-method-addresses.json')
exec(compile(text,'native_rate_method_scan','exec'))
