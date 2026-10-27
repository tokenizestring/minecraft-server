import json, os

ROOT = os.path.dirname(os.path.abspath(__file__))

for tag in ('26_2', '26_3'):
    blocks = json.load(open(os.path.join(ROOT, '..', 'reference', 'gen_' + tag, 'reports', 'blocks.json')))
    lines = []
    for name, block in blocks.items():
        default = [state['id'] for state in block['states'] if state.get('default')]
        lines.append('%s %d' % (name, default[0] if default else block['states'][0]['id']))
    path = os.path.join(ROOT, '..', '..', 'data', 'java', 'blocks_%s.txt' % tag)
    with open(path, 'w', newline='\n') as out:
        out.write('\n'.join(lines) + '\n')
    print(tag, len(lines), 'blocks ->', path)
