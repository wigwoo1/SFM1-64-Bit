from pathlib import Path
p=Path('PROJECT_REQUIREMENTS.md')
s=p.read_bytes().decode('utf-8','surrogateescape')
s=''.join(bytes([ord(c)-0xdc00]).decode('cp1252') if 0xdc80<=ord(c)<=0xdcff else c for c in s)
p.write_bytes(s.encode('utf-8'))
p.read_bytes().decode('utf-8')
