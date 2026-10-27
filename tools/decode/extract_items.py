import json, os

ROOT = os.path.dirname(os.path.abspath(__file__))

for tag in ('26_2', '26_3'):
    registries = json.load(open(os.path.join(ROOT, '..', 'reference', 'gen_' + tag, 'reports', 'registries.json')))
    entries = registries['minecraft:item']['entries']
    names = [name for name, entry in sorted(entries.items(), key=lambda item: item[1]['protocol_id'])]
    assert [entries[name]['protocol_id'] for name in names] == list(range(len(names)))
    path = os.path.join(ROOT, '..', '..', 'data', 'java', 'items_%s.txt' % tag)
    with open(path, 'w', newline='\n') as out:
        out.write('\n'.join(names) + '\n')
    print(tag, len(names), 'items ->', path)
