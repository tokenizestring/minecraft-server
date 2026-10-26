import asyncio, struct, time, sys, statistics

HOST = '127.0.0.1'
PORT = 25565
BOTS = int(sys.argv[1]) if len(sys.argv) > 1 else 100
SECONDS = float(sys.argv[2]) if len(sys.argv) > 2 else 20.0
PROTOCOL = 777
POSITION_OUT = 0x49
KEEP_ALIVE_OUT = 0x2D
KEEP_ALIVE_IN = 0x1C
MOVE_IN = 0x1E

def varint(v):
    v &= 0xFFFFFFFF
    out = bytearray()
    while True:
        b = v & 0x7F
        v >>= 7
        if v:
            out.append(b | 0x80)
        else:
            out.append(b)
            return bytes(out)

def frame(pid, body=b''):
    p = varint(pid) + body
    return varint(len(p)) + p

def mstr(t):
    t = t.encode()
    return varint(len(t)) + t

async def read_packet(reader):
    length = shift = 0
    while True:
        b = (await reader.readexactly(1))[0]
        length |= (b & 0x7F) << shift
        shift += 7
        if not b & 0x80:
            break
    data = await reader.readexactly(length)
    pid = shift = 0
    i = 0
    while True:
        b = data[i]; i += 1
        pid |= (b & 0x7F) << shift
        shift += 7
        if not b & 0x80:
            break
    return pid, data[i:], length

stats = dict(joined=0, failed=0, bytes_in=0, packets_out=0, keep_alives=0)
join_times = []

async def bot(index, start_gate):
    await start_gate.wait()
    began = time.perf_counter()
    try:
        reader, writer = await asyncio.open_connection(HOST, PORT)
        writer.write(frame(0, varint(PROTOCOL) + mstr('localhost') + struct.pack('>H', PORT) + varint(2)))
        writer.write(frame(0, mstr('bot%d' % index) + index.to_bytes(16, 'big')))
        await writer.drain()
        state = 'login'
        deadline = began + SECONDS + 30
        joined_at = None
        mover = None
        while time.perf_counter() < deadline:
            pid, body, size = await asyncio.wait_for(read_packet(reader), timeout=40)
            stats['bytes_in'] += size
            if state == 'login' and pid == 2:
                writer.write(frame(3)); state = 'config'
            elif state == 'config' and pid == 3:
                writer.write(frame(3)); state = 'play'
            elif state == 'play' and pid == POSITION_OUT and joined_at is None:
                joined_at = time.perf_counter()
                join_times.append(joined_at - began)
                stats['joined'] += 1
                writer.write(frame(0x00, varint(1)))
                mover = asyncio.ensure_future(move(writer, joined_at))
            elif state == 'play' and pid == KEEP_ALIVE_OUT:
                writer.write(frame(KEEP_ALIVE_IN, body)); stats['keep_alives'] += 1
            elif pid == 0 and state == 'login':
                raise RuntimeError('kicked')
            if mover and mover.done():
                break
            await writer.drain()
        writer.close()
    except Exception as e:
        stats['failed'] += 1
        if stats['failed'] <= 3:
            print('bot', index, 'failed:', repr(e))

async def move(writer, joined_at):
    x = 8.5
    while time.perf_counter() - joined_at < SECONDS:
        x += 0.01
        writer.write(frame(MOVE_IN, struct.pack('>dddB', x, 65.0, 8.5, 1)))
        stats['packets_out'] += 1
        await writer.drain()
        await asyncio.sleep(0.05)

async def main():
    gate = asyncio.Event()
    tasks = [asyncio.ensure_future(bot(i, gate)) for i in range(BOTS)]
    began = time.perf_counter()
    gate.set()
    await asyncio.gather(*tasks)
    total = time.perf_counter() - began
    joins = sorted(join_times)
    if joins:
        print('bots %d joined %d failed %d' % (BOTS, stats['joined'], stats['failed']))
        print('join latency ms: median %.1f  p95 %.1f  max %.1f' % (statistics.median(joins) * 1000, joins[int(len(joins) * 0.95) - 1] * 1000, joins[-1] * 1000))
        print('all joined within %.2f s' % joins[-1])
    print('received %.1f MB, sent %d movement packets (%.0f/s), keep-alives %d, wall %.1f s' % (stats['bytes_in'] / 1048576, stats['packets_out'], stats['packets_out'] / max(SECONDS, 1), stats['keep_alives'], total))

asyncio.run(main())
