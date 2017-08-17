from scipy.fftpack import fft
from scipy.io import wavfile
from scipy.signal import hamming
import numpy as np
import sys

def fft_file (file_name):
    sf, data = wavfile.read(file_name)
    size = len (data)
    #15ms * sf = 240, 10ms * sf = 160. cut the file to fit the length of frame
    cnt = 0
    max_cnt = (size-(240)) / (160)

    spectrum_map = np.zeros((max_cnt, 400, 3), dtype = np.complex128)
    while (cnt < max_cnt): #do fft for each frame(25ms)
        begin_point = cnt*160
        spectrum_map[cnt] = fft(data[begin_point:begin_point+400]*hamming(3, 400))
        cnt += 1
    return spectrum_map

def get_phase_map (spectrum_map) :
    phase_map   = np.zeros((spectrum_map.shape), dtype = np.float32)
    ptr_frame   = phase_map.shape[0]
    ptr_point   = phase_map.shape[1]
    ptr_channal = phase_map.shape[2]
    while ptr_frame != 0:
        ptr_frame -= 1
        while ptr_point != 0:
            ptr_point -= 1
            while ptr_channal != 0:
                ptr_channal -= 1
                phase_map[ptr_frame][ptr_point][ptr_channal] = np.arctan2(
                        phase_map[ptr_frame][ptr_point][ptr_channal].imag, 
                        phase_map[ptr_frame][ptr_point][ptr_channal].real
                        )*np.pi
    return phase_map

def write_phase_map (phase_map, label_map, outprefix = "./mxfeat_doa_test_use_only"):
    index = 0
    max_index = phase_map.shape[0]
    record_io = mx.recordio.MXRecordIO(outfile, 'w')
    while index < max_index:
        recordio.write(phase_map[index].tostring() + label_map[index].tostring() + struct.pack('Q', index))
