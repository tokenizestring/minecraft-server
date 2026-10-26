import json, os, sys

DOCS = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'reference', 'bedrock_docs')
cache = {}

def load(name):
    name = name.replace('./', '').split('#')[0]
    if name not in cache:
        cache[name] = json.load(open(os.path.join(DOCS, name)))
    return cache[name]

def extras(node):
    keep = []
    for key in ('x-underlying-type', 'x-serialization-options', 'x-dependency-property', 'x-dependency-value', 'x-control-value-type'):
        if key in node:
            keep.append('%s=%s' % (key[2:], node[key]))
    if 'enum' in node and len(node['enum']) <= 12:
        keep.append('enum=%s' % node['enum'])
    if 'x-enumerator-name' in node or 'x-enumerator-names' in node:
        keep.append('names=%s' % node.get('x-enumerator-names', node.get('x-enumerator-name')))
    return ' '.join(keep)

def show(node, indent, seen):
    pad = '  ' * indent
    if '$ref' in node:
        ref = node['$ref']
        target = load(ref)
        title = target.get('title', ref)
        print('%s-> %s %s' % (pad, title, extras(node)))
        if title in seen:
            print(pad + '   (recursive)')
            return
        show(target, indent + 1, seen | {title})
        return
    if node.get('type') == 'object' and 'properties' in node:
        props = sorted(node['properties'].items(), key=lambda kv: kv[1].get('x-ordinal-index', 999))
        for name, prop in props:
            print('%s[%s] %s : %s %s' % (pad, prop.get('x-ordinal-index', '?'), name, prop.get('type', ''), extras(prop)))
            show(prop, indent + 1, seen)
    if node.get('type') == 'array' and 'items' in node:
        print('%s items: %s' % (pad, extras(node['items'])))
        show(node['items'], indent + 1, seen)
    for key in ('oneOf', 'anyOf', 'allOf'):
        for index, option in enumerate(node.get(key, [])):
            print('%s%s[%d] %s' % (pad, key, index, extras(option)))
            show(option, indent + 1, seen)
    if 'enum' in node and len(node['enum']) > 12:
        print('%s enum (%d values) %s ...' % (pad, len(node['enum']), node['enum'][:8]))
    for key in ('if', 'then', 'else'):
        if key in node:
            print('%s%s:' % (pad, key))
            show(node[key], indent + 1, seen)

for name in sys.argv[1:]:
    root = load(name + '.json')
    meta = root.get('$metaProperties', {})
    print('=== %s id=%s' % (name, meta.get('[cereal:packet]')))
    show(root, 0, {name})
