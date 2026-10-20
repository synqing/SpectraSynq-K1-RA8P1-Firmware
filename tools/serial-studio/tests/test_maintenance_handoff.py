"""Actual Unix IPC and deployment consumers; only target replies are synthetic."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import socket
import struct
import tempfile
import time

import pytest
import titan_control_server as server
from test_native_broker_channel import admit, bound, identity_body, health_body, BUILD, UID

HELPER = Path('/Users/spectrasynq/Workspace_Management/Software/agent-skills/skills/smc-titan-mixer/scripts/deploy_admission.py')
spec=importlib.util.spec_from_file_location('maintenance_admission',HELPER)
helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)


class Client:
    def __init__(self,path):
        self.sock=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM);self.sock.settimeout(2);self.sock.connect(str(path))
        self.file=self.sock.makefile('rwb')
    def call(self,row):
        self.file.write((json.dumps(row)+'\n').encode());self.file.flush()
        return json.loads(self.file.readline())
    def close(self):self.file.close();self.sock.close()


def released(plane):
    end=time.monotonic()+1
    while plane.maintenance_id is not None and time.monotonic()<end:time.sleep(.001)
    assert plane.maintenance_id is None


def test_persistent_controller_and_real_deployment_consumers(tmp_path,monkeypatch):
    admit(tmp_path,monkeypatch)
    lease=tmp_path/'owner.json'
    monkeypatch.setattr(server,'OWNER_LEASE_PATH',lease)
    monkeypatch.setattr(helper,'OWNER_LEASE_PATH',lease)
    count=[0];calls=[]
    def transact(op,payload):
        calls.append(op)
        if op==server.NATIVE_OPCODES['native_identity']:
            row=json.loads(identity_body());row.update(source_id=BUILD,core_commit='source')
            body=json.dumps(row).encode()
        elif op==server.NATIVE_OPCODES['native_health']:
            count[0]+=1;body=health_body(count[0],2000000)
        elif op in (server.NATIVE_OPCODES['native_status'],server.BIND_OPCODE):
            body=struct.pack('<4I',1,7,1,1)+b'\0'*96
        elif op==server.NATIVE_OPCODES['native_global']:
            body=b''
        else:raise AssertionError(op)
        return {'ok':True,'status':0,'body':body}
    plane=server.RestrictedControlPlane(transact=transact,evidence=tmp_path,bound=bound())
    with tempfile.TemporaryDirectory(prefix='tmh-',dir='/tmp') as directory:
        broker=server.ControlServer(plane,Path(directory)/'control.sock');broker.start()
        controller=Client(broker.path)
        try:
            hello=controller.call({'op':'hello'});token=hello['session']
            other=Client(broker.path)
            assert other.call({'op':'hello'})['error']=='writer_busy';other.close()
            lease.write_text(json.dumps({'pid':os.getpid(),'task':'offline-test'}))
            assert helper.broker_hello_fields()['maintenance']['owner_lease_sha256']==hashlib.sha256(lease.read_bytes()).hexdigest()
            # The actual client crosses the actual server and native wire decoder.
            ready=helper.query_broker(str(broker.path),UID,BUILD,2,'native')
            assert ready['protocol']=='native' and ready['broker_status']['boot']==7
            released(plane)
            assert controller.call({'op':'native_global','session':token})['error']=='maintenance_hold'
            settings=helper.SettingsConnection(str(broker.path),dict(uid=UID,build=BUILD,protocol='native',schema='replacement-health-v1'))
            assert len(settings.states()['scenes'])==2
            reply=settings.call('native_global',boot=7,brightness=100,output_limit=.5,expected_revision=0)
            assert reply['ok'] and reply['saved'] is False
            settings.close();released(plane)
            assert controller.call({'op':'hello'})['session']==token
            assert controller.call({'op':'native_global','session':token})['error']=='maintenance_hold'
            observer=controller.call({'op':'native_status','session':token})
            assert observer['ok'] and observer['boot']==7
            wrong=Client(broker.path)
            assert wrong.call({'op':'hello','maintenance':{'owner_lease_sha256':'0'*64}})['error']=='maintenance_lease_invalid'
            wrong.close()
            maintenance=Client(broker.path)
            mh=maintenance.call({'op':'hello',**helper.broker_hello_fields()})
            assert mh['ok'] and mh['session']!=token
            lease.unlink()
            denied=maintenance.call({'op':'native_status','session':mh['session']})
            assert denied['error']=='maintenance_lease_revoked'
            maintenance.close();released(plane)
            normal=controller.call({'op':'native_global','session':token,'boot':7,'brightness':100,'output_limit':.5,'expected_revision':0})
            assert normal['ok']
        finally:
            controller.close();broker.close()
    assert server.NATIVE_OPCODES['native_global'] in calls


def test_wrong_owner_ancestor_never_autoattaches(tmp_path,monkeypatch):
    lease=tmp_path/'owner.json';lease.write_text(json.dumps({'pid':99999999}))
    monkeypatch.setattr(helper,'OWNER_LEASE_PATH',lease)
    monkeypatch.setattr(helper.os,'getppid',lambda:1)
    assert helper.broker_hello_fields()=={}


def test_dead_owner_and_arbitrary_path_do_not_grant_maintenance(tmp_path,monkeypatch):
    lease=tmp_path/'owner.json';lease.write_text(json.dumps({'pid':99999999}))
    monkeypatch.setattr(server,'OWNER_LEASE_PATH',lease)
    def dead(pid, signal): raise ProcessLookupError('test owner absent')
    monkeypatch.setattr(server.os,'kill',dead)
    plane=server.RestrictedControlPlane(bound=bound())
    proof={'owner_lease_sha256':hashlib.sha256(lease.read_bytes()).hexdigest()}
    assert plane.handle({'op':'hello','maintenance':proof},conn_id='test')['error']=='maintenance_lease_invalid'
    assert plane.handle({'op':'hello','maintenance':dict(proof,path='/another-owner')},conn_id='test')['error']=='maintenance_lease_invalid'
