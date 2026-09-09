from pathlib import Path
# Independent integer fixture generator. Keep raw bytes as oracle, not converter output.
import wave,subprocess
f=Path(__file__).parent/'fixtures';f.mkdir(exist_ok=True)
for bits in (16,24,32):
 seed=0x12345678;data=bytearray()
 for i in range(44100*2+34):
  seed=(seed*1664525+1013904223)&0xffffffff
  v=[0,1,-1,-(1<<(bits-1)),(1<<(bits-1))-1][i%5] if i<20 else (seed&((1<<bits)-1))-(1<<(bits-1))
  data+=v.to_bytes(bits//8,'little',signed=True)
 (f/f'pcm{bits}.raw').write_bytes(data)
 with wave.open(str(f/f'pcm{bits}.wav'),'wb') as w:
  w.setparams((2,bits//8,44100,0,'NONE','not compressed'));w.writeframes(data)
 if bits != 32:
  subprocess.run(['ffmpeg','-y','-v','error','-i',str(f/f'pcm{bits}.wav'),'-c:a','flac',str(f/f'pcm{bits}.flac')],check=True)
 else:
  # FFmpeg's FLAC encoder may silently reduce s32 to 24 bits; use libFLAC for the 32-bit fixture.
  import ctypes, os, ctypes.util
  component = Path(__file__).resolve().parents[1]/'src/xamp/x64/Release/components'
  dll_search = os.add_dll_directory(str(component)) if os.name == 'nt' else None
  lib = ctypes.CDLL(str(component/'FLAC.dll') if os.name == 'nt' else ctypes.util.find_library('FLAC'))
  lib.FLAC__stream_encoder_new.restype = ctypes.c_void_p
  for name in ('set_channels','set_bits_per_sample','set_sample_rate'):
   fn=getattr(lib,'FLAC__stream_encoder_'+name);fn.argtypes=[ctypes.c_void_p,ctypes.c_uint];fn.restype=ctypes.c_int
  lib.FLAC__stream_encoder_init_file.argtypes=[ctypes.c_void_p,ctypes.c_char_p,ctypes.c_void_p,ctypes.c_void_p]
  lib.FLAC__stream_encoder_process_interleaved.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_int32),ctypes.c_uint]
  lib.FLAC__stream_encoder_finish.argtypes=[ctypes.c_void_p]
  lib.FLAC__stream_encoder_delete.argtypes=[ctypes.c_void_p]
  encoder=lib.FLAC__stream_encoder_new()
  try:
   assert lib.FLAC__stream_encoder_set_channels(encoder,2)
   assert lib.FLAC__stream_encoder_set_bits_per_sample(encoder,32)
   assert lib.FLAC__stream_encoder_set_sample_rate(encoder,44100)
   assert lib.FLAC__stream_encoder_init_file(encoder,str((f/'pcm32.flac').resolve()).encode(),None,None)==0
   samples=(ctypes.c_int32*(len(data)//4)).from_buffer_copy(data)
   assert lib.FLAC__stream_encoder_process_interleaved(encoder,samples,len(data)//8)
   assert lib.FLAC__stream_encoder_finish(encoder)
  finally:
   lib.FLAC__stream_encoder_delete(encoder)

for name, args in [('float32', ['-c:a', 'pcm_f32le']), ('mono', ['-ac', '1', '-c:a', 'pcm_s16le'])]:
 subprocess.run(['ffmpeg','-y','-v','error','-i',str(f/'pcm16.wav'),*args,str(f/(name+'.wav'))],check=True)
