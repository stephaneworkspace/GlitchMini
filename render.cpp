#include <Bela.h>
#include <vector>
#include <libraries/Gui/Gui.h>      				 // Need this to use the GUI
#include <libraries/GuiController/GuiController.h>   // Need this to use the GUI
#include "ClockProcessor.h"
#include "AudioLooper.h"

// Const
const bool SW_DEBUG = false;
const int CLOCKIN_96PPQN = 96;
const int CLOCKIN_48PPQN = 48;
const int CLOCKIN_16TH = 16;
const float THRESHOLD = 0.1;  // Seuil pour la détection du signal
const unsigned int DURATION_RELEASE = 2;  // Durée de relâchement du signal en millisecondes
const int SOURCE = 1;  // 0 = PPQN 96, 1 = PPQN 48, 2 = 16TH
int ledPin = 0; // Digital Pin D0
//int outputPin = 1;  // Digital Pin D1 --> TOO DANGEROUS BECAUSE 3.3V MAX !
int resetPin = 1;  // Analog Pin A1
int gShutdownPin = 3;  // Shutdown button
bool buttonShutdownPressed = false;
unsigned int ledState = 0;
unsigned long ledLastChangeTime = 0;
unsigned long ledOnDuration = 100;

// Create objects
ClockProcessor clockProcessor(SOURCE, THRESHOLD);
AudioLooper looper(149.0, 7); // PushButton on D7

bool setup(BelaContext *context, void *userData)
{
    pinMode(context, 0, ledPin, OUTPUT);  // Configurer la LED
    //pinMode(context, 0, outputPin, OUTPUT);  // Configurer la sortie
    pinMode(context, 0, gShutdownPin, INPUT);  // Configurer le bouton de shutdown

    // Configurer le source et ajuster les paramètres
    switch (SOURCE) {
        case 0: clockProcessor.setupSource(CLOCKIN_96PPQN); ledOnDuration *= 4; break;
        case 1: clockProcessor.setupSource(CLOCKIN_48PPQN); ledOnDuration *= 2; break;
        case 2: clockProcessor.setupSource(CLOCKIN_16TH); break;
        default: clockProcessor.setupSource(CLOCKIN_48PPQN); ledOnDuration *= 2; break;
    }

    looper.calculateBufferSize(context->audioSampleRate, 0);  // Initialiser la taille du buffer
    looper.calculateBufferSize(context->audioSampleRate, 1);  // Initialiser la taille du buffer
    looper.calculateBufferSize(context->audioSampleRate, 2);  // Initialiser la taille du buffer
    looper.calculateBufferSize(context->audioSampleRate, 3);  // Initialiser la taille du buffer
    looper.setupButton(context);  // Configurer le bouton D7

    return true;
}

void render(BelaContext *context, void *userData)
{
    unsigned long currentTime = context->audioFramesElapsed / (context->audioSampleRate / 1000);
    for(unsigned int n = 0; n < context->audioFrames; n++) {
        // Shutdown btn
        int buttonState = digitalRead(context, n, gShutdownPin);
        if(buttonState == LOW && !buttonShutdownPressed) {
            buttonShutdownPressed = true;
            rt_printf("Shutdown button pressed!\n");
            usleep(500000); // Petit délai
            system("shutdown -h now");
        }
        if(buttonState == HIGH && buttonShutdownPressed) {
            buttonShutdownPressed = false;
        }

        // Reset
        float resetValue = analogRead(context, n/2, resetPin);
        if (resetValue > 0.5) {
            clockProcessor.resetPulseCounters();
            if (SW_DEBUG)
            	rt_printf("Reset detected, pulseCounter reset!\n");
        }

        // Read clock
        float analogValue = analogRead(context, n/2, 0); // A0
        clockProcessor.handleClockPulse(context, n, analogValue, currentTime);
        int pulseCounter = clockProcessor.getPulseCounter();
        if (clockProcessor.getAverageBPM() > 0 && pulseCounter == 0) {
            digitalWrite(context, n, ledPin, HIGH);  // Turn on the LED
            //digitalWrite(context, n, outputPin, HIGH);  // Envoyer le trigger sur D1
            ledLastChangeTime = currentTime;
            looper.setBpm(clockProcessor.getAverageBPM(), context->audioSampleRate);
            if (SW_DEBUG)
            	rt_printf("LED ON, Trigger ON, BPM: %.2f\n", clockProcessor.getAverageBPM());
        }

        // Shutdown LED after some time
        if (digitalRead(context, n, ledPin) == HIGH && currentTime - ledLastChangeTime > ledOnDuration) {
            digitalWrite(context, n, ledPin, LOW);  // Éteindre la LED
            //digitalWrite(context, n, outputPin, LOW);  // Remettre le trigger à LOW
            //rt_printf("LED OFF, Trigger OFF after %lu ms\n", currentTime - ledLastChangeTime);
        }
    }
    looper.processAudio(context); 
}

void cleanup(BelaContext *context, void *userData)
{
}