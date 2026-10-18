from pathlib import Path
import json
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
body=region(0x100d6070,0x100d611c)
Path('outputs/sfm64-cvar-startup-registration.json').write_text(json.dumps(dict(sha256=sha,body=body,finding='Guard nonnullCvarprovider/unregistered; store suppliedflags1019fcf0 andregisteredtrue BEFOREprovider slot8 handle allocation. Savehandle1017b7f0. Store suppliedaccesspointer1019fcec or fallback1017b868. Walk pendinghead1019fce8; eachnode captures flags and node+4 next BEFOREslot3(flags), thenreloadsnodevtableforslot11 andcontinuescapturednext. Reloadproviderforslot31 thenagainforslot40. Clearpendinghead atcompletion. No rollback or returnvalue.',limits='Fresh static bytes only; node/accessobject/provider ABI and runtime methods unverified.'),indent=2)+'\n')
print('verified_cvar_startup_registration_bytes='+str(body['bytes']))
