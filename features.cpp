#include "features.h"

// source : https://www.codeproject.com/Articles/525620/Recording-Audio-to-WAV-with-WASAPI-in-Windows-Stor

static const int BUFFER_DURATION = 1;
static const int CHANNEL_COUNT = 2;
static const int SAMPLE_RATE = 48000;
static const int BUFFER_SIZE = 10;
static const int NUMBER_OF_BUFFERS = 5;
const int DATA_SIZE = SAMPLE_RATE * BUFFER_DURATION * CHANNEL_COUNT;

HWAVEIN hWaveIn;
WAVEHDR WaveHeaders[NUMBER_OF_BUFFERS];
static std::ofstream audioFile;


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



void WriteWavHeader(std::ofstream& file, int sampleRate, int bitsPerSample, int channels, int dataSize) {
    // Write the RIFF header
    file.write("RIFF", 4);
    DWORD totalFileSize = dataSize + 36;
    file.write(reinterpret_cast<const char*>(&totalFileSize), 4);
    file.write("WAVE", 4);

    // Write the "fmt " subchunk
    file.write("fmt ", 4);
    DWORD fmtSize = 16;
    file.write(reinterpret_cast<const char*>(&fmtSize), 4);
    WORD audioFormat = 1; // PCM
    file.write(reinterpret_cast<const char*>(&audioFormat), 2);
    file.write(reinterpret_cast<const char*>(&channels), 2);
    file.write(reinterpret_cast<const char*>(&sampleRate), 4);
    DWORD byteRate = sampleRate * channels * bitsPerSample / 8;
    file.write(reinterpret_cast<const char*>(&byteRate), 4);
    WORD blockAlign = channels * bitsPerSample / 8;
    file.write(reinterpret_cast<const char*>(&blockAlign), 2);
    file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    // Write the "data" subchunk
    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&dataSize), 4);
}

void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg == WIM_DATA) {
        WAVEHDR* pWaveHeader = (WAVEHDR*)dwParam1;

        audioFile.write(pWaveHeader->lpData, pWaveHeader->dwBufferLength);

        waveInUnprepareHeader(hWaveIn, pWaveHeader, sizeof(WAVEHDR));
        delete[] pWaveHeader->lpData;

        pWaveHeader->lpData = new char[DATA_SIZE];
        pWaveHeader->dwBufferLength = DATA_SIZE;
        waveInPrepareHeader(hWaveIn, pWaveHeader, sizeof(WAVEHDR));
        waveInAddBuffer(hWaveIn, pWaveHeader, sizeof(WAVEHDR));
    }
}

void features::RecordMicrophone() {
    WAVEFORMATEX waveform;
    waveform.wFormatTag = WAVE_FORMAT_PCM;
    waveform.nChannels = CHANNEL_COUNT;
    waveform.nSamplesPerSec = SAMPLE_RATE;
    waveform.nAvgBytesPerSec = SAMPLE_RATE * CHANNEL_COUNT * (waveform.wBitsPerSample / 8);
    waveform.wBitsPerSample = 16; // Set to 16 bits per sample
    waveform.nBlockAlign = (waveform.wBitsPerSample / 8) * CHANNEL_COUNT;
    waveform.cbSize = 0;

    MMRESULT result = waveInOpen(&hWaveIn, WAVE_MAPPER, &waveform, (DWORD_PTR)waveInProc, 0, CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "waveInOpen failed with error code: " << result << std::endl;
        return;
    }

    audioFile.open("recorded_audio.wav", std::ios::binary);

    // Write the WAV file header
    WriteWavHeader(audioFile, SAMPLE_RATE, 16, CHANNEL_COUNT, 0);

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

    std::cout << "Recording is in progress...." << std::endl;
    Sleep(5 * 1000);

    // Clean up and stop recording
    waveInStop(hWaveIn);
    waveInReset(hWaveIn);
    std::cout << "Recording ended...." << std::endl;
    for (int i = 0; i < NUMBER_OF_BUFFERS; i++) {
        waveInUnprepareHeader(hWaveIn, &WaveHeaders[i], sizeof(WAVEHDR));
        delete[] WaveHeaders[i].lpData;
    }

    // Calculate and update the data size in the header
    std::streampos fileSize = audioFile.tellp();
    DWORD dataSize = static_cast<DWORD>(fileSize) - 44;
    audioFile.seekp(40, std::ios::beg); // Move to the position where the data size is stored
    audioFile.write(reinterpret_cast<const char*>(&dataSize), 4);

    waveInClose(hWaveIn);
    audioFile.close();


}



