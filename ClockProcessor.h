// ClockProcessor.h

#ifndef CLOCKPROCESSOR_H
#define CLOCKPROCESSOR_H

#include <vector>
#include <math.h>
#include <Bela.h>
#include <libraries/Scope/Scope.h>

class ClockProcessor {
public:
    // Constructor
    ClockProcessor(int source, float threshold)
        : source(source), threshold(threshold), pulseCounter(0), outputPulseCounter(0), 
          previousState(false), signalActive(false), signalStartTime(0),
          averageBPM(120.0), smoothingFactor(0.3) {}

    // Initialisation
    void setupSource(int pulsesPerBeat) {
        pulseDurations.resize(pulsesPerBeat);
        this->pulsesPerBeat = pulsesPerBeat;
    }

    // Handle clock pulses
    void handleClockPulse(BelaContext *context, unsigned int n, float analogValue, unsigned long currentTime) {
        if (analogValue > threshold && !previousState) {
            pulseCounter++;
            pulseDurations[pulseCounter - 1] = currentTime;

            if (pulseCounter >= pulsesPerBeat) {
                pulseCounter = 0;
                handleBPMCalculation(currentTime);
            }

            outputPulseCounter++;
            if (outputPulseCounter >= pulsesPerBeat) {
                signalStartTime = currentTime;
                signalActive = true;
                outputPulseCounter = 0;
            }
        }

        previousState = (analogValue > threshold);
    }

    // Calculate the average BPM
    /*
    void handleBPMCalculation(unsigned long currentTime) {
        unsigned long totalDuration = pulseDurations[pulsesPerBeat - 1] - pulseDurations[0];
        float averagePulseDuration = totalDuration / (float)pulsesPerBeat;
        float bpm = (60.0 * 1000.0) / (averagePulseDuration * pulsesPerBeat);
        averageBPM = lowPassFilter(bpm, averageBPM, smoothingFactor);
    }*/
	void handleBPMCalculation(unsigned long currentTime) {
	    unsigned long totalDuration = pulseDurations[pulsesPerBeat - 1] - pulseDurations[0];
	    float averagePulseDuration = totalDuration / (float)pulsesPerBeat;
	    float bpm = (60.0 * 1000.0) / (averagePulseDuration * pulsesPerBeat);
	    
	    // Si le BPM calculé est très éloigné, sauter le lissage pour réagir plus vite
	    if (fabs(bpm - averageBPM) > 10.0) {  // Par exemple, si la différence est supérieure à 10 BPM
	        averageBPM = bpm;  // Réagir directement sans filtre pour des variations importantes
	    } else {
	        averageBPM = lowPassFilter(bpm, averageBPM, smoothingFactor);  // Filtrer seulement les petites variations
	    }
	}

    // Get the current average BPM
    float getAverageBPM() const {
        return averageBPM;
    }

    // Check if the signal is still active
    bool isSignalActive(unsigned long currentTime, unsigned int durationInSamples) const {
        return signalActive && (currentTime - signalStartTime < durationInSamples);
    }

    // Reset the pulse counters
    void resetPulseCounters() {
        pulseCounter = 0;
        outputPulseCounter = 0;
    }

    // New method to retrieve the current value of pulseCounter
    int getPulseCounter() const {
        return pulseCounter;
    }

private:
    int source;
    float threshold;
    int pulseCounter, outputPulseCounter, pulsesPerBeat;
    bool previousState;
    unsigned long signalStartTime;
    bool signalActive;
    float averageBPM;
    float smoothingFactor;
    std::vector<float> pulseDurations;

    // Low-pass filter to smooth out BPM variations
    float lowPassFilter(float currentValue, float previousValue, float alpha) {
        return alpha * currentValue + (1.0 - alpha) * previousValue;
    }
};

#endif // CLOCKPROCESSOR_H