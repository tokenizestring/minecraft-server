import os, sys, time, uuid, copy
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path[:0] = [os.path.join(HERE, '..', d) for d in ('probes', 'decode')]
import bedrock_players_codec as codec

CAPTURES = os.path.join(HERE, '..', 'captures')
OUT = os.path.join(CAPTURES, 'inject')
REFERENCE = os.path.join(CAPTURES, 'capture_bedrock_tp_watcher')

def capture_ours():
    import clients
    java = clients.Java('JavaGuy', 777); java.join(); time.sleep(0.5)
    bedrock = clients.Bedrock('InjectProbe'); bedrock.join(); time.sleep(2)
    lists = [x for x in bedrock.packets(63) if codec.read_list(x)['name'] == b'JavaGuy']
    adds = [x for x in bedrock.packets(12) if codec.read_add(x)['name'] == b'JavaGuy']
    java.close(); bedrock.close()
    return lists[0], adds[0]

def ids(e, a, step, name):
    fresh = uuid.uuid4().bytes
    e['uuid'] = a['uuid'] = fresh
    e['unique'] = a['raw_id'] = a['runtime'] = 1000 + step
    e['name'] = a['name'] = name
    a['data'] = [(k, c, t, name if k == 4 else v) for k, c, t, v in a['data']]

def main():
    ours_list, ours_add = capture_ours()
    bds_list = codec.read_list(open(os.path.join(REFERENCE, '554_63.bin'), 'rb').read())
    bds_add = codec.read_add(open(os.path.join(REFERENCE, '634_12.bin'), 'rb').read())
    ours_e = codec.read_list(ours_list); ours_a = codec.read_add(ours_add)
    steps = []
    steps.append(('official bytes verbatim', copy.deepcopy(bds_list), copy.deepcopy(bds_add)))
    e, a = copy.deepcopy(bds_list), copy.deepcopy(bds_add); ids(e, a, 1, b'Step1')
    steps.append(('our id style: new uuid, small unique/runtime ids, name', e, a))
    e, a = copy.deepcopy(e), copy.deepcopy(a)
    e['color'] = ours_e['color']; e['build'] = ours_e['build']; a['platform'] = ours_a['platform']; a['device'] = ours_a['device']
    e['skin']['flags'] = ours_e['skin']['flags']; e['skin']['profile_hash'] = ours_e['skin']['profile_hash']; ids(e, a, 2, b'Step2')
    steps.append(('our colour, platform, device id, premium flag, profile hash', e, a))
    e, a = copy.deepcopy(e), copy.deepcopy(a)
    e['skin'] = copy.deepcopy(ours_e['skin']); ids(e, a, 3, b'Step3')
    steps.append(('our generated skin', e, a))
    e, a = copy.deepcopy(e), copy.deepcopy(a)
    a['motion'] = ours_a['motion']; a['data'] = ours_a['data']; a['abilities'] = ours_a['abilities']; a['item'] = ours_a['item']; a['gametype'] = ours_a['gametype']; ids(e, a, 4, b'Step4')
    steps.append(('our addplayer body: position, actor data, abilities', e, a))
    e, a = copy.deepcopy(ours_e), copy.deepcopy(ours_a); ids(e, a, 5, b'Step5')
    steps.append(('our exact packets with fresh ids', e, a))
    steps.append(('our exact packets verbatim (JavaGuy ids)', ours_e, ours_a))
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, 'list.txt'), 'w', newline='\n') as listing, open(os.path.join(OUT, 'steps.txt'), 'w', newline='\n') as legend:
        for index, (label, e, a) in enumerate(steps):
            for pid, body in ((63, codec.write_list(e)), (12, codec.write_add(a))):
                name = 'step%d_%d.bin' % (index, pid)
                open(os.path.join(OUT, name), 'wb').write(body)
                listing.write('%d %d %s\n' % (index, pid, name))
            legend.write('step %d: %s\n' % (index, label))
            print('step %d: %s' % (index, label))
    assert codec.write_list(ours_e) == ours_list and codec.write_add(ours_a) == ours_add

main()
