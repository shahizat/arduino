/* Edge Impulse ingestion SDK
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// If your target is limited in memory remove this macro to save 10K RAM
#define EIDSP_QUANTIZE_FILTERBANK   0


/*
 ** NOTE: If you run into TFLite arena allocation issue.
 **
 ** This may be due to may dynamic memory fragmentation.
 ** Try defining "-DEI_CLASSIFIER_ALLOCATION_STATIC" in boards.local.txt (create
 ** if it doesn't exist) and copy this file to
 ** `<ARDUINO_CORE_INSTALL_PATH>/arduino/hardware/<mbed_core>/<core_version>/`.
 **
 ** See
 ** (https://support.arduino.cc/hc/en-us/articles/360012076960-Where-are-the-installed-cores-located-)
 ** to find where Arduino installs cores on your machine.
 **
 ** If the problem persists then there's not enough memory for this model and application.
 */

/* Includes ---------------------------------------------------------------- */
#include <PDM.h>
#include <Audio_classification_v1_inferencing.h>
#include "Arduino_GigaDisplay_GFX.h"

#define screen_size_x 480
#define screen_size_y 800

GigaDisplay_GFX display; // create the object
#define BLACK 0x0000

/** Audio buffers, pointers and selectors */
typedef struct {
    int16_t *buffer;
    uint8_t buf_ready;
    uint32_t buf_count;
    uint32_t n_samples;
} inference_t;

static inference_t inference;
static signed short sampleBuffer[2048];
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
//static const int frequency = 20000; 
/**
 * @brief      Arduino setup function
 */
void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);
    // comment out the below line to cancel the wait for USB connection (needed for native USB)
    while (!Serial);
    Serial.println("Edge Impulse Inferencing Demo");
    display.begin();
    display.fillScreen(BLACK);
    display.setTextSize(2);
    display.setRotation(1);
    delay(1000);
    // summary of inferencing settings (from model_metadata.h)
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: %.2f ms.\n", (float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));

    if (microphone_inference_start(EI_CLASSIFIER_RAW_SAMPLE_COUNT) == false) {
        ei_printf("ERR: Could not allocate audio buffer (size %d), this could be due to the window length of your model\r\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
        return;
    }
}

/**
 * @brief      Arduino main function. Runs the inferencing loop.
 */
void loop()
{
    ei_printf("Starting inferencing in 2 seconds...\n");

    delay(2000);

    ei_printf("Recording...\n");

    bool m = microphone_inference_record();
    if (!m) {
        ei_printf("ERR: Failed to record audio...\n");
        return;
    }

    ei_printf("Recording done\n");

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &microphone_audio_signal_get_data;
    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR r = run_classifier(&signal, &result, debug_nn);
    if (r != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", r);
        return;
    }

    // print the predictions
    ei_printf("Predictions ");
    ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
        result.timing.dsp, result.timing.classification, result.timing.anomaly);
    ei_printf(": \n");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
    }

    const char *resultLabel = getMax(result.classification);
    display.fillScreen(BLACK);
    Serial.println(resultLabel);
    //display.println(resultLabel);
    int artLength = 9; // Change this value based on the length of your ASCII art
    int centerX = (screen_size_x - (artLength * 10)) / 2; // 10 is the approximate width of each character in the font

   if (strcmp(resultLabel, "speech") == 0) {
        display.fillScreen(BLACK);
        // Display ASCII art for speech
        display.setCursor(100, 150);
        display.println("\n");
        // Print each line of the ASCII art
        display.setCursor(100, 160);
        display.println(" #####  ######  ####### #######  #####  #     # ");
        display.setCursor(100, 180);
        display.println("#     # #     # #       #       #     # #     # ");
        display.setCursor(100, 200);
        display.println("#       #     # #       #       #       #     # ");
        display.setCursor(100, 220);
        display.println(" #####  ######  #####   #####   #       ####### ");
        display.setCursor(100, 240);
        display.println("      # #       #       #       #       #     # ");
        display.setCursor(100, 260);
        display.println("#     # #       #       #       #     # #     # ");
        display.setCursor(100, 280);
        display.println(" #####  #       ####### #######  #####  #     # \n");
    } else if (strcmp(resultLabel, "music") == 0) {
      display.fillScreen(BLACK);
      // Display ASCII art for music
      display.setCursor(200, 150);
      display.println("\n");
      // Print each line of the ASCII art
      display.setCursor(200, 160);
      display.println("#     # #     #  #####  ###  #####  ");
      display.setCursor(200, 180);
      display.println("##   ## #     # #     #  #  #     # ");
      display.setCursor(200, 200);
      display.println("# # # # #     # #        #  #       ");
      display.setCursor(200, 220);
      display.println("#  #  # #     #  #####   #  #       ");
      display.setCursor(200, 240);
      display.println("#     # #     #       #  #  #       ");
      display.setCursor(200, 260);
      display.println("#     # #     # #     #  #  #     # ");
      display.setCursor(200, 280);
      display.println("#     #  #####   #####  ###  #####  \n");
    } else if (strcmp(resultLabel, "noise") == 0) {
        display.fillScreen(BLACK);
      // Display ASCII art for noise
      display.setCursor(200, 150);
      display.println("\n");
      // Print each line of the ASCII art
      display.setCursor(200, 160);
      display.println("#     # ####### ###  #####  ####### ");
      display.setCursor(200, 180);
      display.println("##    # #     #  #  #     # #       ");
      display.setCursor(200, 200);
      display.println("# #   # #     #  #  #       #       ");
      display.setCursor(200, 220);
      display.println("#  #  # #     #  #   #####  #####   ");
      display.setCursor(200, 240);
      display.println("#   # # #     #  #        # #       ");
      display.setCursor(200, 260);
      display.println("#    ## #     #  #  #     # #       ");
      display.setCursor(200, 280);
      display.println("#     # ####### ###  #####  ####### \n");
        }


#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif
}

const char* getMax(ei_impulse_result_classification_t *classifications)
{
    uint8_t maxLabelIndex = 0;
    for (size_t i = 1; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (classifications[i].value > classifications[maxLabelIndex].value) {
            maxLabelIndex = i;
        }
    }

    return classifications[maxLabelIndex].label;
}

/**
 * @brief      PDM buffer full callback
 *             Get data and call audio thread callback
 */
static void pdm_data_ready_inference_callback(void)
{
    int bytesAvailable = PDM.available();

    // read into the sample buffer
    int bytesRead = PDM.read((char *)&sampleBuffer[0], bytesAvailable);

    if (inference.buf_ready == 0) {
        for(int i = 0; i < bytesRead>>1; i++) {
            inference.buffer[inference.buf_count++] = sampleBuffer[i];

            if(inference.buf_count >= inference.n_samples) {
                inference.buf_count = 0;
                inference.buf_ready = 1;
                break;
            }
        }
    }
}

/**
 * @brief      Init inferencing struct and setup/start PDM
 *
 * @param[in]  n_samples  The n samples
 *
 * @return     { description_of_the_return_value }
 */
static bool microphone_inference_start(uint32_t n_samples)
{
    inference.buffer = (int16_t *)malloc(n_samples * sizeof(int16_t));

    if(inference.buffer == NULL) {
        return false;
    }

    inference.buf_count  = 0;
    inference.n_samples  = n_samples;
    inference.buf_ready  = 0;

    // configure the data receive callback
    PDM.onReceive(&pdm_data_ready_inference_callback);

    PDM.setBufferSize(512);

    // initialize PDM with:
    // - one channel (mono mode)
    // - a 16 kHz sample rate
    if (!PDM.begin(1, 20000)) {
        ei_printf("Failed to start PDM!");
        microphone_inference_end();

        return false;
    }

    // set the gain, defaults to 20
    PDM.setGain(5);

    return true;
}

/**
 * @brief      Wait on new data
 *
 * @return     True when finished
 */
static bool microphone_inference_record(void)
{
    inference.buf_ready = 0;
    inference.buf_count = 0;

    while(inference.buf_ready == 0) {
        delay(10);
    }

    return true;
}

/**
 * Get raw audio signal data
 */
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr)
{
    numpy::int16_to_float(&inference.buffer[offset], out_ptr, length);

    return 0;
}

/**
 * @brief      Stop PDM and release buffers
 */
static void microphone_inference_end(void)
{
    PDM.end();
    free(inference.buffer);
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif 