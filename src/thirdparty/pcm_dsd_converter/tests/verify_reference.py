"""Integration tests; numpy/scipy/numba needed only for reference validation."""
import argparse
import ctypes as C
import importlib.util
from pathlib import Path
import sys
import time
import subprocess
import tempfile
import numpy as np

class Config(C.Structure):
    _fields_=[('struct_size',C.c_uint32),('input_sample_rate',C.c_uint32),('channels',C.c_uint32),
              ('dsd_multiplier',C.c_uint32),('input_gain',C.c_double),('dither_amplitude',C.c_double),('seed',C.c_uint64)]

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--reference',type=Path,required=True)
    p.add_argument('--dll',type=Path,default=Path(__file__).resolve().parents[1]/'build/Release/pcm_dsd_converter.dll')
    args=p.parse_args()
    spec=importlib.util.spec_from_file_location('reference_core',args.reference/'dsd15_core.py')
    ref=importlib.util.module_from_spec(spec); sys.modules[spec.name]=ref; spec.loader.exec_module(ref)
    dll=C.CDLL(str(args.dll.resolve()))
    dll.pcm_dsd_default_config.argtypes=[C.POINTER(Config)]
    dll.pcm_dsd_create.argtypes=[C.POINTER(Config),C.POINTER(C.c_void_p)]
    dll.pcm_dsd_destroy.argtypes=[C.c_void_p]
    dll.pcm_dsd_reset.argtypes=[C.c_void_p]
    dll.pcm_dsd_process.argtypes=[C.c_void_p,C.c_void_p,C.c_size_t,C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t),C.POINTER(C.c_size_t)]
    dll.pcm_dsd_flush.argtypes=[C.c_void_p,C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
    dll.dsd15_modulate_pack.argtypes=[C.c_void_p,C.c_longlong,C.c_void_p,C.c_double,C.c_double,
        C.POINTER(C.c_uint64),C.c_void_p,C.POINTER(C.c_double),C.POINTER(C.c_uint8),
        C.POINTER(C.c_longlong),C.POINTER(C.c_double),C.POINTER(C.c_int32),C.POINTER(C.c_int32),
        C.c_void_p,C.c_longlong,C.POINTER(C.c_longlong)]
    ref.load_native_core=lambda: dll
    profile=ref.load_dc15_profile()
    generator=np.random.default_rng(42)
    for amplitude in (0.,2.**-24):
        for signal in (np.zeros(10001), generator.uniform(-1,1,10001), np.sin(np.arange(10001)*.003),np.full(10001,4.)):
            native=ref.make_modulator_state(profile,ref.DEFAULT_SEED)
            baseline=ref.make_modulator_state(profile,ref.DEFAULT_SEED)
            nb=nc=pb=pc=0
            for index,block in enumerate(np.array_split(signal,23)):
                actual,n,nb,nc=ref.modulate_mono_dc15_pack_native(block,profile.sos,.5,amplitude,native,nb,nc)
                result=ref._modulate_mono_dc15_pack_state_numba(block,profile.sos,.5,amplitude,
                    np.uint64(baseline.rng),baseline.states,baseline.delayed_feedback,np.uint8(baseline.prev_bit),
                    baseline.run,baseline.last_return,pb,pc)
                expected,count,baseline.rng,baseline.delayed_feedback,baseline.prev_bit,baseline.run,baseline.last_return,pb,pc=result
                assert n==count and np.array_equal(actual[:n],expected[:count]), (amplitude,index,signal[:3],native.states,baseline.states)
                assert nb==pb and nc==pc
                assert np.array_equal(native.states,baseline.states)
                assert native.delayed_feedback==baseline.delayed_feedback and native.rng==baseline.rng
    print('PASS: native modulator bytes and states exactly match Numba (8 signals, 23 chunks each)',flush=True)
    with tempfile.TemporaryDirectory() as tmp:
        path=Path(tmp)/'upsampled.bin'
        subprocess.run([str(args.dll.resolve().parent/'pcm_dsd_dsp_test.exe'),str(path)],check=True)
        actual=np.fromfile(path,np.float64).reshape(-1,2)
        t=np.arange(9001,dtype=np.float64)
        reference=ref.legacy_fir_upsample_stereo(np.column_stack((.8*np.sin(t*.031),.5*np.cos(t*.073))),64)
        error=np.max(np.abs(actual-reference))
        assert error<1e-10,error
        print(f'PASS: 9001-frame DSD64 FIR waveform vs original SciPy pipeline: max error {error:.3g}',flush=True)

    def convert(pcm,rate=44100,mult=64,chunk=4096,capacity=4096):
        pcm=np.ascontiguousarray(pcm,dtype=np.float64)
        cfg=Config(); dll.pcm_dsd_default_config(C.byref(cfg))
        cfg.input_sample_rate=rate; cfg.dsd_multiplier=mult; cfg.channels=pcm.shape[1]
        handle=C.c_void_p(); assert dll.pcm_dsd_create(C.byref(cfg),C.byref(handle))==0
        out=(C.c_uint8*capacity)(); used=C.c_size_t(); written=C.c_size_t(); parts=[]; pos=0
        try:
            while pos<len(pcm):
                count=min(chunk,len(pcm)-pos)
                assert dll.pcm_dsd_process(handle,pcm.ctypes.data+pos*pcm.shape[1]*8,count,out,capacity,C.byref(used),C.byref(written))==0
                assert used.value or written.value
                pos+=used.value; parts.append(bytes(out[:written.value]))
            while True:
                status=dll.pcm_dsd_flush(handle,out,capacity,C.byref(written))
                parts.append(bytes(out[:written.value]))
                if status==3: break
                assert status==0 and written.value
            assert dll.pcm_dsd_flush(handle,out,capacity,C.byref(written))==3 and written.value==0
            assert dll.pcm_dsd_process(handle,None,0,out,capacity,C.byref(used),C.byref(written))==4
            data=b''.join(parts)
            base=44100 if rate%44100==0 else 48000
            assert len(data)==((len(pcm)*(base*mult//rate)+7)//8)*2
            return data
        finally: dll.pcm_dsd_destroy(handle)

    pcm=generator.uniform(-.8,.8,(777,2))
    start=time.perf_counter()
    whole=convert(pcm)
    assert whole==convert(pcm,chunk=7,capacity=3)
    assert whole==convert(pcm,chunk=257,capacity=510)
    print('PASS: caller chunk and tiny output-buffer invariance',flush=True)
    for rate in (44100,48000):
        for mult in (16,32,64,128,256,512,1024,2048):
            data=convert(np.zeros((9,1)),rate,mult)
            packed=np.frombuffer(data,np.uint8).reshape(-1,2)
            assert np.array_equal(packed[:,0],packed[:,1])
        print(f'PASS: {rate} family DSD16..2048, mono duplication and output lengths',flush=True)
    for frames in (0,1,7,8,255,256,257):
        data=convert(np.zeros((frames,2)),rate=705600,mult=16)
        if frames%8:
            assert data[-1]&((1<<(8-frames%8))-1)==0
    # At up=1, compare the complete stream to the original modulator.
    data=convert(pcm,rate=2822400,mult=64)
    bits=[]
    for ch in range(2):
        state=ref.make_modulator_state(profile,ref.DEFAULT_SEED)
        result=ref._modulate_mono_dc15_pack_state_numba(pcm[:,ch],profile.sos,.5,2.**-24,
            state.rng,state.states,0.,np.uint8(0),0,0.,0,0)
        packed,count,*_=result
        b=bytes(packed[:count]); carry,count_bits=result[-2:]
        if count_bits: b+=bytes([carry<<(8-count_bits)])
        bits.append(np.frombuffer(b,np.uint8))
    assert data==np.column_stack(bits).tobytes()
    print('PASS: unity-ratio end-to-end byte equality, EOF/padding boundaries',flush=True)
    print(f'Integration tests finished in {time.perf_counter()-start:.2f}s',flush=True)

if __name__=='__main__': main()
