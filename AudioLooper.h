// AudioLooper.h

#include <Bela.h>
#include <vector>

class AudioLooper {
public:
    // Constructor
    AudioLooper(float bpm, int buttonPin)
        : bpm(bpm), buttonPin(buttonPin) {}

    // Function to calculate the buffer size based on BPM, subdivision, and sample rate
    void calculateBufferSize(float sampleRate, int mChanel) {
        float beatsPerSecond = bpm / 60.0;
        float secondsPerSubdivision = 1.0 / (beatsPerSecond * subdivisions[mChanel] / 4.0);
        bufferSizes[mChanel] = static_cast<int>(secondsPerSubdivision * sampleRate);
        audioBuffersL[mChanel].resize(bufferSizes[mChanel]);
        audioBuffersR[mChanel].resize(bufferSizes[mChanel]);
    }

    // Function to change the BPM on the fly
    void setBpm(float newBpm, float sampleRate) {
        bpm = newBpm;
        for(int i = 0; i < numSubdivisions; i++) {
        	calculateBufferSize(sampleRate, i);  // Recalculer la taille du buffer
        }
    }

    // Initialization function to set up the button
    void setupButton(BelaContext *context) {
        pinMode(context, 0, buttonPin, INPUT);
    }

    // Function to handle audio and the button
    void processAudio(BelaContext *context) {
        // Read the button state
        int buttonState = digitalRead(context, 0, buttonPin);

        // If the button is pressed, reload the buffer
        if (buttonState == HIGH) {
            resetBuffer();
        }

        for(unsigned int n = 0; n < context->audioFrames; n++) {
            float inputSignalL = audioRead(context, n, 0);  // Read the input signal on channel 0 (L)
            float inputSignalR = audioRead(context, n, 1);  // Read the input signal on channel 1 (R)
            // For each subdivision
	        for(int i = 0; i < numSubdivisions; i++) {
                // Fill the circular buffer with the subdivision samples
	            if (!isBufferFilled[i]) {
	                audioBuffersL[i][writeIndices[i]] = inputSignalL;
	                audioBuffersR[i][writeIndices[i]] = inputSignalR;
	                writeIndices[i]++;
	                if (writeIndices[i] >= bufferSizes[i]) {
	                    writeIndices[i] = 0;  // Reset the write index
	                    isBufferFilled[i] = true;  // The buffer is full
	                    isRepeating[i] = true;  // Enable repetition
	                }
	            }
                // Repeat the captured segment if the buffer is filled
		        if (isRepeating[i]) {
		            float outputSignalL = audioBuffersL[i][playbackIndices[i]];
		            float outputSignalR = audioBuffersR[i][playbackIndices[i]];
		            playbackIndices[i]++;
		            if (playbackIndices[i] >= bufferSizes[i]) {
		                playbackIndices[i] = 0;  // Réinitialiser l'indice de lecture
		            }

                    // Send the signal to stereo outputs based on the subdivision
		            int outputLeft = i * 2;  // Canaux gauche
		            int outputRight = i * 2 + 1;  // Canaux droit
		            audioWrite(context, n, outputLeft, outputSignalL);  // Canal gauche
		            audioWrite(context, n, outputRight, outputSignalR);  // Canal droit
		        } else {
                    // While the buffer is not filled, perform a normal pass-through
		            int outputLeft = i * 2;  // Canaux gauche
		            int outputRight = i * 2 + 1;  // Canaux droit
		            audioWrite(context, n, outputLeft, inputSignalL);  // Canal gauche
		            audioWrite(context, n, outputRight, inputSignalR);  // Canal droit
		        }
	        }
        }
    }

private:
    float bpm;  // Desired BPM
    static const int numSubdivisions = 4;  // Total number of subdivisions
    int subdivisions[numSubdivisions] = {16, 32, 64, 128};  // Array for each subdivision
    int bufferSizes[numSubdivisions];  // Array for the buffer size for each subdivision
    std::vector<float> audioBuffersL[numSubdivisions];  // Arrays for the circular buffers for each subdivision (L)
    std::vector<float> audioBuffersR[numSubdivisions];  // Arrays for the circular buffers for each subdivision (R)
    int writeIndices[numSubdivisions] = {0, 0, 0, 0}; // Array for the write indices for each subdivision
    int playbackIndices[numSubdivisions] = {0, 0, 0, 0};  // Array for the read indices for each subdivision
    bool isBufferFilled[numSubdivisions] = {false, false, false, false};  // Array to indicate if each buffer is full
    bool isRepeating[numSubdivisions] = {false, false, false, false};  // Array to enable repetition for each subdivision
    
    int buttonPin;

    // Reset the buffer
    void resetBuffer() {
    	for(int i = 0; i < numSubdivisions; i++) {
	        writeIndices[i] = 0;
	        isBufferFilled[i] = false;
	        isRepeating[i] = false;
    	}
    }
};

