"""Cross-check an inventory against dumpbin; inspected binaries are never executed."""
import argparse
import json
from pathlib import Path
import re
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('inventory', type=Path)
    parser.add_argument('dumpbin', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    data = json.loads(args.inventory.read_text(encoding='utf-8'))
    checks = []
    modules = [(Path(data['root']) / label, row) for label, row in data['modules'].items() if row]
    if data.get('owned_editor'):
        modules.append((Path(data['owned_editor']['path']), data['owned_editor']))
    for path, row in modules:
        result = subprocess.run([str(args.dumpbin), '/dependents', '/headers', str(path)],
                                capture_output=True, text=True, check=True)
        actual = {name.lower() for name in re.findall(r'^    ([\w.\-]+\.(?:dll|drv))\s*$', result.stdout, re.M | re.I)}
        expected = {name.lower() for name in row['imports'] + row['delay_imports']}
        architecture = bool(re.search(r'\b' + (r'8664 machine \(x64\)' if row['architecture'] == 'x64'
                                               else r'14C machine \(x86\)'), result.stdout, re.I))
        checks.append(dict(path=str(path), imports_match=actual == expected, architecture_match=architecture,
                           missing=sorted(expected - actual), unexpected=sorted(actual - expected)))
    passed = bool(checks) and all(c['imports_match'] and c['architecture_match'] for c in checks)
    args.output.write_text(json.dumps(dict(passed=passed, checks=checks), indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(passed=passed, modules_checked=len(checks))))
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
