"""Lossless compact AP/VP values; schema is independently emitted by the donor."""
import math
import struct


def read_schema(text):
    rows=[line.split('=') for line in text.splitlines() if line!='END']
    if len(rows)!=526 or any(len(row)!=2 or row[1] not in ('f','i') for row in rows):
        raise ValueError('invalid frozen schema')
    names=[row[0] for row in rows]
    if len(set(names))!=526 or not all(f'pixel[{i}]' in names for i in range(320)):
        raise ValueError('duplicate/incomplete schema')
    return rows


def decode(body, schema):
    offset=0; lines=[]
    for name,kind in schema:
        if offset>=len(body): raise ValueError('truncated binary trace')
        tag=chr(body[offset]); offset+=1
        if kind=='f' and tag=='f':
            size=4
            if offset+size>len(body): raise ValueError('truncated float')
            value=struct.unpack_from('<f',body,offset)[0]
            if not math.isfinite(value): raise ValueError('non-finite binary trace')
            value=format(value,'.17g')
        elif kind=='i' and tag in '12348':
            size=int(tag)
            if offset+size>len(body): raise ValueError('truncated integer')
            value=str(int.from_bytes(body[offset:offset+size],'little'))
        else: raise ValueError('binary trace type mismatch')
        offset+=size; lines.append(name+'='+value+'\n')
    if offset!=len(body): raise ValueError('extra binary trace fields')
    return ''.join(lines).encode()
