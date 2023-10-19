#include "features.h"

void features::PrintAvailableAudioDevices() {
    UINT numDevices = waveInGetNumDevs();

    if (numDevices == 0) {
        std::cout << "No audio input devices found." << std::endl;
        return;
    }

    std::cout << "Available audio input devices:" << std::endl;

    for (UINT deviceId = 0; deviceId < numDevices; deviceId++) {
        WAVEINCAPS deviceInfo;
        MMRESULT result = waveInGetDevCaps(deviceId, &deviceInfo, sizeof(WAVEINCAPS));

        if (result == MMSYSERR_NOERROR) {
            std::cout << "Device " << deviceId << ": " << deviceInfo.szPname << std::endl;
        } else {
            std::cerr << "Error getting device information for Device " << deviceId << std::endl;
        }
    }
}

const int BUFFER_DURATION = 1;
const int CHANNEL_COUNT = 1;
const int SAMPLE_RATE = 44100;
const int DATA_SIZE = SAMPLE_RATE * BUFFER_DURATION * CHANNEL_COUNT;
const int BUFFER_SIZE = 10;
const int NUMBER_OF_BUFFERS = 5;

HWAVEIN hWaveIn;
WAVEHDR WaveHeaders[NUMBER_OF_BUFFERS];
std::ofstream audioFile;

void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg == WIM_DATA) {
        WAVEHDR* pWaveHeader = (WAVEHDR*)dwParam1;

        audioFile.write(pWaveHeader->lpData, pWaveHeader->dwBufferLength);

        waveInAddBuffer(hWaveIn, pWaveHeader, sizeof(WAVEHDR));
    }
}

void features::RecordMicrophone() {


    WAVEFORMATEX waveform;
    waveform.wFormatTag = WAVE_FORMAT_PCM;
    waveform.nChannels = CHANNEL_COUNT;
    waveform.nSamplesPerSec = SAMPLE_RATE;
    waveform.nAvgBytesPerSec = SAMPLE_RATE * CHANNEL_COUNT * (waveform.wBitsPerSample / 8);
    waveform.nBlockAlign = (waveform.wBitsPerSample / 8) * CHANNEL_COUNT;
    waveform.wBitsPerSample = 8 * CHANNEL_COUNT;
    waveform.cbSize = 0;

    MMRESULT result = waveInOpen(&hWaveIn, WAVE_MAPPER, &waveform, (DWORD_PTR)waveInProc, 0, CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "waveInOpen failed with error code: " << result << std::endl;
        return;
    }

    audioFile.open("recorded_audio.wav", std::ios::binary);

    for (int i = 0; i < NUMBER_OF_BUFFERS; i++) {
        WaveHeaders[i].lpData = new char[DATA_SIZE];
        WaveHeaders[i].dwBufferLength = DATA_SIZE;
        result = waveInPrepareHeader(hWaveIn, &WaveHeaders[i], sizeof(WAVEHDR));
        if (result != MMSYSERR_NOERROR) {
            std::cerr << "waveInPrepareHeader failed with error code: " << result << std::endl;
            return;
        }
        result = waveInAddBuffer(hWaveIn, &WaveHeaders[i], sizeof(WAVEHDR));
        if (result != MMSYSERR_NOERROR) {
            std::cerr << "waveInAddBuffer failed with error code: " << result << std::endl;
            return;
        }
    }

    result = waveInStart(hWaveIn);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "waveInStart failed with error code: " << result << std::endl;
        return;
    }

    std::cout << "Recording is in progress. Press Enter to stop..." << std::endl;
    std::cin.get();

    // Clean up and stop recording
    waveInStop(hWaveIn);
    waveInReset(hWaveIn);

    for (int i = 0; i < NUMBER_OF_BUFFERS; i++) {
        waveInUnprepareHeader(hWaveIn, &WaveHeaders[i], sizeof(WAVEHDR));
        delete[] WaveHeaders[i].lpData;
    }

    waveInClose(hWaveIn);
    audioFile.close();
}



